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
    void reload();
    void reloadCategories();
    [[nodiscard]] const model::Task *selectedTask() const;
    [[nodiscard]] const model::TaskCategory *selectedCategory() const;

    model::TaskService &m_taskService;
    model::TaskCategoryService *m_categoryService{nullptr};
    TaskPlanProjection m_projection;
    QList<model::TaskCategory> m_categories;
    model::TaskId m_selectedTaskId;
};

} // namespace smartmate::viewmodel
