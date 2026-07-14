#include "planner/TaskOrderingPolicy.h"

#include "dependencies/TaskDependencyGraph.h"
#include "planner/TaskDeadlinePolicy.h"

#include <QHash>

#include <algorithm>
#include <utility>

namespace smartmate::model {
namespace {

[[nodiscard]] int statusGroup(const TaskStatus status) noexcept
{
    // 数字越小越靠前：进行中、待办、终态、归档。
    switch (status) {
    case TaskStatus::InProgress:
        return 0;
    case TaskStatus::Todo:
        return 1;
    case TaskStatus::Done:
    case TaskStatus::Cancelled:
        return 2;
    case TaskStatus::Archived:
        return 3;
    }
    return 4;
}

[[nodiscard]] int priorityRank(const TaskPriority priority) noexcept
{
    // 排序排名与枚举数值解耦，避免依赖展示索引的偶然顺序。
    switch (priority) {
    case TaskPriority::Urgent:
        return 0;
    case TaskPriority::High:
        return 1;
    case TaskPriority::Normal:
        return 2;
    case TaskPriority::Low:
        return 3;
    }
    return 4;
}

[[nodiscard]] QString stableId(const Task &task)
{
    // 所有业务字段相同时用稳定 ID 收尾，保证排序是确定且全序的。
    return task.id().toString(QUuid::WithoutBraces);
}

[[nodiscard]] bool todoComesBefore(const Task &left,
                                   const Task &right,
                                   const QDateTime &nowUtc)
{
    // Todo 依次比较：逾期、优先级、是否有截止时间、截止时间、创建时间、稳定 ID。
    const bool leftOverdue = TaskDeadlinePolicy::isOverdue(left, nowUtc);
    const bool rightOverdue = TaskDeadlinePolicy::isOverdue(right, nowUtc);
    if (leftOverdue != rightOverdue) {
        return leftOverdue;
    }

    const int leftPriority = priorityRank(left.priority());
    const int rightPriority = priorityRank(right.priority());
    if (leftPriority != rightPriority) {
        return leftPriority < rightPriority;
    }

    const bool leftHasDeadline = left.deadline().has_value();
    const bool rightHasDeadline = right.deadline().has_value();
    if (leftHasDeadline != rightHasDeadline) {
        return leftHasDeadline;
    }
    if (leftHasDeadline && *left.deadline() != *right.deadline()) {
        return *left.deadline() < *right.deadline();
    }

    if (left.createdAtUtc() != right.createdAtUtc()) {
        return left.createdAtUtc() < right.createdAtUtc();
    }
    return stableId(left) < stableId(right);
}

[[nodiscard]] bool recentlyUpdatedComesBefore(const Task &left,
                                              const Task &right)
{
    // 已完成、已取消和已归档按最近变化优先，便于用户看到刚处理的结果。
    if (left.updatedAtUtc() != right.updatedAtUtc()) {
        return left.updatedAtUtc() > right.updatedAtUtc();
    }
    return stableId(left) < stableId(right);
}

[[nodiscard]] bool baseOrderComesBefore(const Task &left,
                                        const Task &right,
                                        const QDateTime &nowUtc)
{
    const int leftGroup = statusGroup(left.status());
    const int rightGroup = statusGroup(right.status());
    if (leftGroup != rightGroup) {
        return leftGroup < rightGroup;
    }
    if (left.status() == TaskStatus::Todo) {
        return todoComesBefore(left, right, nowUtc);
    }
    if (leftGroup >= 2) {
        return recentlyUpdatedComesBefore(left, right);
    }
    return stableId(left) < stableId(right);
}

[[nodiscard]] TaskOrderReason reasonFor(const Task &task,
                                        const QDateTime &nowUtc)
{
    // 理由只表达主导排序语义；blocked 和 overdue 仍作为独立派生字段保留。
    switch (task.status()) {
    case TaskStatus::InProgress:
        return TaskOrderReason::InProgress;
    case TaskStatus::Todo:
        if (TaskDeadlinePolicy::isOverdue(task, nowUtc)) {
            return TaskOrderReason::Overdue;
        }
        if (task.priority() == TaskPriority::Urgent) {
            return TaskOrderReason::UrgentPriority;
        }
        if (task.priority() == TaskPriority::High) {
            return TaskOrderReason::HighPriority;
        }
        if (task.deadline().has_value()) {
            return TaskOrderReason::UpcomingDeadline;
        }
        return TaskOrderReason::Todo;
    case TaskStatus::Done:
        return TaskOrderReason::Completed;
    case TaskStatus::Cancelled:
        return TaskOrderReason::Cancelled;
    case TaskStatus::Archived:
        return TaskOrderReason::Archived;
    }
    return TaskOrderReason::Todo;
}

} // namespace

QList<PlannedTask> orderTasks(const QList<Task> &tasks, const QDateTime &nowUtc)
{
    // 无依赖重载复用统一算法，所有活动任务自然落入 Ready 分组。
    return orderTasks(tasks, {}, nowUtc);
}

QList<PlannedTask> orderTasks(const QList<Task> &tasks,
                              const QList<TaskDependency> &dependencies,
                              const QDateTime &nowUtc)
{
    const TaskDependencyGraph graph{tasks, dependencies};
    // 分组指针只引用输入快照，排序不会复制或修改原始 Task 值。
    QList<const Task *> readyTasks;
    QList<const Task *> blockedTasks;
    QList<const Task *> terminalTasks;
    QList<const Task *> archivedTasks;
    QHash<TaskId, const Task *> blockedById;

    for (const Task &task : tasks) {
        if (task.status() == TaskStatus::Archived) {
            archivedTasks.append(&task);
            continue;
        }
        if (task.status() == TaskStatus::Done
            || task.status() == TaskStatus::Cancelled) {
            terminalTasks.append(&task);
            continue;
        }
        if (graph.dependencyState(task.id()).blocked) {
            blockedTasks.append(&task);
            blockedById.insert(task.id(), &task);
        } else {
            readyTasks.append(&task);
        }
    }

    // Ready、终态和归档分别应用同一套确定性基础比较规则。
    const auto baseComparator = [&nowUtc](const Task *left, const Task *right) {
        return baseOrderComesBefore(*left, *right, nowUtc);
    };
    std::sort(readyTasks.begin(), readyTasks.end(), baseComparator);
    std::sort(terminalTasks.begin(), terminalTasks.end(), baseComparator);
    std::sort(archivedTasks.begin(), archivedTasks.end(), baseComparator);

    // Blocked 分组内部使用 Kahn 排序。跨越 Ready→Blocked 的边无需计入入度，
    // 因为整个 Ready 分组已经固定出现在 Blocked 之前。
    QHash<TaskId, int> blockedIndegrees;
    QHash<TaskId, QList<TaskId>> blockedSuccessors;
    for (const Task *task : blockedTasks) {
        blockedIndegrees.insert(task->id(), 0);
    }
    for (const TaskDependency &dependency : dependencies) {
        if (blockedById.contains(dependency.predecessorId)
            && blockedById.contains(dependency.successorId)) {
            ++blockedIndegrees[dependency.successorId];
            blockedSuccessors[dependency.predecessorId].append(dependency.successorId);
        }
    }

    // 当前入度为零的 Blocked 节点可以进入稳定候选队列。
    QList<const Task *> availableBlocked;
    for (const Task *task : blockedTasks) {
        if (blockedIndegrees.value(task->id()) == 0) {
            availableBlocked.append(task);
        }
    }

    QList<const Task *> topologicalBlocked;
    while (!availableBlocked.isEmpty()) {
        std::sort(availableBlocked.begin(), availableBlocked.end(), baseComparator);
        const Task *task = availableBlocked.takeFirst();
        topologicalBlocked.append(task);
        for (const TaskId &successorId : blockedSuccessors.value(task->id())) {
            const int newIndegree = blockedIndegrees.value(successorId) - 1;
            blockedIndegrees.insert(successorId, newIndegree);
            if (newIndegree == 0) {
                availableBlocked.append(blockedById.value(successorId));
            }
        }
    }

    // Service 会拒绝持久化环；若直接调用策略时输入了坏图，仍稳定保留全部任务。
    if (topologicalBlocked.size() != blockedTasks.size()) {
        QList<const Task *> cyclicRemainder;
        for (const Task *task : blockedTasks) {
            if (!topologicalBlocked.contains(task)) {
                cyclicRemainder.append(task);
            }
        }
        std::sort(cyclicRemainder.begin(), cyclicRemainder.end(), baseComparator);
        topologicalBlocked.append(cyclicRemainder);
    }

    // 最终大组顺序固定：可执行、拓扑阻塞、完成/取消、归档。
    QList<const Task *> orderedTasks;
    orderedTasks.reserve(tasks.size());
    orderedTasks.append(readyTasks);
    orderedTasks.append(topologicalBlocked);
    orderedTasks.append(terminalTasks);
    orderedTasks.append(archivedTasks);

    // 规划策略填入顺序、理由和依赖状态；Service 随后补充命令资格。
    QList<PlannedTask> plan;
    plan.reserve(orderedTasks.size());
    for (const Task *task : orderedTasks) {
        plan.append(PlannedTask{*task,
                                reasonFor(*task, nowUtc),
                                TaskDeadlinePolicy::isOverdue(*task, nowUtc),
                                graph.dependencyState(task->id()),
                                {}});
    }
    return plan;
}

} // namespace smartmate::model
