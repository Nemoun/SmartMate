#pragma once

#include "domain/Task.h"
#include "repositories/RepositoryException.h"

#include <QList>

#include <optional>

namespace smartmate::model {

/// 单个批量状态写入项；expectedStatus 用于在事务中防御预检后的并发状态变化。
struct TaskStateChange final {
    /// 待更新任务的稳定身份。
    TaskId taskId;
    /// Service 预检时观察到的状态；SQL 条件不匹配表示发生并发变化。
    TaskStatus expectedStatus{TaskStatus::Todo};
    /// 由 Model 状态机计算出的目标状态，不由 Persistence 自行推导。
    TaskStatus targetStatus{TaskStatus::Todo};
    /// 归档时记录恢复点；非归档状态必须为空。
    std::optional<TaskStatus> statusBeforeArchive;
    /// 本批命令共用的 UTC 更新时间。
    QDateTime updatedAtUtc;
};

/// 批量状态端口的原子写入结果；存在冲突时实现必须回滚并返回 updatedTaskCount=0。
struct TaskBatchWriteResult final {
    /// 成功事务实际更新的任务数；存在任一冲突时必须为 0。
    int updatedTaskCount{0};
    /// 条件更新未命中的稳定 ID；Service 据此生成结构化命令错误。
    QList<TaskId> conflictingTaskIds;
};

/// 批量状态转换命令端口；全部条件更新必须位于同一事务中，禁止产生部分成功。
/// 端口只返回写入事实，不发送 tasksChanged()；Service 确认结果完整后统一通知绑定方。
class ITaskBatchTransitionRepository {
public:
    virtual ~ITaskBatchTransitionRepository() = default;

    /// 按稳定 TaskId 顺序执行条件更新；持久化故障抛出 RepositoryException。
    [[nodiscard]] virtual TaskBatchWriteResult updateTaskStatesAtomically(
        const QList<TaskStateChange> &changes) = 0;
};

} // namespace smartmate::model
