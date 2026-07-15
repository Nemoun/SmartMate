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
/// Model 或时间变化后重新选择唯一进行中任务/首个可执行推荐任务，并统一通知焦点绑定。
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
    /// 重新查询计划并决定 FocusState 与稳定焦点 TaskId。
    void reload();
    /// 类别目录变化时刷新焦点类别展示。
    void reloadCategories();
    [[nodiscard]] const model::Task *focusTask() const;
    [[nodiscard]] const model::TaskCategory *focusCategory() const;

    /// 非拥有 Service 引用。
    model::TaskService &m_taskService;
    /// 非拥有可选类别 Service。
    model::TaskCategoryService *m_categoryService{nullptr};
    /// 每分钟重算逾期与推荐焦点；不持久化定时状态。
    QTimer m_reloadTimer;
    /// 当前计划展示快照。
    TaskPlanProjection m_projection;
    /// 类别展示快照。
    QList<model::TaskCategory> m_categories;
    /// 当前焦点稳定身份；NoTasks/AllBlocked 时可为空。
    model::TaskId m_focusTaskId;
    FocusState m_focusState{FocusState::NoTasks};
};

} // namespace smartmate::viewmodel
