#include "TaskListViewModel.h"

#include "TaskCategoryPresentation.h"
#include "TaskPresentationFormatter.h"

#include <algorithm>

namespace smartmate::viewmodel {

TaskListViewModel::TaskListViewModel(
    model::TaskService &taskService,
    TaskPlanProjectionSource &planSource,
    TaskCategoryProjectionSource &categorySource,
    QObject *parent)
    : TaskListContract(parent)
    , m_taskService(taskService)
    , m_planSource(planSource)
    , m_categorySource(categorySource)
{
    connect(&m_planSource, &TaskPlanProjectionSource::projectionChanged,
            this, &TaskListViewModel::applyPlanProjection);
    connect(&m_planSource, &TaskPlanProjectionSource::refreshSucceeded,
            this, &TaskListViewModel::syncSourceError);
    connect(&m_planSource, &TaskPlanProjectionSource::refreshFailed,
            this, &TaskListViewModel::syncSourceError);
    connect(&m_categorySource, &TaskCategoryProjectionSource::categoriesChanged,
            this, &TaskListViewModel::applyCategories);
    connect(&m_categorySource, &TaskCategoryProjectionSource::refreshSucceeded,
            this, &TaskListViewModel::syncSourceError);
    connect(&m_categorySource, &TaskCategoryProjectionSource::refreshFailed,
            this, &TaskListViewModel::syncSourceError);
    applyCategories();
    applyPlanProjection();
    syncSourceError();
}

int TaskListViewModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visibleTasks.size();
}

int TaskListViewModel::count() const noexcept
{
    return m_visibleTasks.size();
}

QVariant TaskListViewModel::data(const QModelIndex &index, const int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleTasks.size()) {
        return {};
    }

    const auto &task = m_visibleTasks.at(index.row());
    switch (role) {
    case TaskIdRole:
        return task.id().toString(QUuid::WithoutBraces);
    case TitleRole:
        return task.title();
    case DescriptionRole:
        return task.description();
    case StatusRole:
        return static_cast<int>(taskStatusVisual(task.status()));
    case StatusTextRole:
        return taskStatusText(task.status());
    case PriorityRole:
        return static_cast<int>(taskPriorityVisual(task.priority()));
    case PriorityTextRole:
        return taskPriorityText(task.priority());
    case DeadlineTextRole:
        return taskDeadlineText(task, {});
    case EstimatedMinutesRole:
        return task.estimatedMinutes().has_value() ? *task.estimatedMinutes() : 0;
    case ArchivedRole:
        return task.status() == model::TaskStatus::Archived;
    case OverdueRole:
        return m_planSource.projection().overdueStates.value(task.id(), false);
    case OrderReasonTextRole:
        return m_planSource.projection().orderReasonTexts.value(task.id());
    case BlockedRole:
        return m_planSource.projection().dependencyProjections.value(task.id()).blocked;
    case BlockingReasonTextRole:
        return m_planSource.projection().dependencyProjections.value(task.id()).blockingReasonText;
    case PredecessorCountRole:
        return m_planSource.projection().dependencyProjections.value(task.id()).predecessorCount;
    case UnlockCountRole:
        return m_planSource.projection().dependencyProjections.value(task.id()).unlockCount;
    case CanEditTaskRole:
        return availabilityFor(task.id()).canEditTask;
    case CanEditDependenciesRole:
        return availabilityFor(task.id()).canEditDependencies;
    case CanStartRole:
        return availabilityFor(task.id()).canStart;
    case CanCancelRole:
        return availabilityFor(task.id()).canCancel;
    case CanCompleteRole:
        return availabilityFor(task.id()).canComplete;
    case CanRedoRole:
        return availabilityFor(task.id()).canRedo;
    case CanArchiveRole:
        return availabilityFor(task.id()).canArchive;
    case CanRestoreRole:
        return availabilityFor(task.id()).canRestore;
    case CanDeletePermanentlyRole:
        return availabilityFor(task.id()).canDeletePermanently;
    case BulkSelectedRole:
        return m_bulkSelectedTaskIds.contains(task.id());
    case BulkSelectableRole:
        return isBulkSelectable(task.id());
    case CategoryIdRole:
        return task.categoryId().has_value()
            ? task.categoryId()->toString(QUuid::WithoutBraces) : QString{};
    case CategoryNameRole: {
        const auto *category = categoryForTask(&task);
        return category ? category->name : QString{};
    }
    case CategoryAccentRole: {
        const auto *category = categoryForTask(&task);
        return category ? taskCategoryAccent(category->color)
                        : taskUncategorizedAccent();
    }
    case HasCategoryRole:
        return categoryForTask(&task) != nullptr;
    default:
        return {};
    }
}

