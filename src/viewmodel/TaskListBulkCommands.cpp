#include "TaskListViewModel.h"

#include "TaskErrorMapper.h"
#include "services/TaskService.h"

#include <QStringList>

#include <algorithm>
#include <utility>

namespace smartmate::viewmodel {

void TaskListViewModel::beginBulkSelection()
{
    if (m_bulkSelectionMode) {
        return;
    }
    m_bulkSelectionMode = true;
    m_bulkSelectedTaskIds.clear();
    emit bulkSelectionChanged();
}

bool TaskListViewModel::toggleBulkSelection(const QString &taskId)
{
    if (!m_bulkSelectionMode) {
        return false;
    }
    const model::TaskId id = parseTaskId(taskId);
    if (id.isNull() || !isBulkSelectable(id)) {
        return false;
    }

    QSet<model::TaskId> selection = m_bulkSelectedTaskIds;
    if (!selection.remove(id)) {
        selection.insert(id);
    }
    setBulkSelection(std::move(selection));
    return true;
}

void TaskListViewModel::toggleSelectAllVisible()
{
    if (!m_bulkSelectionMode) {
        return;
    }

    QSet<model::TaskId> visibleSelection;
    for (const model::Task &task : std::as_const(m_visibleTasks)) {
        if (isBulkSelectable(task.id())) {
            visibleSelection.insert(task.id());
        }
    }
    setBulkSelection(allVisibleSelected() ? QSet<model::TaskId>{}
                                          : std::move(visibleSelection));
}

void TaskListViewModel::clearBulkSelection()
{
    setBulkSelection({});
}

void TaskListViewModel::cancelBulkSelection()
{
    if (!m_bulkSelectionMode && m_bulkSelectedTaskIds.isEmpty()) {
        return;
    }
    m_bulkSelectionMode = false;
    if (m_bulkSelectedTaskIds.isEmpty()) {
        emit bulkSelectionChanged();
    } else {
        setBulkSelection({});
    }
}

bool TaskListViewModel::archiveSelectedTasks()
{
    const auto result = m_taskService.archiveTasks(sortedBulkSelection());
    if (!result.ok()) {
        QList<model::TaskId> contextIds = result.context.conflictingTaskIds;
        if (contextIds.isEmpty()) {
            contextIds = result.context.blockingTaskIds;
        }
        if (contextIds.isEmpty()) {
            contextIds = sortedBulkSelection();
        }
        const QString context = taskIdsContext(contextIds);
        setError(context.isEmpty()
                     ? taskErrorMessage(result.error)
                     : QStringLiteral("%1：%2")
                           .arg(context, taskErrorMessage(result.error)));
        return false;
    }
    setError({});
    cancelBulkSelection();
    return true;
}

bool TaskListViewModel::restoreSelectedTasks()
{
    const auto result = m_taskService.restoreTasks(sortedBulkSelection());
    if (!result.ok()) {
        QList<model::TaskId> contextIds = result.context.conflictingTaskIds;
        if (contextIds.isEmpty()) {
            contextIds = result.context.blockingTaskIds;
        }
        if (contextIds.isEmpty()) {
            contextIds = sortedBulkSelection();
        }
        const QString context = taskIdsContext(contextIds);
        setError(context.isEmpty()
                     ? taskErrorMessage(result.error)
                     : QStringLiteral("%1：%2")
                           .arg(context, taskErrorMessage(result.error)));
        return false;
    }
    setError({});
    cancelBulkSelection();
    return true;
}

bool TaskListViewModel::deleteSelectedArchivedTasks()
{
    const auto result = m_taskService.deleteArchivedTasks(sortedBulkSelection());
    if (!result.ok()) {
        QList<model::TaskId> contextIds = result.context.conflictingTaskIds;
        if (contextIds.isEmpty()) {
            contextIds = result.context.blockingTaskIds;
        }
        if (contextIds.isEmpty()) {
            contextIds = sortedBulkSelection();
        }
        const QString context = taskIdsContext(contextIds);
        setError(context.isEmpty()
                     ? taskErrorMessage(result.error)
                     : QStringLiteral("%1：%2")
                           .arg(context, taskErrorMessage(result.error)));
        return false;
    }
    setError({});
    cancelBulkSelection();
    return true;
}

model::TaskId TaskListViewModel::parseTaskId(const QString &taskId)
{
    return QUuid::fromString(taskId.trimmed());
}

const model::Task *TaskListViewModel::taskForId(
    const model::TaskId &taskId) const
{
    if (taskId.isNull()) {
        return nullptr;
    }
    const auto &tasks = m_planSource.projection().tasks;
    const auto iterator = std::find_if(
        tasks.cbegin(), tasks.cend(), [&taskId](const model::Task &task) {
            return task.id() == taskId;
        });
    return iterator == tasks.cend() ? nullptr : &*iterator;
}

const model::TaskCommandAvailability &TaskListViewModel::availabilityFor(
    const model::TaskId &taskId) const
{
    return m_planSource.projection().availabilityFor(taskId);
}

bool TaskListViewModel::isBulkSelectable(const model::TaskId &taskId) const
{
    if (!m_visibleTaskIds.contains(taskId)) {
        return false;
    }
    const model::TaskCommandAvailability &availability = availabilityFor(taskId);
    return m_showArchived ? availability.canDeletePermanently
                          : availability.canArchive;
}

QList<model::TaskId> TaskListViewModel::sortedBulkSelection() const
{
    QList<model::TaskId> ids = m_bulkSelectedTaskIds.values();
    std::sort(ids.begin(), ids.end(), [](const model::TaskId &left,
                                         const model::TaskId &right) {
        return left.toString(QUuid::WithoutBraces)
            < right.toString(QUuid::WithoutBraces);
    });
    return ids;
}

QString TaskListViewModel::taskIdsContext(
    const QList<model::TaskId> &taskIds) const
{
    QStringList labels;
    QList<model::TaskId> ids = taskIds;
    std::sort(ids.begin(), ids.end(), [](const model::TaskId &left,
                                         const model::TaskId &right) {
        return left.toString(QUuid::WithoutBraces)
            < right.toString(QUuid::WithoutBraces);
    });
    const int visibleLabelCount = std::min(3, static_cast<int>(ids.size()));
    labels.reserve(visibleLabelCount);
    for (int index = 0; index < visibleLabelCount; ++index) {
        const model::TaskId &id = ids.at(index);
        const model::Task *task = taskForId(id);
        const QString shortId = id.toString(QUuid::WithoutBraces).left(8);
        labels.push_back(task == nullptr
                             ? shortId
                             : QStringLiteral("“%1”（%2）")
                                   .arg(task->title(), shortId));
    }
    if (ids.size() > visibleLabelCount) {
        labels.push_back(QStringLiteral("另 %1 项")
                             .arg(ids.size() - visibleLabelCount));
    }
    return labels.join(QStringLiteral("、"));
}

void TaskListViewModel::pruneBulkSelection()
{
    QSet<model::TaskId> retained;
    for (const model::TaskId &id : std::as_const(m_bulkSelectedTaskIds)) {
        if (isBulkSelectable(id)) {
            retained.insert(id);
        }
    }
    m_bulkSelectedTaskIds = std::move(retained);
}

void TaskListViewModel::setBulkSelection(QSet<model::TaskId> selection)
{
    if (m_bulkSelectedTaskIds == selection) {
        return;
    }
    m_bulkSelectedTaskIds = std::move(selection);
    // 选择属于会话级展示状态，仅精确通知 BulkSelectedRole，避免无关 Role 重绑定。
    if (!m_visibleTasks.isEmpty()) {
        emit dataChanged(index(0), index(m_visibleTasks.size() - 1),
                         {BulkSelectedRole});
    }
    emit bulkSelectionChanged();
}

} // namespace smartmate::viewmodel

