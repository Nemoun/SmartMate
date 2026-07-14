#pragma once

#include "domain/Task.h"
#include "domain/TaskCommandAvailability.h"
#include "planner/TaskOrderingPolicy.h"

#include <QHash>

namespace smartmate::viewmodel {

/// Model 计划快照的展示级依赖信息，不重新计算任何依赖资格。
struct TaskDependencyProjection {
    bool blocked{false};
    QString blockingReasonText;
    int predecessorCount{0};
    int unlockCount{0};

    bool operator==(const TaskDependencyProjection &) const = default;
};

/// 列表、焦点和详情共享的只读展示映射结果。
struct TaskPlanProjection {
    QList<model::Task> tasks;
    QHash<model::TaskId, QString> orderReasonTexts;
    QHash<model::TaskId, bool> overdueStates;
    QHash<model::TaskId, TaskDependencyProjection> dependencyProjections;
    QHash<model::TaskId, model::TaskCommandAvailability> availabilities;

    [[nodiscard]] const model::Task *taskForId(const model::TaskId &taskId) const;
    [[nodiscard]] const model::TaskCommandAvailability &availabilityFor(
        const model::TaskId &taskId) const;

    bool operator==(const TaskPlanProjection &) const = default;
};

/// 将 Model 已计算的计划语义一次映射为中文展示文案。
[[nodiscard]] TaskPlanProjection makeTaskPlanProjection(
    const QList<model::PlannedTask> &plannedTasks);

} // namespace smartmate::viewmodel
