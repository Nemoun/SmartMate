#pragma once

#include "TaskDependencyViewModel.h"
#include "AppearanceSettingsViewModel.h"
#include "TaskEditorViewModel.h"
#include "TaskCategoryViewModel.h"
#include "TaskGraphViewModel.h"
#include "TaskListViewModel.h"
#include "TaskFocusViewModel.h"
#include "TaskDetailsViewModel.h"

#include <QObject>

namespace smartmate::model {
class TaskService;
class AppearanceSettingsService;
class TaskCategoryService;
}

namespace smartmate::viewmodel {

/// 应用级 ViewModel 只负责组合子 ViewModel；它不保存界面控件，
/// 也不把具体 Repository 或数据库实现泄漏给 View。
/// 子 ViewModel 通过共享 Service 通知独立刷新，彼此不直接调用。
class AppViewModel : public QObject {
public:
    /// 下列构造重载用于按功能注入 Service；正式组合根使用完整依赖版本。
    explicit AppViewModel(model::TaskService &taskService, QObject *parent = nullptr);
    AppViewModel(model::TaskService &taskService,
                 model::AppearanceSettingsService &appearanceService,
                 QObject *parent = nullptr);
    AppViewModel(model::TaskService &taskService,
                 model::TaskCategoryService &categoryService,
                 QObject *parent = nullptr);
    AppViewModel(model::TaskService &taskService,
                 model::TaskCategoryService &categoryService,
                 model::AppearanceSettingsService &appearanceService,
                 QObject *parent = nullptr);

    // 返回稳定子对象地址，供组合根以对应 Contract 引用注入 Widget；调用方不拥有指针。
    [[nodiscard]] TaskListViewModel *taskList() noexcept;
    [[nodiscard]] TaskFocusViewModel *taskFocus() noexcept;
    [[nodiscard]] TaskDetailsViewModel *taskDetails() noexcept;
    [[nodiscard]] TaskEditorViewModel *taskEditor() noexcept;
    [[nodiscard]] TaskDependencyViewModel *taskDependencies() noexcept;
    [[nodiscard]] TaskGraphViewModel *taskGraph() noexcept;
    [[nodiscard]] TaskCategoryViewModel *taskCategories() noexcept;
    [[nodiscard]] AppearanceSettingsViewModel *appearanceSettings() noexcept;

private:
    // 子 ViewModel 共享任务与类别 Service 的状态通知，但彼此不直接调用，
    // 从而让列表、编辑器、类别管理和依赖图保持同步而不形成隐式耦合。
    /// 值成员与 AppViewModel 同生命周期，地址在运行期间稳定。
    TaskCategoryViewModel m_taskCategories;
    TaskListViewModel m_taskList;
    TaskFocusViewModel m_taskFocus;
    TaskDetailsViewModel m_taskDetails;
    TaskEditorViewModel m_taskEditor;
    /// QObject 子对象；使用指针是因为其构造路径需要可选依赖，所有权仍归 this。
    TaskDependencyViewModel *m_taskDependencies;
    TaskGraphViewModel m_taskGraph;
    AppearanceSettingsViewModel m_appearanceSettings;
};

} // namespace smartmate::viewmodel
