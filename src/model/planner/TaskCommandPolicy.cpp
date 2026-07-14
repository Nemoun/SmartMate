#include "planner/TaskCommandPolicy.h"

#include "dependencies/TaskDependencyGraph.h"
#include "domain/TaskStateMachine.h"

#include <algorithm>

namespace smartmate::model {
namespace {

[[nodiscard]] Task withStatus(const Task &source, const TaskStatus status)
{
    // 构造假想状态快照，只替换状态且不改写用户详情或时间。
    return Task{source.id(),
                source.title(),
                source.description(),
                source.priority(),
                status,
                std::nullopt,
                source.deadline(),
                source.estimatedMinutes(),
                source.createdAtUtc(),
                source.updatedAtUtc(),
                source.categoryId()};
}

[[nodiscard]] TaskStatus effectiveStatus(const Task &task) noexcept
{
    // 校验归档任务对依赖的保护语义时，使用其归档前状态。
    return task.status() == TaskStatus::Archived
        ? task.statusBeforeArchive().value_or(TaskStatus::Todo)
        : task.status();
}

[[nodiscard]] bool protectedStatesAreConsistent(
    const QList<Task> &tasks,
    const QList<TaskDependency> &dependencies)
{
    const TaskDependencyGraph graph{tasks, dependencies};
    if (!graph.validation().ok()) {
        // 坏图不能证明危险转换安全，因此该保护性检查保守返回 false。
        return false;
    }
    // 已开始或已完成的任务不得存在仍为 Pending 的直接前置。
    return std::none_of(tasks.cbegin(), tasks.cend(), [&graph](const Task &task) {
        const TaskStatus status = effectiveStatus(task);
        return (status == TaskStatus::InProgress || status == TaskStatus::Done)
            && !graph.unsatisfiedPredecessorIds(task.id()).isEmpty();
    });
}

[[nodiscard]] bool transitionPreservesDependencies(
    const QList<Task> &tasks,
    const QList<TaskDependency> &dependencies,
    const Task &task,
    const TaskTransition transition)
{
    // 在副本上应用目标状态，再检查整张图，绝不修改调用方持有的领域快照。
    const auto target = TaskStateMachine::targetStatus(task, transition);
    if (!target.has_value()) {
        return false;
    }
    QList<Task> hypothetical = tasks;
    const auto iterator = std::find_if(
        hypothetical.begin(), hypothetical.end(), [&task](const Task &candidate) {
            return candidate.id() == task.id();
        });
    if (iterator == hypothetical.end()) {
        return false;
    }
    *iterator = withStatus(task, *target);
    return protectedStatesAreConsistent(hypothetical, dependencies);
}

} // namespace

QHash<TaskId, TaskCommandAvailability> taskCommandAvailabilities(
    const QList<Task> &tasks,
    const QList<TaskDependency> &dependencies)
{
    const TaskDependencyGraph graph{tasks, dependencies};
    // 单进行中约束是全局资格，不能只查看当前任务自身状态。
    const bool hasInProgress = std::any_of(
        tasks.cbegin(), tasks.cend(), [](const Task &task) {
            return task.status() == TaskStatus::InProgress;
        });

    QHash<TaskId, TaskCommandAvailability> result;
    result.reserve(tasks.size());
    for (const Task &task : tasks) {
        // 每个资格均来自领域实体、状态机或完整依赖快照，ViewModel 不再二次推导。
        const TaskDependencyState dependencyState = graph.dependencyState(task.id());
        TaskCommandAvailability availability;
        availability.canEditTask = task.canEditDetails();
        availability.canEditDependencies = task.status() == TaskStatus::Todo;
        availability.canStart = TaskStateMachine::canApply(task, TaskTransition::Start)
            && !dependencyState.blocked && !hasInProgress;
        availability.canCancel = TaskStateMachine::canApply(
            task, TaskTransition::Cancel);
        availability.canComplete = TaskStateMachine::canApply(
            task, TaskTransition::Complete) && !dependencyState.blocked;
        availability.canRedo = transitionPreservesDependencies(
            tasks, dependencies, task, TaskTransition::Redo);
        availability.canArchive = TaskStateMachine::canApply(
            task, TaskTransition::Archive);
        availability.canRestore = transitionPreservesDependencies(
            tasks, dependencies, task, TaskTransition::Restore);
        availability.canDeletePermanently = task.canDeletePermanently();
        result.insert(task.id(), availability);
    }
    return result;
}

} // namespace smartmate::model