QHash<int, QByteArray> TaskListViewModel::roleNames() const
{
    return {
        {TaskIdRole, "taskId"},
        {TitleRole, "title"},
        {DescriptionRole, "description"},
        {StatusRole, "status"},
        {StatusTextRole, "statusText"},
        {PriorityRole, "priority"},
        {PriorityTextRole, "priorityText"},
        {DeadlineTextRole, "deadlineText"},
        {EstimatedMinutesRole, "estimatedMinutes"},
        {ArchivedRole, "archived"},
        {OverdueRole, "overdue"},
        {OrderReasonTextRole, "orderReasonText"},
        {BlockedRole, "blocked"},
        {BlockingReasonTextRole, "blockingReasonText"},
        {PredecessorCountRole, "predecessorCount"},
        {UnlockCountRole, "unlockCount"},
        {CanEditTaskRole, "canEditTask"},
        {CanEditDependenciesRole, "canEditDependencies"},
        {CanStartRole, "canStart"},
        {CanCancelRole, "canCancel"},
        {CanCompleteRole, "canComplete"},
        {CanRedoRole, "canRedo"},
        {CanArchiveRole, "canArchive"},
        {CanRestoreRole, "canRestore"},
        {CanDeletePermanentlyRole, "canDeletePermanently"},
        {BulkSelectedRole, "bulkSelected"},
        {BulkSelectableRole, "bulkSelectable"},
        {CategoryIdRole, "categoryId"},
        {CategoryNameRole, "categoryName"},
        {CategoryAccentRole, "categoryAccent"},
        {HasCategoryRole, "hasCategory"},
    };
}

bool TaskListViewModel::showArchived() const noexcept
{
    return m_showArchived;
}

QString TaskListViewModel::searchText() const
{
    return m_searchText;
}

int TaskListViewModel::priorityFilterIndex() const noexcept
{
    return m_priorityFilterIndex;
}

QStringList TaskListViewModel::priorityFilterOptions() const
{
    return taskPriorityFilterOptions();
}

QVariantList TaskListViewModel::categoryFilterOptions() const
{
    const auto &categories = m_categorySource.categories();
    QVariantList options;
    options.reserve(categories.size() + 2);
    options.append(QVariantMap{{QStringLiteral("mode"), 0},
                               {QStringLiteral("categoryId"), QString{}},
                               {QStringLiteral("name"), QStringLiteral("全部类别")},
                               {QStringLiteral("accent"), taskAllCategoriesAccent()}});
    options.append(QVariantMap{{QStringLiteral("mode"), 1},
                               {QStringLiteral("categoryId"), QString{}},
                               {QStringLiteral("name"), QStringLiteral("未分类")},
                               {QStringLiteral("accent"), taskUncategorizedAccent()}});
    for (const auto &category : categories) {
        options.append(QVariantMap{
            {QStringLiteral("mode"), 2},
            {QStringLiteral("categoryId"), category.id.toString(QUuid::WithoutBraces)},
            {QStringLiteral("name"), category.name},
            {QStringLiteral("accent"), taskCategoryAccent(category.color)}});
    }
    return options;
}

int TaskListViewModel::categoryFilterMode() const noexcept
{
    return m_categoryFilterMode;
}

QString TaskListViewModel::categoryFilterCategoryId() const
{
    return m_categoryFilterMode == 2 && !m_categoryFilterCategoryId.isNull()
        ? m_categoryFilterCategoryId.toString(QUuid::WithoutBraces)
        : QString{};
}

bool TaskListViewModel::hasActiveFilters() const
{
    return !m_searchText.trimmed().isEmpty()
        || m_priorityFilterIndex != allPrioritiesFilterIndex
        || m_categoryFilterMode != 0;
}

bool TaskListViewModel::bulkSelectionMode() const noexcept
{
    return m_bulkSelectionMode;
}

int TaskListViewModel::bulkSelectedCount() const noexcept
{
    return m_bulkSelectedTaskIds.size();
}

int TaskListViewModel::bulkSelectableVisibleCount() const
{
    return static_cast<int>(std::count_if(
        m_visibleTasks.cbegin(), m_visibleTasks.cend(),
        [this](const model::Task &task) { return isBulkSelectable(task.id()); }));
}

bool TaskListViewModel::allVisibleSelected() const
{
    const int selectableCount = bulkSelectableVisibleCount();
    return selectableCount > 0 && selectableCount == m_bulkSelectedTaskIds.size();
}

