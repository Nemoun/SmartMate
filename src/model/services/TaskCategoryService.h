#pragma once

#include "repositories/ITaskCategoryRepository.h"
#include "services/TaskCategoryResult.h"

#include <QObject>

namespace smartmate::model {

/// 类别名称、颜色及生命周期规则的唯一 Model 入口；不拥有注入的 Repository。
///
/// create/update/delete 是强类型业务命令；listCategories 是 ViewModel 重建类别投影的
/// 查询入口。变化信号只表示 Model 数据已失效，具体 ViewModel 收到后重新查询并发布
/// Contract 通知，Service 不直接绑定 Widget。
class TaskCategoryService final : public QObject {
    Q_OBJECT

public:
    /// 注入类别 Repository 端口；QObject 父对象仅管理 Service 自身生命周期。
    explicit TaskCategoryService(ITaskCategoryRepository &repository,
                                 QObject *parent = nullptr);

    /// 查询命令：返回按规范化名称和稳定 ID 排序的全部类别快照，不发送变化通知。
    [[nodiscard]] TaskCategoryListResult listCategories() const;
    /// 创建命令：校验名称与颜色并生成稳定 ID；成功写入后发送 categoriesChanged()。
    [[nodiscard]] TaskCategoryResult createCategory(
        const TaskCategoryDraft &draft);
    /// 更新命令：按稳定 ID 保存；实际变化后发送 categoriesChanged()，幂等保存不通知。
    [[nodiscard]] TaskCategoryResult updateCategory(
        const TaskCategoryId &id,
        const TaskCategoryDraft &draft);
    /// 删除命令：原子删除类别并解除任务归属；按实际影响发送两类变化通知。
    [[nodiscard]] TaskCategoryDeletionResult deleteCategory(
        const TaskCategoryId &id);

signals:
    /// 类别目录失效通知：实际创建、修改或删除后发送，订阅 ViewModel 应重新查询类别投影。
    void categoriesChanged();
    /// 任务类别归属失效通知：仅解除至少一个任务归属时发送，任务列表/详情/图据此重投影。
    void taskCategoryAssignmentsChanged();

private:
    /// 非拥有端口引用；由组合根创建，其生命周期必须长于本 Service。
    ITaskCategoryRepository &m_repository;
};

} // namespace smartmate::model
