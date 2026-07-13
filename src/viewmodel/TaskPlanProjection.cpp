#include "TaskPlanProjection.h"

#include <QStringList>

#include <algorithm>

namespace smartmate::viewmodel {
namespace {

const model::TaskCommandAvailability emptyAvailability{};

QString orderReasonText(const model::TaskOrderReason reason)
{
    using enum model::TaskOrderReason;
    switch (reason) {
    case InProgress: return QStringLiteral("正在进行");
    case Overdue: return QStringLiteral("已逾期");
    case UrgentPriority: return QStringLiteral("紧急优先");
    case HighPriority: return QStringLiteral("高优先");
    case UpcomingDeadline: return QStringLiteral("截止较近");
    case Todo: return QStringLiteral("待办");
    case Completed: return QStringLiteral("已完成");
    case Cancelled: return QStringLiteral("已取消");
    case Archived: return QStringLiteral("已归档");
    }
    return {};
}

QString blockingReasonText(const QList<model::TaskId> &blockingIds,
                           const QHash<model::TaskId, QString> &taskTitles,
                           const QHash<QString, int> &titleCounts)
{
    if (blockingIds.isEmpty()) {
        return QStringLiteral("存在尚未完成或取消的前置任务");
    }
    QStringList visibleTitles;
    visibleTitles.reserve(blockingIds.size());
    for (const model::TaskId &id : blockingIds) {
        const QString title = taskTitles.value(id);
        if (title.isEmpty()) {
            visibleTitles.push_back(QStringLiteral("未知任务（%1）")
                .arg(id.toString(QUuid::WithoutBraces).left(8)));
        } else if (titleCounts.value(title) > 1) {
            visibleTitles.push_back(QStringLiteral("“%1”（%2）")
                .arg(title, id.toString(QUuid::WithoutBraces).left(8)));
        } else {
            visibleTitles.push_back(QStringLiteral("“%1”").arg(title));
        }
    }
    return QStringLiteral("等待%1完成或取消")
        .arg(visibleTitles.join(QStringLiteral("、")));
}

} // namespace

const model::Task *TaskPlanProjection::taskForId(
    const model::TaskId &taskId) const
{
    const auto it = std::find_if(tasks.cbegin(), tasks.cend(),
        [&taskId](const model::Task &task) { return task.id() == taskId; });
    return it == tasks.cend() ? nullptr : &*it;
}

const model::TaskCommandAvailability &TaskPlanProjection::availabilityFor(
    const model::TaskId &taskId) const
{
    const auto it = availabilities.constFind(taskId);
    return it == availabilities.cend() ? emptyAvailability : it.value();
}

TaskPlanProjection makeTaskPlanProjection(
    const QList<model::PlannedTask> &plannedTasks)
{
    TaskPlanProjection projection;
    projection.tasks.reserve(plannedTasks.size());
    projection.orderReasonTexts.reserve(plannedTasks.size());
    projection.overdueStates.reserve(plannedTasks.size());
    projection.dependencyProjections.reserve(plannedTasks.size());
    projection.availabilities.reserve(plannedTasks.size());

    QHash<model::TaskId, QString> taskTitles;
    QHash<QString, int> titleCounts;
    for (const model::PlannedTask &plannedTask : plannedTasks) {
        taskTitles.insert(plannedTask.task.id(), plannedTask.task.title());
        ++titleCounts[plannedTask.task.title()];
    }
    for (const model::PlannedTask &plannedTask : plannedTasks) {
        projection.tasks.push_back(plannedTask.task);
        projection.orderReasonTexts.insert(plannedTask.task.id(),
                                            orderReasonText(plannedTask.reason));
        projection.overdueStates.insert(plannedTask.task.id(), plannedTask.overdue);
        const model::TaskDependencyState &state = plannedTask.dependencyState;
        projection.dependencyProjections.insert(
            plannedTask.task.id(),
            TaskDependencyProjection{
                state.blocked,
                state.blocked
                    ? blockingReasonText(state.unsatisfiedPredecessorIds,
                                         taskTitles, titleCounts)
                    : QString{},
                static_cast<int>(state.predecessorIds.size()),
                state.unlockCount,
            });
        projection.availabilities.insert(plannedTask.task.id(),
                                         plannedTask.availability);
    }
    return projection;
}

} // namespace smartmate::viewmodel
