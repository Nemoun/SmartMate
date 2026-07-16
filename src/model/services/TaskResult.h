#pragma once

#include "domain/Task.h"
#include "domain/TaskDependencyEditContext.h"
#include "domain/TaskDependency.h"
#include "domain/TaskGraph.h"
#include "planner/TaskOrderingPolicy.h"

#include <QList>
#include <QString>

#include <optional>
#include <utility>

namespace smartmate::model {

/// Model 与 ViewModel 之间传递的稳定错误类别；detail 仅用于技术诊断，不是界面文案。
enum class TaskError {
    /// 操作成功。
    None,
    /// 标题规范化后为空。
    EmptyTitle,
    /// 标题超过领域长度上限。
    TitleTooLong,
    /// 描述超过领域长度上限。
    DescriptionTooLong,
    /// 截止时间对象无效。
    InvalidDeadline,
    /// 预计用时超出分钟数边界。
    InvalidEstimate,
    /// 优先级不是已定义枚举值。
    InvalidPriority,
    /// 持久化快照包含未知状态。
    InvalidStatus,
    /// 当前状态不允许请求的状态转换。
    InvalidTaskTransition,
    /// 依赖关系的某个端点不存在。
    DependencyEndpointNotFound,
    /// 任务被设置为自己的前置。
    DependencySelfReference,
    /// 同一依赖边重复出现。
    DependencyDuplicate,
    /// 依赖图产生有向环。
    DependencyCycle,
    /// 目标任务当前不允许编辑前置集合。
    DependencyTargetNotEditable,
    /// 新增前置任务处于归档或取消状态。
    DependencyPredecessorNotEligible,
    /// 当前任务的直接前置尚未全部解析。
    TaskBlocked,
    /// 命令会使其他受保护任务违反依赖状态约束。
    DependencyStateConflict,
    /// 当前状态不允许编辑任务详情。
    TaskDetailsNotEditable,
    /// 当前任务不是可永久删除的归档状态。
    TaskDeletionNotAllowed,
    /// 草稿或图查询引用的稳定类别 ID 不存在。
    TaskCategoryNotFound,
    /// 批量命令没有有效稳定 ID。
    EmptyTaskSelection,
    /// 指定稳定 TaskId 不存在。
    NotFound,
    /// 已有其他任务进行中，或并发写入造成冲突。
    InProgressConflict,
    /// Repository 读写或事务执行失败。
    PersistenceFailure,
};

/// 业务错误的机器可读关联对象；界面无需解析 detail 文本即可定位冲突任务。
struct TaskErrorContext final {
    /// 尚未满足且直接造成阻塞的前置任务。
    QList<TaskId> blockingTaskIds;
    /// 与当前命令发生状态、端点或重复冲突的任务。
    QList<TaskId> conflictingTaskIds;
    /// 成环时包含首尾相同的完整有向路径。
    QList<TaskId> cyclePath;

    friend bool operator==(const TaskErrorContext &,
                           const TaskErrorContext &) = default;
};

/// 无返回值的纯校验结果，成功时 error 为 None 且 detail 为空。
struct TaskValidationResult final {
    /// 机器可读校验错误；成功时为 None。
    TaskError error{TaskError::None};
    /// 面向日志的补充诊断，不承担稳定界面文案职责。
    QString detail;

    [[nodiscard]] bool ok() const noexcept
    {
        return error == TaskError::None;
    }

    [[nodiscard]] static TaskValidationResult success()
    {
        return {};
    }

    [[nodiscard]] static TaskValidationResult failure(TaskError resultError,
                                                       QString resultDetail = {})
    {
        TaskValidationResult result;
        result.error = resultError;
        result.detail = std::move(resultDetail);
        return result;
    }
};

/// Service 操作结果；成功必须同时具有 value 且 error 为 None。
/// 该结果是 Model→ViewModel 的同步数据边界，不是 Widget 可直接绑定的 Contract。
template<typename T>
struct ServiceResult final {
    /// 成功时的领域结果；失败时为空。
    std::optional<T> value;
    /// 供 ViewModel 映射通知和命令状态的稳定错误。
    TaskError error{TaskError::None};
    /// Repository 或校验失败的技术细节。
    QString detail;
    /// 与失败相关的稳定 TaskId 集合，避免调用方解析 detail。
    TaskErrorContext context;

    [[nodiscard]] bool ok() const noexcept
    {
        return error == TaskError::None && value.has_value();
    }

    [[nodiscard]] static ServiceResult success(T resultValue)
    {
        ServiceResult result;
        result.value.emplace(std::move(resultValue));
        return result;
    }

    [[nodiscard]] static ServiceResult failure(TaskError resultError,
                                               QString resultDetail = {},
                                               TaskErrorContext resultContext = {})
    {
        ServiceResult result;
        result.error = resultError;
        result.detail = std::move(resultDetail);
        result.context = std::move(resultContext);
        return result;
    }
};

using TaskResult = ServiceResult<Task>;
using TaskListResult = ServiceResult<QList<Task>>;
/// 批量命令成功时返回按稳定 TaskId 排序的最终任务快照及删除的依赖数量。
struct TaskBatchOutcome final {
    /// 命令完成后的任务快照，按稳定 TaskId 排序。
    QList<Task> tasks;
    /// 永久删除任务时随事务移除的入边与出边总数。
    int removedDependencyCount{0};
};
using TaskBatchResult = ServiceResult<TaskBatchOutcome>;
using TaskPlanResult = ServiceResult<QList<PlannedTask>>;
using TaskDependencyListResult = ServiceResult<QList<TaskDependency>>;
using TaskGraphResult = ServiceResult<TaskGraphSnapshot>;
using TaskDependencyEditContextResult = ServiceResult<TaskDependencyEditContext>;

} // namespace smartmate::model
