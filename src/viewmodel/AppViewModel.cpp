#include "AppViewModel.h"

namespace smartmate::viewmodel {

AppViewModel::AppViewModel(model::TaskService &taskService, QObject *parent)
    : QObject(parent)
    , m_taskCategories(taskService)
    , m_taskList(taskService)
    , m_taskFocus(taskService)
    , m_taskDetails(taskService)
    , m_taskEditor(taskService)
    , m_taskDependencies(new TaskDependencyViewModel(taskService, this))
    , m_taskGraph(taskService)
    , m_appearanceSettings()
{
}

AppViewModel::AppViewModel(model::TaskService &taskService,
                           model::AppearanceSettingsService &appearanceService,
                           QObject *parent)
    : QObject(parent)
    , m_taskCategories(taskService)
    , m_taskList(taskService)
    , m_taskFocus(taskService)
    , m_taskDetails(taskService)
    , m_taskEditor(taskService)
    , m_taskDependencies(new TaskDependencyViewModel(taskService, this))
    , m_taskGraph(taskService)
    , m_appearanceSettings(appearanceService)
{
}

AppViewModel::AppViewModel(model::TaskService &taskService,
                           model::TaskCategoryService &categoryService,
                           QObject *parent)
    : QObject(parent)
    , m_taskCategories(taskService, &categoryService)
    , m_taskList(taskService, categoryService)
    , m_taskFocus(taskService, categoryService)
    , m_taskDetails(taskService, categoryService)
    , m_taskEditor(taskService, categoryService)
    , m_taskDependencies(new TaskDependencyViewModel(
          taskService, categoryService, this))
    , m_taskGraph(taskService, categoryService)
    , m_appearanceSettings()
{
}

AppViewModel::AppViewModel(model::TaskService &taskService,
                           model::TaskCategoryService &categoryService,
                           model::AppearanceSettingsService &appearanceService,
                           QObject *parent)
    : QObject(parent)
    , m_taskCategories(taskService, &categoryService)
    , m_taskList(taskService, categoryService)
    , m_taskFocus(taskService, categoryService)
    , m_taskDetails(taskService, categoryService)
    , m_taskEditor(taskService, categoryService)
    , m_taskDependencies(new TaskDependencyViewModel(
          taskService, categoryService, this))
    , m_taskGraph(taskService, categoryService)
    , m_appearanceSettings(appearanceService)
{
    // 子 ViewModel 共享 Service，但没有相互引用；跨投影同步完全依靠 Service 失效通知。
}

TaskListViewModel *AppViewModel::taskList() noexcept
{
    return &m_taskList;
}

TaskFocusViewModel *AppViewModel::taskFocus() noexcept
{
    return &m_taskFocus;
}

TaskDetailsViewModel *AppViewModel::taskDetails() noexcept
{
    return &m_taskDetails;
}

TaskEditorViewModel *AppViewModel::taskEditor() noexcept
{
    return &m_taskEditor;
}

TaskDependencyViewModel *AppViewModel::taskDependencies() noexcept
{
    // 指针由 QObject 父子关系管理；返回仅用于向 Widget 注入 TaskDependencyContract。
    return m_taskDependencies;
}

TaskGraphViewModel *AppViewModel::taskGraph() noexcept
{
    return &m_taskGraph;
}

TaskCategoryViewModel *AppViewModel::taskCategories() noexcept
{
    return &m_taskCategories;
}

AppearanceSettingsViewModel *AppViewModel::appearanceSettings() noexcept
{
    return &m_appearanceSettings;
}

} // namespace smartmate::viewmodel
