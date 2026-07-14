#pragma once

#include "domain/Task.h"
#include "domain/TaskCommandAvailability.h"
#include "domain/TaskDependency.h"

#include <QDateTime>
#include <QList>

namespace smartmate::model {

/// 解释任务出现在推荐位置的主导语义；ViewModel 仅负责将其映射为展示文案。
enum class TaskOrderReason {
    /// 当前正在执行，优先保持焦点。
    InProgress,
    /// 活动任务已超过截止时间。
    Overdue,
    /// 待办任务具有紧急优先级。
    UrgentPriority,
    /// 待办任务具有高优先级。
    HighPriority,
    /// 待办任务设置了尚未逾期的截止时间。
    UpcomingDeadline,
    /// 普通待办任务，没有更高主导理由。
    Todo,
    /// 已完成任务。
    Completed,
    /// 已取消任务。
    Cancelled,
    /// 已归档任务。
    Archived,
};

/// 推荐顺序中的领域快照及其理由，不保存列表行号或持久化排名。
struct PlannedTask final {
    /// 排序时使用的不可变任务快照。
    Task task;
    /// 当前排序位置的主导领域语义，不是界面文案。
    TaskOrderReason reason;
    /// 当前时刻下的逾期派生标记，与主导排序理由相互独立。
    bool overdue{false};
    /// 同一快照下计算的阻塞、前置和解锁信息。
    TaskDependencyState dependencyState;
    /// Service 可在返回计划前填入的完整命令资格。
    TaskCommandAvailability availability;

    friend bool operator==(const PlannedTask &, const PlannedTask &) = default;
};

/// 按任务状态、逾期、优先级和时间生成稳定推荐顺序。
///
/// `nowUtc` 由调用方注入，使“逾期”判断可以确定性测试；排序不会修改输入任务，
/// 也不会把随时间变化的派生排名写入 Repository。
[[nodiscard]] QList<PlannedTask> orderTasks(const QList<Task> &tasks,
                                            const QDateTime &nowUtc);

/// 先排列当前 Ready 任务，再以拓扑顺序排列 Blocked 任务；同一候选集合继续复用
/// 逾期、优先级、截止时间和稳定 ID 规则。
[[nodiscard]] QList<PlannedTask> orderTasks(
    const QList<Task> &tasks,
    const QList<TaskDependency> &dependencies,
    const QDateTime &nowUtc);

} // namespace smartmate::model
