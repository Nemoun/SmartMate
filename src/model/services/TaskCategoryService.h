#pragma once

#include "repositories/ITaskCategoryRepository.h"
#include "services/TaskCategoryResult.h"

#include <QObject>

namespace smartmate::model {

/// 类别名称、颜色及生命周期规则的唯一 Model 入口；不拥有注入的 Repository。
class TaskCategoryService final : public QObject {
    Q_OBJECT

public:
    /// 注入类别 Repository 端口；QObject 父对象仅管理 Service 自身生命周期。
    explicit TaskCategoryService(ITaskCategoryRepository &repository,
                                 QObject *parent = nullptr);

    /// 返回按规范化名称和稳定 ID 排序的全部类别快照。
    [[nodiscard]] TaskCategoryListResult listCategories() const;
    /// 校验名称与颜色，生成稳定 ID 后创建类别。
    [[nodiscard]] TaskCategoryResult createCategory(
        const TaskCategoryDraft &draft);
    /// 按稳定 ID 更新类别；同值保存视为成功但不会发送变化通知。
    [[nodiscard]] TaskCategoryResult updateCategory(
        const TaskCategoryId &id,
        const TaskCategoryDraft &draft);
    /// 原子删除类别并将关联任务改为未分类，不影响任务依赖边。
    [[nodiscard]] TaskCategoryDeletionResult deleteCategory(
        const TaskCategoryId &id);

signals:
    /// 类别目录实际创建、修改或删除后发送；无变化保存不会通知。
    void categoriesChanged();
    /// 删除类别实际解除任务归属时发送，任务投影和依赖图据此刷新。
    void taskCategoryAssignmentsChanged();

private:
    /// 非拥有端口引用；由组合根创建，其生命周期必须长于本 Service。
    ITaskCategoryRepository &m_repository;
};

} // namespace smartmate::model
