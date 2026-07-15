#pragma once

#include "TaskPlanProjection.h"
#include "domain/TaskCategory.h"
#include "viewmodel/contracts/TaskDetailsContract.h"

namespace smartmate::model {
class TaskService;
class TaskCategoryService;
}

namespace smartmate::viewmodel {

/// 独立的任务详情选择与展示投影，只保存稳定 TaskId。
/// Service 变化后自行重载，不读取或调用列表 ViewModel。
class TaskDetailsViewModel final : public TaskDetailsContract {
    Q_OBJECT
public:
    explicit TaskDetailsViewModel(model::TaskService &taskService,
                                  QObject *parent = nullptr);
    TaskDetailsViewModel(model::TaskService &taskService,
                         model::TaskCategoryService &categoryService,
                         QObject *parent = nullptr);

    [[nodiscard]] QString selectedTaskId() const override;
    [[nodiscard]] QString selectedTitle() const override;
    [[nodiscard]] QString selectedDescription() const override;
    [[nodiscard]] QString selectedStatusText() const override;
    [[nodiscard]] QString selectedPriorityText() const override;
    [[nodiscard]] QString selectedDeadlineText() const override;
    [[nodiscard]] int selectedEstimatedMinutes() const noexcept override;
    [[nodiscard]] QString selectedCreatedAtText() const override;
    [[nodiscard]] QString selectedUpdatedAtText() const override;
    [[nodiscard]] QString selectedReasonText() const override;
    [[nodiscard]] QString selectedBlockingReasonText() const override;
    [[nodiscard]] int selectedPredecessorCount() const noexcept override;
    [[nodiscard]] int selectedUnlockCount() const noexcept override;
    [[nodiscard]] bool selectedCanEditTask() const noexcept override;
    [[nodiscard]] bool selectedCanEditDependencies() const noexcept override;
    [[nodiscard]] QString selectedCategoryName() const override;
    [[nodiscard]] QString selectedCategoryAccent() const override;
    [[nodiscard]] bool selectedHasCategory() const noexcept override;

    bool selectTask(const QString &taskId) override;
    void clearSelection() override;

private:
    TaskDetailsViewModel(model::TaskService &taskService,
                         model::TaskCategoryService *categoryService,
                         QObject *parent);
    /// 重建共享计划投影；所选任务消失时同时清空选择。
    void reload();
    /// 刷新类别展示快照并通知详情 getter 重读。
    void reloadCategories();
    /// 在当前计划投影中解析稳定选择；返回指针只在投影未替换期间有效。
    [[nodiscard]] const model::Task *selectedTask() const;
    [[nodiscard]] const model::TaskCategory *selectedCategory() const;

    /// 非拥有 Service 引用，组合根保证生命周期。
    model::TaskService &m_taskService;
    /// 非拥有可选类别 Service。
    model::TaskCategoryService *m_categoryService{nullptr};
    /// 列表、焦点和详情共享格式的只读计划展示快照。
    TaskPlanProjection m_projection;
    /// 类别名称和颜色展示快照。
    QList<model::TaskCategory> m_categories;
    /// 当前详情选择；空 ID 表示未选择。
    model::TaskId m_selectedTaskId;
};

} // namespace smartmate::viewmodel
