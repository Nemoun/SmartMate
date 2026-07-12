#pragma once

#include "domain/TaskTypes.h"
#include "repositories/RepositoryException.h"

namespace smartmate::model {

/// 永久删除命令的原子写入结果；依赖数量用于决定是否发布依赖变更通知。
struct TaskDeletionWriteResult final {
    bool taskDeleted{false};
    int removedDependencyCount{0};
};

/// 归档任务永久删除端口；实现必须在同一事务内清理任务及其全部依赖边。
class ITaskDeletionRepository {
public:
    virtual ~ITaskDeletionRepository() = default;

    /// 仅删除归档任务。目标缺失或已非归档时返回 taskDeleted=false 且不得保留部分修改；
    /// 持久化故障抛出 RepositoryException。
    [[nodiscard]] virtual TaskDeletionWriteResult
    deleteArchivedTaskWithDependencies(const TaskId &taskId) = 0;
};

} // namespace smartmate::model