bool TaskListViewModel::canBulkArchive() const noexcept
{
    return m_bulkSelectionMode && !m_showArchived
        && !m_bulkSelectedTaskIds.isEmpty();
}

bool TaskListViewModel::canBulkRestore() const
{
    return m_bulkSelectionMode && m_showArchived
        && !m_bulkSelectedTaskIds.isEmpty()
        && std::all_of(m_bulkSelectedTaskIds.cbegin(),
                       m_bulkSelectedTaskIds.cend(),
                       [this](const model::TaskId &id) {
                           return availabilityFor(id).canRestore;
                       });
}

bool TaskListViewModel::canBulkDelete() const noexcept
{
    return m_bulkSelectionMode && m_showArchived
        && !m_bulkSelectedTaskIds.isEmpty();
}

QString TaskListViewModel::errorMessage() const
{
    return m_errorMessage;
}

void TaskListViewModel::setShowArchived(const bool showArchived)
{
    if (m_showArchived == showArchived) {
        return;
    }

    m_bulkSelectionMode = false;
    m_bulkSelectedTaskIds.clear();
    m_showArchived = showArchived;
    emit showArchivedChanged();
    rebuildVisibleTasks();
}

void TaskListViewModel::setSearchText(const QString &searchText)
{
    if (m_searchText == searchText) {
        return;
    }

    const bool previouslyActive = hasActiveFilters();
    m_bulkSelectedTaskIds.clear();
    m_searchText = searchText;
    emit searchTextChanged();
    if (previouslyActive != hasActiveFilters()) {
        emit hasActiveFiltersChanged();
    }
    rebuildVisibleTasks();
}

void TaskListViewModel::setPriorityFilterIndex(const int priorityFilterIndex)
{
    if (priorityFilterIndex < allPrioritiesFilterIndex
        || priorityFilterIndex >= taskPriorityFilterOptions().size()
        || m_priorityFilterIndex == priorityFilterIndex) {
        return;
    }

    const bool previouslyActive = hasActiveFilters();
    m_bulkSelectedTaskIds.clear();
    m_priorityFilterIndex = priorityFilterIndex;
    emit priorityFilterIndexChanged();
    if (previouslyActive != hasActiveFilters()) {
        emit hasActiveFiltersChanged();
    }
    rebuildVisibleTasks();
}

bool TaskListViewModel::setCategoryFilter(const int mode,
                                          const QString &categoryId)
{
    if (mode < 0 || mode > 2) return false;
    model::TaskCategoryId id;
    if (mode == 2) {
        id = QUuid::fromString(categoryId.trimmed());
        const auto &categories = m_categorySource.categories();
        const bool exists = std::any_of(
            categories.cbegin(), categories.cend(),
            [&id](const auto &category) { return category.id == id; });
        if (id.isNull() || !exists) return false;
    }
    if (m_categoryFilterMode == mode
        && (mode != 2 || m_categoryFilterCategoryId == id)) return true;

    const bool previouslyActive = hasActiveFilters();
    m_bulkSelectedTaskIds.clear();
    m_categoryFilterMode = mode;
    m_categoryFilterCategoryId = mode == 2 ? id : model::TaskCategoryId{};
    emit categoryFilterChanged();
    if (previouslyActive != hasActiveFilters()) emit hasActiveFiltersChanged();
    rebuildVisibleTasks();
    return true;
}

void TaskListViewModel::reload()
{
    m_planSource.refresh();
}

void TaskListViewModel::clearFilters()
{
    const bool searchChanged = !m_searchText.isEmpty();
    const bool priorityChanged =
        m_priorityFilterIndex != allPrioritiesFilterIndex;
    const bool categoryChanged = m_categoryFilterMode != 0;
    if (!searchChanged && !priorityChanged && !categoryChanged) {
        return;
    }

    const bool previouslyActive = hasActiveFilters();
    m_bulkSelectedTaskIds.clear();
    m_searchText.clear();
    m_priorityFilterIndex = allPrioritiesFilterIndex;
    m_categoryFilterMode = 0;
    m_categoryFilterCategoryId = model::TaskCategoryId{};
    if (searchChanged) {
        emit searchTextChanged();
    }
    if (priorityChanged) {
        emit priorityFilterIndexChanged();
    }
    if (categoryChanged) emit categoryFilterChanged();
    if (previouslyActive != hasActiveFilters()) {
        emit hasActiveFiltersChanged();
    }
    // 批量清除只重建一次，避免 Widget 短暂观察到多个中间筛选状态。
    rebuildVisibleTasks();
}

} // namespace smartmate::viewmodel
