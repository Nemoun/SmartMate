#pragma once

#include "domain/TaskCategory.h"

#include <QList>

#include <optional>

namespace smartmate::model {

/// 删除类别的原子写入结果；任务只解除归属，不删除任务或依赖边。
struct CategoryDeletionWriteResult final {
    /// 类别行是否在事务内成功删除。
    bool categoryDeleted{false};
    /// 同一事务中被置为未分类的任务数，用于 Service 决定通知范围。
    int unassignedTaskCount{0};
};

/// 类别持久化端口；方法名显式携带 Category，以便 SQLite Adapter 同时实现多个窄端口。
/// 查询快照由 Service 交给 ViewModel 投影；写方法本身不发送 Contract 或 Model 通知。
class ITaskCategoryRepository {
public:
    virtual ~ITaskCategoryRepository() = default;

    /// 读取全部类别快照；持久化故障抛出 RepositoryException。
    [[nodiscard]] virtual QList<TaskCategory> findAllCategories() const = 0;
    /// 按稳定 ID 查询；不存在返回空值，读取故障抛出 RepositoryException。
    [[nodiscard]] virtual std::optional<TaskCategory> findCategoryById(
        const TaskCategoryId &id) const = 0;
    /// 插入已由 Service 校验并分配稳定 ID 的类别快照。
    virtual void insertCategory(const TaskCategory &category) = 0;
    /// 覆盖指定类别；ID 不存在返回 false，写入故障抛出 RepositoryException。
    [[nodiscard]] virtual bool updateCategory(const TaskCategory &category) = 0;

    /// 同一事务内解除全部任务归属并删除类别；不得改动任务状态或依赖边。
    [[nodiscard]] virtual CategoryDeletionWriteResult
    deleteCategoryAndUnassignTasks(const TaskCategoryId &id,
                                   const QDateTime &updatedAtUtc) = 0;
};

} // namespace smartmate::model
