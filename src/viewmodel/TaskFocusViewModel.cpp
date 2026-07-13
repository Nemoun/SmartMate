#include "TaskFocusViewModel.h"

#include "TaskCategoryPresentation.h"
#include "TaskPresentationFormatter.h"
#include "services/TaskCategoryService.h"
#include "services/TaskService.h"

#include <algorithm>

namespace smartmate::viewmodel {

TaskFocusViewModel::TaskFocusViewModel(model::TaskService &taskService,
                                       QObject *parent)
    : TaskFocusViewModel(taskService, nullptr, parent)
{
}

TaskFocusViewModel::TaskFocusViewModel(
    model::TaskService &taskService,
    model::TaskCategoryService &categoryService,
    QObject *parent)
    : TaskFocusViewModel(taskService, &categoryService, parent)
{
}

TaskFocusViewModel::TaskFocusViewModel(
    model::TaskService &taskService,
    model::TaskCategoryService *categoryService,
    QObject *parent)
    : TaskFocusContract(parent)
    , m_taskService(taskService)
    , m_categoryService(categoryService)
    , m_reloadTimer(this)
{
    connect(&m_taskService, &model::TaskService::tasksChanged,
            this, &TaskFocusViewModel::reload);
    connect(&m_taskService, &model::TaskService::dependenciesChanged,
            this, &TaskFocusViewModel::reload);
    if (m_categoryService) {
        connect(m_categoryService, &model::TaskCategoryService::categoriesChanged,
                this, &TaskFocusViewModel::reloadCategories);
        connect(m_categoryService,
                &model::TaskCategoryService::taskCategoryAssignmentsChanged,
                this, &TaskFocusViewModel::reload);
    }
    connect(&m_reloadTimer, &QTimer::timeout, this, &TaskFocusViewModel::reload);
    m_reloadTimer.start(60'000);
    reloadCategories();
    reload();
}

TaskFocusContract::FocusState TaskFocusViewModel::focusState() const noexcept { return m_focusState; }
QString TaskFocusViewModel::focusTaskId() const { return m_focusTaskId.isNull() ? QString{} : m_focusTaskId.toString(QUuid::WithoutBraces); }
QString TaskFocusViewModel::focusTitle() const { const auto *task = focusTask(); return task ? task->title() : QString{}; }
QString TaskFocusViewModel::focusDescription() const { const auto *task = focusTask(); return task ? task->description() : QString{}; }
QString TaskFocusViewModel::focusStatusText() const { const auto *task = focusTask(); return task ? taskStatusText(task->status()) : QString{}; }
QString TaskFocusViewModel::focusPriorityText() const { const auto *task = focusTask(); return task ? taskPriorityText(task->priority()) : QString{}; }
QString TaskFocusViewModel::focusDeadlineText() const { const auto *task = focusTask(); return task ? taskDeadlineText(*task, {}) : QString{}; }
int TaskFocusViewModel::focusEstimatedMinutes() const noexcept { const auto *task = focusTask(); return task && task->estimatedMinutes() ? *task->estimatedMinutes() : 0; }
QString TaskFocusViewModel::focusReasonText() const { return m_projection.orderReasonTexts.value(m_focusTaskId); }
bool TaskFocusViewModel::focusOverdue() const noexcept { return m_projection.overdueStates.value(m_focusTaskId, false); }
bool TaskFocusViewModel::focusCanStart() const noexcept { return m_projection.availabilityFor(m_focusTaskId).canStart; }
bool TaskFocusViewModel::focusCanComplete() const noexcept { return m_projection.availabilityFor(m_focusTaskId).canComplete; }
QString TaskFocusViewModel::focusCategoryName() const { const auto *category = focusCategory(); return category ? category->name : QString{}; }
QString TaskFocusViewModel::focusCategoryAccent() const { const auto *category = focusCategory(); return category ? taskCategoryAccent(category->color) : QStringLiteral("#94a3b8"); }
bool TaskFocusViewModel::focusHasCategory() const noexcept { return focusCategory() != nullptr; }

void TaskFocusViewModel::reload()
{
    const auto result = m_taskService.listRecommendedTasks();
    if (!result.ok()) return;

    TaskPlanProjection projection = makeTaskPlanProjection(*result.value);
    model::TaskId focusId;
    FocusState state = FocusState::NoTasks;
    bool hasTodo = false;
    for (const model::Task &task : projection.tasks) {
        if (task.status() == model::TaskStatus::InProgress) {
            focusId = task.id();
            state = FocusState::InProgress;
            break;
        }
        hasTodo = hasTodo || task.status() == model::TaskStatus::Todo;
    }
    if (focusId.isNull()) {
        for (const model::Task &task : projection.tasks) {
            if (projection.availabilityFor(task.id()).canStart) {
                focusId = task.id();
                state = FocusState::Suggested;
                break;
            }
        }
    }
    if (focusId.isNull()) state = hasTodo ? FocusState::AllBlocked : FocusState::NoTasks;

    if (m_projection == projection && m_focusTaskId == focusId && m_focusState == state) return;
    m_projection = std::move(projection);
    m_focusTaskId = focusId;
    m_focusState = state;
    emit focusTaskChanged();
}

void TaskFocusViewModel::reloadCategories()
{
    if (!m_categoryService) return;
    const auto result = m_categoryService->listCategories();
    if (!result.ok() || m_categories == *result.value) return;
    m_categories = *result.value;
    emit focusTaskChanged();
}

const model::Task *TaskFocusViewModel::focusTask() const { return m_projection.taskForId(m_focusTaskId); }

const model::TaskCategory *TaskFocusViewModel::focusCategory() const
{
    const model::Task *task = focusTask();
    if (!task || !task->categoryId()) return nullptr;
    const auto it = std::find_if(m_categories.cbegin(), m_categories.cend(),
        [task](const model::TaskCategory &category) { return category.id == *task->categoryId(); });
    return it == m_categories.cend() ? nullptr : &*it;
}

} // namespace smartmate::viewmodel
