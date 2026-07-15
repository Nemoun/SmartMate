#include "TaskDetailsViewModel.h"

#include "TaskCategoryPresentation.h"
#include "TaskPresentationFormatter.h"
#include "services/TaskCategoryService.h"
#include "services/TaskService.h"

#include <algorithm>

namespace smartmate::viewmodel {
namespace { constexpr auto detailDateTimeFormat = "yyyy-MM-dd HH:mm"; }

TaskDetailsViewModel::TaskDetailsViewModel(model::TaskService &taskService,
                                           QObject *parent)
    : TaskDetailsViewModel(taskService, nullptr, parent)
{
}

TaskDetailsViewModel::TaskDetailsViewModel(
    model::TaskService &taskService,
    model::TaskCategoryService &categoryService,
    QObject *parent)
    : TaskDetailsViewModel(taskService, &categoryService, parent)
{
}

TaskDetailsViewModel::TaskDetailsViewModel(
    model::TaskService &taskService,
    model::TaskCategoryService *categoryService,
    QObject *parent)
    : TaskDetailsContract(parent)
    , m_taskService(taskService)
    , m_categoryService(categoryService)
{
    // 详情不调用列表 ViewModel；共享 Service 的粗粒度失效通知触发本投影独立重建。
    connect(&m_taskService, &model::TaskService::tasksChanged,
            this, &TaskDetailsViewModel::reload);
    connect(&m_taskService, &model::TaskService::dependenciesChanged,
            this, &TaskDetailsViewModel::reload);
    if (m_categoryService) {
        connect(m_categoryService, &model::TaskCategoryService::categoriesChanged,
                this, &TaskDetailsViewModel::reloadCategories);
        connect(m_categoryService,
                &model::TaskCategoryService::taskCategoryAssignmentsChanged,
                this, &TaskDetailsViewModel::reload);
    }
    reloadCategories();
    reload();
}

QString TaskDetailsViewModel::selectedTaskId() const { return m_selectedTaskId.isNull() ? QString{} : m_selectedTaskId.toString(QUuid::WithoutBraces); }
QString TaskDetailsViewModel::selectedTitle() const { const auto *task = selectedTask(); return task ? task->title() : QString{}; }
QString TaskDetailsViewModel::selectedDescription() const { const auto *task = selectedTask(); return task ? task->description() : QString{}; }
QString TaskDetailsViewModel::selectedStatusText() const { const auto *task = selectedTask(); return task ? taskStatusText(task->status()) : QString{}; }
QString TaskDetailsViewModel::selectedPriorityText() const { const auto *task = selectedTask(); return task ? taskPriorityText(task->priority()) : QString{}; }
QString TaskDetailsViewModel::selectedDeadlineText() const { const auto *task = selectedTask(); return task ? taskDeadlineText(*task, {}) : QString{}; }
int TaskDetailsViewModel::selectedEstimatedMinutes() const noexcept { const auto *task = selectedTask(); return task && task->estimatedMinutes() ? *task->estimatedMinutes() : 0; }
QString TaskDetailsViewModel::selectedCreatedAtText() const { const auto *task = selectedTask(); return task ? task->createdAtUtc().toLocalTime().toString(QString::fromLatin1(detailDateTimeFormat)) : QString{}; }
QString TaskDetailsViewModel::selectedUpdatedAtText() const { const auto *task = selectedTask(); return task ? task->updatedAtUtc().toLocalTime().toString(QString::fromLatin1(detailDateTimeFormat)) : QString{}; }
QString TaskDetailsViewModel::selectedReasonText() const { return m_projection.orderReasonTexts.value(m_selectedTaskId); }
QString TaskDetailsViewModel::selectedBlockingReasonText() const { return m_projection.dependencyProjections.value(m_selectedTaskId).blockingReasonText; }
int TaskDetailsViewModel::selectedPredecessorCount() const noexcept { return m_projection.dependencyProjections.value(m_selectedTaskId).predecessorCount; }
int TaskDetailsViewModel::selectedUnlockCount() const noexcept { return m_projection.dependencyProjections.value(m_selectedTaskId).unlockCount; }
bool TaskDetailsViewModel::selectedCanEditTask() const noexcept { return m_projection.availabilityFor(m_selectedTaskId).canEditTask; }
bool TaskDetailsViewModel::selectedCanEditDependencies() const noexcept { return m_projection.availabilityFor(m_selectedTaskId).canEditDependencies; }
QString TaskDetailsViewModel::selectedCategoryName() const { const auto *category = selectedCategory(); return category ? category->name : QString{}; }
QString TaskDetailsViewModel::selectedCategoryAccent() const { const auto *category = selectedCategory(); return category ? taskCategoryAccent(category->color) : QStringLiteral("#94a3b8"); }
bool TaskDetailsViewModel::selectedHasCategory() const noexcept { return selectedCategory() != nullptr; }

bool TaskDetailsViewModel::selectTask(const QString &taskId)
{
    const model::TaskId id = QUuid::fromString(taskId.trimmed());
    if (id.isNull() || !m_projection.taskForId(id)) return false;
    if (m_selectedTaskId == id) return true;
    m_selectedTaskId = id;
    emit selectionChanged();
    return true;
}

void TaskDetailsViewModel::clearSelection()
{
    if (m_selectedTaskId.isNull()) return;
    m_selectedTaskId = model::TaskId{};
    emit selectionChanged();
}

void TaskDetailsViewModel::reload()
{
    const auto result = m_taskService.listRecommendedTasks();
    if (!result.ok()) return;
    TaskPlanProjection projection = makeTaskPlanProjection(*result.value);
    const bool selectionRemoved = !m_selectedTaskId.isNull()
        && !projection.taskForId(m_selectedTaskId);
    // 计划和选择均未变化时不通知，避免详情控件重复回填产生双向绑定回路。
    if (m_projection == projection && !selectionRemoved) return;
    m_projection = std::move(projection);
    if (selectionRemoved) m_selectedTaskId = model::TaskId{};
    emit selectionChanged();
}

void TaskDetailsViewModel::reloadCategories()
{
    if (!m_categoryService) return;
    const auto result = m_categoryService->listCategories();
    // 类别颜色或名称会改变详情 getter，因此复用 selectionChanged 统一要求绑定重读。
    if (!result.ok() || m_categories == *result.value) return;
    m_categories = *result.value;
    emit selectionChanged();
}

const model::Task *TaskDetailsViewModel::selectedTask() const { return m_projection.taskForId(m_selectedTaskId); }

const model::TaskCategory *TaskDetailsViewModel::selectedCategory() const
{
    const model::Task *task = selectedTask();
    if (!task || !task->categoryId()) return nullptr;
    const auto it = std::find_if(m_categories.cbegin(), m_categories.cend(),
        [task](const model::TaskCategory &category) { return category.id == *task->categoryId(); });
    return it == m_categories.cend() ? nullptr : &*it;
}

} // namespace smartmate::viewmodel
