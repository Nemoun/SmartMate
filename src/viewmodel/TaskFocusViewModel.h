#pragma once

#include "TaskPlanProjection.h"
#include "domain/TaskCategory.h"
#include "viewmodel/contracts/TaskFocusContract.h"

#include <QTimer>

namespace smartmate::model {
class TaskService;
class TaskCategoryService;
}

namespace smartmate::viewmodel {

/// 独立观察 TaskService 的焦点任务投影，不读取列表筛选或其他 ViewModel。
class TaskFocusViewModel final : public TaskFocusContract {
    Q_OBJECT
public:
    explicit TaskFocusViewModel(model::TaskService &taskService,
                                QObject *parent = nullptr);
    TaskFocusViewModel(model::TaskService &taskService,
                       model::TaskCategoryService &categoryService,
                       QObject *parent = nullptr);

    [[nodiscard]] FocusState focusState() const noexcept override;
    [[nodiscard]] QString focusTaskId() const override;
    [[nodiscard]] QString focusTitle() const override;
    [[nodiscard]] QString focusDescription() const override;
    [[nodiscard]] QString focusStatusText() const override;
    [[nodiscard]] QString focusPriorityText() const override;
    [[nodiscard]] QString focusDeadlineText() const override;
    [[nodiscard]] int focusEstimatedMinutes() const noexcept override;
    [[nodiscard]] QString focusReasonText() const override;
    [[nodiscard]] bool focusOverdue() const noexcept override;
    [[nodiscard]] bool focusCanStart() const noexcept override;
    [[nodiscard]] bool focusCanComplete() const noexcept override;
    [[nodiscard]] QString focusCategoryName() const override;
    [[nodiscard]] QString focusCategoryAccent() const override;
    [[nodiscard]] bool focusHasCategory() const noexcept override;

private:
    TaskFocusViewModel(model::TaskService &taskService,
                       model::TaskCategoryService *categoryService,
                       QObject *parent);
    void reload();
    void reloadCategories();
    [[nodiscard]] const model::Task *focusTask() const;
    [[nodiscard]] const model::TaskCategory *focusCategory() const;

    model::TaskService &m_taskService;
    model::TaskCategoryService *m_categoryService{nullptr};
    QTimer m_reloadTimer;
    TaskPlanProjection m_projection;
    QList<model::TaskCategory> m_categories;
    model::TaskId m_focusTaskId;
    FocusState m_focusState{FocusState::NoTasks};
};

} // namespace smartmate::viewmodel
