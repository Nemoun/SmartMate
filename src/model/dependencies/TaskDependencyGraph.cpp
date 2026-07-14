#include "dependencies/TaskDependencyGraph.h"

#include <QHash>
#include <QSet>

#include <algorithm>
#include <utility>

namespace smartmate::model {
namespace {

[[nodiscard]] QString stableId(const TaskId &taskId)
{
    // UUID 文本用于确定性排序和边键，不作为新的业务身份。
    return taskId.toString(QUuid::WithoutBraces);
}

void normalizeIds(QList<TaskId> &taskIds)
{
    // 所有集合型结果统一排序并去重，使输出与 Repository 返回顺序无关。
    std::sort(taskIds.begin(), taskIds.end(), [](const TaskId &left,
                                                 const TaskId &right) {
        return stableId(left) < stableId(right);
    });
    taskIds.erase(std::unique(taskIds.begin(), taskIds.end()), taskIds.end());
}

[[nodiscard]] QString edgeKey(const TaskDependency &dependency)
{
    // 有向边键保留前置→后继方向，用于识别完全相同的重复关系。
    return stableId(dependency.predecessorId) + QLatin1Char('>')
        + stableId(dependency.successorId);
}

} // namespace

TaskDependencyGraph::TaskDependencyGraph(QList<Task> tasks,
                                         QList<TaskDependency> dependencies)
    : m_tasks(std::move(tasks))
    , m_dependencies(std::move(dependencies))
{
    // 预建 O(1) 平均查找索引，避免长链算法反复线性扫描任务列表。
    m_taskIndexes.reserve(m_tasks.size());
    for (qsizetype index = 0; index < m_tasks.size(); ++index) {
        m_taskIndexes.insert(m_tasks.at(index).id(), index);
    }
}

DependencyGraphValidation TaskDependencyGraph::validation() const
{
    // 校验顺序固定，保证同一坏快照始终返回相同的首要错误语义。
    QList<TaskId> missingTaskIds;
    for (const TaskDependency &dependency : m_dependencies) {
        if (!containsTask(dependency.predecessorId)) {
            missingTaskIds.append(dependency.predecessorId);
        }
        if (!containsTask(dependency.successorId)) {
            missingTaskIds.append(dependency.successorId);
        }
    }
    normalizeIds(missingTaskIds);
    if (!missingTaskIds.isEmpty()) {
        return {DependencyGraphError::MissingTask, missingTaskIds, {}};
    }

    QList<TaskId> selfDependencies;
    for (const TaskDependency &dependency : m_dependencies) {
        if (dependency.predecessorId == dependency.successorId) {
            selfDependencies.append(dependency.predecessorId);
        }
    }
    normalizeIds(selfDependencies);
    if (!selfDependencies.isEmpty()) {
        return {DependencyGraphError::SelfDependency, selfDependencies, {}};
    }

    // 端点合法后再检查重复边，错误上下文只报告真实任务身份。
    QSet<QString> knownEdges;
    QList<TaskId> duplicateEndpoints;
    for (const TaskDependency &dependency : m_dependencies) {
        const QString key = edgeKey(dependency);
        if (knownEdges.contains(key)) {
            duplicateEndpoints.append(dependency.predecessorId);
            duplicateEndpoints.append(dependency.successorId);
        } else {
            knownEdges.insert(key);
        }
    }
    normalizeIds(duplicateEndpoints);
    if (!duplicateEndpoints.isEmpty()) {
        return {DependencyGraphError::DuplicateDependency, duplicateEndpoints, {}};
    }

    // 邻接表按稳定 ID 排序，确保 DFS 发现的循环路径可以确定性测试和展示。
    QHash<TaskId, QList<TaskId>> successorsByTask;
    for (const TaskDependency &dependency : m_dependencies) {
        successorsByTask[dependency.predecessorId].append(dependency.successorId);
    }
    for (auto iterator = successorsByTask.begin(); iterator != successorsByTask.end();
         ++iterator) {
        normalizeIds(iterator.value());
    }

    QList<TaskId> orderedTaskIds;
    orderedTaskIds.reserve(m_tasks.size());
    for (const Task &task : m_tasks) {
        orderedTaskIds.append(task.id());
    }
    normalizeIds(orderedTaskIds);

    // 0=未访问、1=当前DFS路径、2=已完成。显式栈避免极长依赖链耗尽调用栈。
    QHash<TaskId, int> colors;
    QList<TaskId> cyclePath;
    struct DfsFrame final {
        /// 当前显式递归帧对应的任务。
        TaskId taskId;
        /// 下一个尚未访问的后继下标。
        qsizetype nextSuccessorIndex{0};
    };

    for (const TaskId &rootTaskId : orderedTaskIds) {
        if (colors.value(rootTaskId, 0) != 0) {
            continue;
        }

        QList<DfsFrame> stack;
        stack.append({rootTaskId, 0});
        colors.insert(rootTaskId, 1);
        while (!stack.isEmpty()) {
            DfsFrame &frame = stack.last();
            const auto successors = successorsByTask.constFind(frame.taskId);
            if (successors == successorsByTask.cend()
                || frame.nextSuccessorIndex >= successors.value().size()) {
                colors.insert(frame.taskId, 2);
                stack.removeLast();
                continue;
            }

            const TaskId successorId =
                successors.value().at(frame.nextSuccessorIndex++);
            const int successorColor = colors.value(successorId, 0);
            if (successorColor == 0) {
                colors.insert(successorId, 1);
                stack.append({successorId, 0});
                continue;
            }
            if (successorColor != 1) {
                continue;
            }

            // 指向当前 DFS 路径中的灰色节点即为回边；截取并闭合实际循环路径。
            qsizetype cycleStart = 0;
            while (cycleStart < stack.size()
                   && stack.at(cycleStart).taskId != successorId) {
                ++cycleStart;
            }
            for (qsizetype index = cycleStart; index < stack.size(); ++index) {
                cyclePath.append(stack.at(index).taskId);
            }
            cyclePath.append(successorId);

            QList<TaskId> conflictingIds = cyclePath;
            normalizeIds(conflictingIds);
            return {DependencyGraphError::Cycle, conflictingIds, cyclePath};
        }
    }

    return {};
}

bool TaskDependencyGraph::containsTask(const TaskId &taskId) const
{
    return findTask(taskId) != nullptr;
}

QList<TaskId> TaskDependencyGraph::predecessorIds(const TaskId &taskId) const
{
    QList<TaskId> result;
    for (const TaskDependency &dependency : m_dependencies) {
        if (dependency.successorId == taskId) {
            result.append(dependency.predecessorId);
        }
    }
    normalizeIds(result);
    return result;
}

QList<TaskId> TaskDependencyGraph::successorIds(const TaskId &taskId) const
{
    QList<TaskId> result;
    for (const TaskDependency &dependency : m_dependencies) {
        if (dependency.predecessorId == taskId) {
            result.append(dependency.successorId);
        }
    }
    normalizeIds(result);
    return result;
}

QList<TaskId> TaskDependencyGraph::unsatisfiedPredecessorIds(const TaskId &taskId) const
{
    // AND 语义下只要结果非空，后继就仍被至少一个 Pending 前置阻塞。
    QList<TaskId> result;
    for (const TaskId &predecessorId : predecessorIds(taskId)) {
        const Task *predecessor = findTask(predecessorId);
        if (predecessor == nullptr
            || dependencyResolution(*predecessor)
                == TaskDependencyResolution::Pending) {
            result.append(predecessorId);
        }
    }
    return result;
}

QList<TaskId> TaskDependencyGraph::cancelledPredecessorIds(
    const TaskId &taskId) const
{
    QList<TaskId> result;
    for (const TaskId &predecessorId : predecessorIds(taskId)) {
        const Task *predecessor = findTask(predecessorId);
        if (predecessor != nullptr
            && dependencyResolution(*predecessor)
                == TaskDependencyResolution::Cancelled) {
            result.append(predecessorId);
        }
    }
    return result;
}

TaskDependencyState TaskDependencyGraph::dependencyState(const TaskId &taskId) const
{
    TaskDependencyState state;
    state.predecessorIds = predecessorIds(taskId);
    state.unsatisfiedPredecessorIds = unsatisfiedPredecessorIds(taskId);
    state.cancelledPredecessorIds = cancelledPredecessorIds(taskId);

    // 终态仍保留诊断列表，但只有 Todo/InProgress 会被标记为当前阻塞。
    const Task *task = findTask(taskId);
    const bool active = task != nullptr
        && (task->status() == TaskStatus::Todo
            || task->status() == TaskStatus::InProgress);
    state.blocked = active && !state.unsatisfiedPredecessorIds.isEmpty();

    // 只有当前唯一阻塞前置恰好是本任务时，完成或取消本任务才会直接解锁后继。
    for (const TaskId &successorId : successorIds(taskId)) {
        const Task *successor = findTask(successorId);
        if (successor == nullptr || successor->status() != TaskStatus::Todo) {
            continue;
        }
        const QList<TaskId> blockers = unsatisfiedPredecessorIds(successorId);
        if (blockers.size() == 1 && blockers.constFirst() == taskId) {
            ++state.unlockCount;
        }
    }
    return state;
}

const QList<TaskDependency> &TaskDependencyGraph::dependencies() const noexcept
{
    return m_dependencies;
}

QHash<TaskId, int> TaskDependencyGraph::dependencyLevels() const
{
    if (!validation().ok()) {
        return {};
    }

    // levels 保存最长已知路径；indegrees 控制节点在全部前置完成后出队。
    QHash<TaskId, int> levels;
    QHash<TaskId, int> indegrees;
    QHash<TaskId, QList<TaskId>> successorsByTask;
    QList<TaskId> readyTaskIds;
    for (const Task &task : m_tasks) {
        levels.insert(task.id(), 0);
        indegrees.insert(task.id(), 0);
    }
    for (const TaskDependency &dependency : m_dependencies) {
        ++indegrees[dependency.successorId];
        successorsByTask[dependency.predecessorId].append(dependency.successorId);
    }
    for (auto iterator = successorsByTask.begin(); iterator != successorsByTask.end();
         ++iterator) {
        normalizeIds(iterator.value());
    }
    for (const Task &task : m_tasks) {
        if (indegrees.value(task.id()) == 0) {
            readyTaskIds.append(task.id());
        }
    }
    normalizeIds(readyTaskIds);

    // Kahn 遍历保证只有在全部前置处理完毕后才确定层级，菱形图会选择最长路径。
    qsizetype nextReadyIndex = 0;
    qsizetype visitedCount = 0;
    while (nextReadyIndex < readyTaskIds.size()) {
        const TaskId taskId = readyTaskIds.at(nextReadyIndex++);
        ++visitedCount;
        for (const TaskId &successorId : successorsByTask.value(taskId)) {
            levels[successorId] = std::max(levels.value(successorId),
                                           levels.value(taskId) + 1);
            const int remainingPredecessors = indegrees.value(successorId) - 1;
            indegrees.insert(successorId, remainingPredecessors);
            if (remainingPredecessors == 0) {
                readyTaskIds.append(successorId);
            }
        }
    }

    // validation 已排除环；此防线避免未来调用路径在异常快照上返回半套层级。
    return visitedCount == m_tasks.size() ? levels : QHash<TaskId, int>{};
}

namespace {

QList<TaskId> directedClosure(const TaskId &taskId,
                              const QHash<TaskId, QList<TaskId>> &adjacency)
{
    // 使用可增长队列迭代遍历，visited 同时防重复并抵御异常回边。
    QList<TaskId> pending = adjacency.value(taskId);
    QSet<TaskId> visited;
    qsizetype index = 0;
    while (index < pending.size()) {
        const TaskId current = pending.at(index++);
        if (visited.contains(current)) {
            continue;
        }
        visited.insert(current);
        for (const TaskId &next : adjacency.value(current)) {
            if (!visited.contains(next)) {
                pending.append(next);
            }
        }
    }
    QList<TaskId> result(visited.cbegin(), visited.cend());
    normalizeIds(result);
    return result;
}

} // namespace

QList<TaskId> TaskDependencyGraph::predecessorClosure(const TaskId &taskId) const
{
    if (!containsTask(taskId) || !validation().ok()) {
        return {};
    }
    QHash<TaskId, QList<TaskId>> predecessors;
    for (const TaskDependency &dependency : m_dependencies) {
        predecessors[dependency.successorId].append(dependency.predecessorId);
    }
    return directedClosure(taskId, predecessors);
}

QList<TaskId> TaskDependencyGraph::successorClosure(const TaskId &taskId) const
{
    if (!containsTask(taskId) || !validation().ok()) {
        return {};
    }
    QHash<TaskId, QList<TaskId>> successors;
    for (const TaskDependency &dependency : m_dependencies) {
        successors[dependency.predecessorId].append(dependency.successorId);
    }
    return directedClosure(taskId, successors);
}

QList<TaskId> TaskDependencyGraph::connectedTaskIds(
    const QList<TaskId> &seedTaskIds) const
{
    // 类别图需要连接上下文，因此这里临时忽略边方向，把关系视为无向邻接。
    QHash<TaskId, QList<TaskId>> neighborsByTask;
    for (const TaskDependency &dependency : m_dependencies) {
        if (!containsTask(dependency.predecessorId)
            || !containsTask(dependency.successorId)) {
            continue;
        }
        neighborsByTask[dependency.predecessorId].append(dependency.successorId);
        neighborsByTask[dependency.successorId].append(dependency.predecessorId);
    }
    for (auto iterator = neighborsByTask.begin(); iterator != neighborsByTask.end();
         ++iterator) {
        normalizeIds(iterator.value());
    }

    QList<TaskId> pendingTaskIds;
    for (const TaskId &taskId : seedTaskIds) {
        if (containsTask(taskId)) {
            pendingTaskIds.append(taskId);
        }
    }
    normalizeIds(pendingTaskIds);

    // 从所有合法种子同时进行广度遍历，最终仍按稳定 ID 返回。
    QSet<TaskId> visitedTaskIds;
    qsizetype nextPendingIndex = 0;
    while (nextPendingIndex < pendingTaskIds.size()) {
        const TaskId taskId = pendingTaskIds.at(nextPendingIndex++);
        if (visitedTaskIds.contains(taskId)) {
            continue;
        }
        visitedTaskIds.insert(taskId);
        for (const TaskId &neighborId : neighborsByTask.value(taskId)) {
            if (!visitedTaskIds.contains(neighborId)) {
                pendingTaskIds.append(neighborId);
            }
        }
    }

    QList<TaskId> connectedIds = visitedTaskIds.values();
    normalizeIds(connectedIds);
    return connectedIds;
}

TaskDependencyResolution TaskDependencyGraph::dependencyResolution(
    const Task &task) noexcept
{
    if (task.status() == TaskStatus::Done) {
        return TaskDependencyResolution::Satisfied;
    }
    if (task.status() == TaskStatus::Cancelled) {
        return TaskDependencyResolution::Cancelled;
    }
    if (task.status() == TaskStatus::Archived) {
        // 归档不删除关系；解析语义继续沿用归档前的 Done/Cancelled 终态。
        if (task.statusBeforeArchive()
            == std::optional<TaskStatus>{TaskStatus::Done}) {
            return TaskDependencyResolution::Satisfied;
        }
        if (task.statusBeforeArchive()
            == std::optional<TaskStatus>{TaskStatus::Cancelled}) {
            return TaskDependencyResolution::Cancelled;
        }
    }
    return TaskDependencyResolution::Pending;
}

bool TaskDependencyGraph::satisfiesDependency(const Task &task) noexcept
{
    return dependencyResolution(task) == TaskDependencyResolution::Satisfied;
}

const Task *TaskDependencyGraph::findTask(const TaskId &taskId) const
{
    // 指针只引用 m_tasks 内部元素，有效期不超过当前图快照。
    const auto iterator = m_taskIndexes.constFind(taskId);
    return iterator == m_taskIndexes.cend()
        ? nullptr
        : &m_tasks.at(iterator.value());
}

} // namespace smartmate::model
