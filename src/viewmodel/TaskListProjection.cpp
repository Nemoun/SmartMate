#include "TaskListViewModel.h"

#include "TaskErrorMapper.h"
#include "TaskPresentationFormatter.h"

#include <algorithm>
#include <utility>

namespace smartmate::viewmodel {

void TaskListViewModel::rebuildVisibleTasks()
{
    const auto &tasks = m_planSource.projection().tasks;
    // 整批替换投影时遵循 QAbstractItemModel 的重置协议，使 Widget 安全重建行绑定。
    beginResetModel();
    m_visibleTasks.clear();
    m_visibleTasks.reserve(tasks.size());
    m_visibleTaskIds.clear();
    m_visibleTaskIds.reserve(tasks.size());

    const QString keyword = m_searchText.trimmed();
    const bool filtersPriority =
        m_priorityFilterIndex != allPrioritiesFilterIndex;
    const auto selectedPriority = taskPriorityFromIndex(
        m_priorityFilterIndex - firstPriorityFilterIndex);

    for (const auto &task : tasks) {
        const bool archived = task.status() == model::TaskStatus::Archived;
        if (archived != m_showArchived) {
            continue;
        }
        if (!keyword.isEmpty()
            && !task.title().contains(keyword, Qt::CaseInsensitive)
            && !task.description().contains(keyword, Qt::CaseInsensitive)) {
            continue;
        }
        if (filtersPriority
            && (!selectedPriority.has_value()
                || task.priority() != *selectedPriority)) {
            continue;
        }
        if (m_categoryFilterMode == 1 && task.categoryId().has_value()) {
            continue;
        }
        if (m_categoryFilterMode == 2
            && (!task.categoryId().has_value()
                || *task.categoryId() != m_categoryFilterCategoryId)) {
            continue;
        }
        // 过滤只删除不匹配项，剩余任务严格保留Model给出的计划顺序。
        m_visibleTasks.push_back(task);
        m_visibleTaskIds.insert(task.id());
    }

    // 刷新或重排只保留仍在当前结果中且仍具备资格的稳定 TaskId。
    pruneBulkSelection();
    endResetModel();
    emit countChanged();
    emit bulkSelectionChanged();
}

void TaskListViewModel::applyPlanProjection()
{
    rebuildVisibleTasks();
}

void TaskListViewModel::applyCategories()
{
    emit categoryOptionsChanged();

    if (m_categoryFilterMode == 2) {
        const auto &categories = m_categorySource.categories();
        const bool exists = std::any_of(
            categories.cbegin(), categories.cend(),
            [this](const auto &category) {
                return category.id == m_categoryFilterCategoryId;
            });
        if (!exists) {
            // 删除当前筛选类别后，相关任务已原子转为未分类，保持用户视角连续。
            m_categoryFilterMode = 1;
            m_categoryFilterCategoryId = model::TaskCategoryId{};
            m_bulkSelectedTaskIds.clear();
            emit categoryFilterChanged();
            emit hasActiveFiltersChanged();
        }
    }
    rebuildVisibleTasks();
}

void TaskListViewModel::syncSourceError()
{
    if (m_planSource.lastError() != model::TaskError::None) {
        setError(taskErrorMessage(m_planSource.lastError()));
        return;
    }
    if (m_categorySource.lastError() != model::TaskCategoryError::None) {
        setError(QStringLiteral("类别数据访问失败，请稍后重试。"));
        return;
    }
    setError({});
}

const model::TaskCategory *TaskListViewModel::categoryForTask(
    const model::Task *task) const
{
    if (!task || !task->categoryId().has_value()) return nullptr;
    const auto &categories = m_categorySource.categories();
    const auto iterator = std::find_if(
        categories.cbegin(), categories.cend(), [&](const auto &category) {
            return category.id == *task->categoryId();
        });
    return iterator == categories.cend() ? nullptr : &*iterator;
}

void TaskListViewModel::setError(const QString &message)
{
    // errorMessage 属性支持持续绑定，notificationRaised 是一次性展示通知；
    // errorOccurred 为兼容既有绑定保留，三者均只表达展示错误，不承载业务控制流。
    if (!message.isEmpty()) {
        emit notificationRaised({smartmate::common::UiSeverity::Error,
                                 QStringLiteral("任务操作失败"),
                                 message});
    }
    if (m_errorMessage == message) {
        if (!message.isEmpty()) {
            emit errorOccurred(message);
        }
        return;
    }

    m_errorMessage = message;
    emit errorMessageChanged();
    if (!message.isEmpty()) {
        emit errorOccurred(message);
    }
}

} // namespace smartmate::viewmodel

