#pragma once

#include "domain/Task.h"
#include "repositories/RepositoryException.h"

#include <QList>

#include <optional>

namespace smartmate::model {

/// 常规任务持久化端口；活动任务的删除采用归档语义，因此这里不提供物理 remove。
/// 归档实体的永久删除由独立 ITaskDeletionRepository 承担跨表事务。
/// 读写故障必须抛出 RepositoryException，Service 会将其映射为结构化错误。
/// 本端口没有 QObject/信号和数据绑定职责；ViewModel 只能经 Service 获得领域快照。
class ITaskRepository {
public:
    virtual ~ITaskRepository() = default;

    /// 读取全部任务；数据库顺序只要求确定性，业务推荐顺序由 Model Planner 计算。
    [[nodiscard]] virtual QList<Task> findAll() const = 0;
    /// 找不到任务返回空值，持久化故障仍抛出异常。
    [[nodiscard]] virtual std::optional<Task> findById(const TaskId &id) const = 0;
    /// 插入已由 Service 校验的完整任务快照；成功返回后由 Service 决定通知。
    virtual void insert(const Task &task) = 0;
    /// ID 不存在返回 false；实际写入故障抛出异常。
    [[nodiscard]] virtual bool update(const Task &task) = 0;
};

} // namespace smartmate::model
