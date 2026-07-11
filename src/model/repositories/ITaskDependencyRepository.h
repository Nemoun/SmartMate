#pragma once

#include "domain/TaskDependency.h"
#include "repositories/RepositoryException.h"

#include <QList>

namespace smartmate::model {

/// 任务依赖持久化端口；业务校验由 Service 和依赖图负责，端口只保证原子替换。
class ITaskDependencyRepository {
public:
    virtual ~ITaskDependencyRepository() = default;

    [[nodiscard]] virtual QList<TaskDependency> findAllDependencies() const = 0;
    /// 原子替换一个后继的全部前置；空列表表示清空，失败抛出 RepositoryException。
    virtual void replacePredecessors(const TaskId &successorId,
                                     const QList<TaskId> &predecessorIds) = 0;
};

} // namespace smartmate::model
