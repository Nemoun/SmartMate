#pragma once

#include "domain/Task.h"
#include "domain/TaskCommandAvailability.h"
#include "planner/TaskOrderingPolicy.h"

#include <QHash>

namespace smartmate::viewmodel {

/// Model 计划快照的展示级依赖信息，不重新计算任何依赖资格。
struct TaskDependencyProjection {
    /// 是否存在尚未解析的直接前置。
    bool blocked{false};
    /// 使用任务标题生成的中文阻塞说明。
    QString blockingReasonText;
    /// 直接前置数量。
    int predecessorCount{0};
    /// 当前任务解析后可直接解锁的后继数量。
    int unlockCount{0};

    bool operator==(const TaskDependencyProjection &) const = default;
};

/// 列表、焦点和详情共享的只读展示映射结果。
struct TaskPlanProjection {
    /// 保留 Model 推荐顺序的任务快照。
    QList<model::Task> tasks;
    /// 按稳定 TaskId 索引的推荐原因文案。
    QHash<model::TaskId, QString> orderReasonTexts;
    /// Model 计算的逾期状态展示映射。
    QHash<model::TaskId, bool> overdueStates;
    /// 依赖状态的中文展示映射。
    QHash<model::TaskId, TaskDependencyProjection> dependencyProjections;
    /// Model 权威命令资格；Widget 和 ViewModel 不重新推导。
    QHash<model::TaskId, model::TaskCommandAvailability> availabilities;

    /// 按稳定 ID 查找当前快照；返回指针仅在 projection 未修改期间有效。
    [[nodiscard]] const model::Task *taskForId(const model::TaskId &taskId) const;
    /// 返回命令资格；未知 ID 使用全 false 的安全空资格。
    [[nodiscard]] const model::TaskCommandAvailability &availabilityFor(
        const model::TaskId &taskId) const;

    bool operator==(const TaskPlanProjection &) const = default;
};

/// 将 Model 已计算的计划语义一次映射为中文展示文案；不改变顺序或业务资格。
[[nodiscard]] TaskPlanProjection makeTaskPlanProjection(
    const QList<model::PlannedTask> &plannedTasks);

} // namespace smartmate::viewmodel
