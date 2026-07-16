#pragma once

#include "domain/TaskDependency.h"
#include "repositories/RepositoryException.h"

#include <QList>

namespace smartmate::model {

/// 任务依赖持久化端口；业务校验由 Service 和依赖图负责，端口只保证原子替换。
/// 查询结果用于 ViewModel 的间接投影，端口不暴露 QSqlQuery/QSqlTableModel，也不发通知。
class ITaskDependencyRepository {
public:
    virtual ~ITaskDependencyRepository() = default;

    /// 读取全部 Finish-to-Start 边；返回顺序必须稳定，故障抛出 RepositoryException。
    [[nodiscard]] virtual QList<TaskDependency> findAllDependencies() const = 0;
    /// 原子替换一个后继的全部前置；空列表表示清空，正常返回后由 Service 发依赖通知。
    virtual void replacePredecessors(const TaskId &successorId,
                                     const QList<TaskId> &predecessorIds) = 0;
};

} // namespace smartmate::model
