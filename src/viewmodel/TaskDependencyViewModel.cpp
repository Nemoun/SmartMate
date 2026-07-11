#include "TaskDependencyViewModel.h"

#include "TaskErrorMapper.h"
#include "services/TaskService.h"

#include <QUuid>

#include <algorithm>

namespace smartmate::viewmodel {

TaskDependencyViewModel::TaskDependencyViewModel(model::TaskService &taskService,
                                                 QObject *parent)
    : QAbstractListModel(parent)
    , m_taskService(taskService)
{
}

int TaskDependencyViewModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_candidates.size();
}

QVariant TaskDependencyViewModel::data(const QModelIndex &index, const int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_candidates.size()) {
        return {};
    }

    const model::Task &task = m_candidates.at(index.row());
    const bool selected = m_selectedPredecessors.contains(task.id());
    switch (role) {
    case TaskIdRole:
        return task.id().toString(QUuid::WithoutBraces);
    case ShortIdRole:
        return task.id().toString(QUuid::WithoutBraces).left(8);
    case TitleRole:
        return task.title();
    case StatusTextRole:
        return statusText(task.status());
    case PriorityTextRole:
        return priorityText(task.priority());
    case SelectedRole:
        return selected;
    case ArchivedRole:
        return task.status() == model::TaskStatus::Archived;
    case SelectableRole:
        // 原有归档关系可以在草稿中撤销和恢复，但不能新增归档前置。
        return task.status() != model::TaskStatus::Archived
            || m_originalPredecessors.contains(task.id());
    default:
        return {};
    }
}

QHash<int, QByteArray> TaskDependencyViewModel::roleNames() const
{
    return {
        {TaskIdRole, "taskId"},
        {ShortIdRole, "shortId"},
        {TitleRole, "title"},
        {StatusTextRole, "statusText"},
        {PriorityTextRole, "priorityText"},
        {SelectedRole, "selected"},
        {ArchivedRole, "archived"},
        {SelectableRole, "selectable"},
    };
}

QString TaskDependencyViewModel::taskId() const
{
    return m_taskId.toString(QUuid::WithoutBraces);
}

QString TaskDependencyViewModel::taskTitle() const
{
    return m_taskTitle;
}

int TaskDependencyViewModel::count() const noexcept
{
    return m_candidates.size();
}

int TaskDependencyViewModel::selectedCount() const noexcept
{
    return m_selectedPredecessors.size();
}

bool TaskDependencyViewModel::dirty() const noexcept
{
    return m_selectedPredecessors != m_originalPredecessors;
}

bool TaskDependencyViewModel::canSave() const noexcept
{
    return !m_taskId.isNull() && dirty();
}

QString TaskDependencyViewModel::errorMessage() const
{
    return m_errorMessage;
}

bool TaskDependencyViewModel::beginEdit(const QString &taskId)
{
    const model::TaskId id = QUuid::fromString(taskId.trimmed());
    if (id.isNull()) {
        setErrorMessage(taskErrorMessage(model::TaskError::NotFound));
        return false;
    }

    const auto tasksResult = m_taskService.listTasks();
    if (!tasksResult.ok()) {
        setErrorMessage(taskErrorMessage(tasksResult.error));
        return false;
    }
    const auto dependenciesResult = m_taskService.listDependencies();
    if (!dependenciesResult.ok()) {
        setErrorMessage(taskErrorMessage(dependenciesResult.error));
        return false;
    }

    const auto taskIterator = std::find_if(
        tasksResult.value->cbegin(), tasksResult.value->cend(),
        [&id](const model::Task &task) { return task.id() == id; });
    if (taskIterator == tasksResult.value->cend()) {
        setErrorMessage(taskErrorMessage(model::TaskError::NotFound));
        return false;
    }
    if (taskIterator->status() != model::TaskStatus::Todo) {
        setErrorMessage(QStringLiteral("只有待办任务可以编辑前置依赖。"));
        return false;
    }

    QSet<model::TaskId> selectedPredecessors;
    for (const model::TaskDependency &dependency : *dependenciesResult.value) {
        if (dependency.successorId == id) {
            selectedPredecessors.insert(dependency.predecessorId);
        }
    }

    QHash<model::TaskId, QString> taskTitles;
    taskTitles.reserve(tasksResult.value->size());
    QList<model::Task> candidates;
    candidates.reserve(tasksResult.value->size());
    for (const model::Task &task : *tasksResult.value) {
        taskTitles.insert(task.id(), task.title());
        if (task.id() == id) {
            continue;
        }
        if (task.status() != model::TaskStatus::Archived
            || selectedPredecessors.contains(task.id())) {
            candidates.push_back(task);
        }
    }

    replaceDraft(*taskIterator, std::move(candidates), std::move(selectedPredecessors),
                 std::move(taskTitles));
    return true;
}

bool TaskDependencyViewModel::setPredecessorSelected(const QString &predecessorTaskId,
                                                     const bool selected)
{
    const model::TaskId id = QUuid::fromString(predecessorTaskId.trimmed());
    const int row = candidateRow(id);
    if (id.isNull() || row < 0) {
        setErrorMessage(taskErrorMessage(model::TaskError::NotFound));
        return false;
    }

    const model::Task &candidate = m_candidates.at(row);
    if (selected && candidate.status() == model::TaskStatus::Archived
        && !m_originalPredecessors.contains(id)) {
        setErrorMessage(QStringLiteral("不能新增已归档任务作为前置任务。"));
        return false;
    }
    if (m_selectedPredecessors.contains(id) == selected) {
        setErrorMessage({});
        return true;
    }

    if (selected) {
        m_selectedPredecessors.insert(id);
    } else {
        m_selectedPredecessors.remove(id);
    }
    emit dataChanged(index(row), index(row), {SelectedRole, SelectableRole});
    notifySelectionChanged();
    setErrorMessage({});
    return true;
}

bool TaskDependencyViewModel::save()
{
    if (!canSave()) {
        setErrorMessage(QStringLiteral("没有需要保存的依赖更改。"));
        return false;
    }

    // 按候选投影顺序提交稳定 ID，避免 QSet 的无序迭代影响测试和日志。
    QList<model::TaskId> predecessorIds;
    predecessorIds.reserve(m_selectedPredecessors.size());
    for (const model::Task &candidate : m_candidates) {
        if (m_selectedPredecessors.contains(candidate.id())) {
            predecessorIds.push_back(candidate.id());
        }
    }

    const auto result = m_taskService.replaceTaskPredecessors(m_taskId, predecessorIds);
    if (!result.ok()) {
        setErrorMessage(dependencyErrorMessage(result.error, result.context));
        return false;
    }

    m_originalPredecessors = m_selectedPredecessors;
    emit formStateChanged();
    setErrorMessage({});
    emit saved(taskId());
    return true;
}

void TaskDependencyViewModel::cancel()
{
    if (m_selectedPredecessors != m_originalPredecessors) {
        m_selectedPredecessors = m_originalPredecessors;
        if (!m_candidates.isEmpty()) {
            emit dataChanged(index(0), index(m_candidates.size() - 1),
                             {SelectedRole, SelectableRole});
        }
        notifySelectionChanged();
    }
    setErrorMessage({});
    emit cancelled();
}

void TaskDependencyViewModel::clearError()
{
    setErrorMessage({});
}

QString TaskDependencyViewModel::statusText(const model::TaskStatus status)
{
    switch (status) {
    case model::TaskStatus::Todo:
        return QStringLiteral("待办");
    case model::TaskStatus::InProgress:
        return QStringLiteral("进行中");
    case model::TaskStatus::Done:
        return QStringLiteral("已完成");
    case model::TaskStatus::Cancelled:
        return QStringLiteral("已取消");
    case model::TaskStatus::Archived:
        return QStringLiteral("已归档");
    }
    return {};
}

QString TaskDependencyViewModel::priorityText(const model::TaskPriority priority)
{
    switch (priority) {
    case model::TaskPriority::Low:
        return QStringLiteral("低");
    case model::TaskPriority::Normal:
        return QStringLiteral("普通");
    case model::TaskPriority::High:
        return QStringLiteral("高");
    case model::TaskPriority::Urgent:
        return QStringLiteral("紧急");
    }
    return {};
}

int TaskDependencyViewModel::candidateRow(const model::TaskId &taskId) const
{
    for (int row = 0; row < m_candidates.size(); ++row) {
        if (m_candidates.at(row).id() == taskId) {
            return row;
        }
    }
    return -1;
}

QString TaskDependencyViewModel::taskDisplayName(const model::TaskId &taskId) const
{
    const QString shortId = taskId.toString(QUuid::WithoutBraces).left(8);
    const QString title = m_taskTitles.value(taskId);
    if (!title.isEmpty()) {
        return QStringLiteral("%1（%2）").arg(title, shortId);
    }
    return QStringLiteral("未知任务（%1）").arg(shortId);
}

QString TaskDependencyViewModel::dependencyErrorMessage(
    const model::TaskError error,
    const model::TaskErrorContext &context) const
{
    const auto formatTasks = [this](const QList<model::TaskId> &ids,
                                    const QString &separator) {
        QStringList names;
        names.reserve(ids.size());
        for (const model::TaskId &id : ids) {
            names.push_back(taskDisplayName(id));
        }
        return names.join(separator);
    };

    if (error == model::TaskError::DependencyCycle && !context.cyclePath.isEmpty()) {
        return QStringLiteral("检测到循环依赖：%1。")
            .arg(formatTasks(context.cyclePath, QStringLiteral(" → ")));
    }
    if (error == model::TaskError::TaskBlocked
        && !context.blockingTaskIds.isEmpty()) {
        return QStringLiteral("任务仍被以下前置任务阻塞：%1。")
            .arg(formatTasks(context.blockingTaskIds, QStringLiteral("、")));
    }
    if (error == model::TaskError::DependencyStateConflict
        && !context.conflictingTaskIds.isEmpty()) {
        return QStringLiteral("状态修改会影响以下后继任务：%1。")
            .arg(formatTasks(context.conflictingTaskIds, QStringLiteral("、")));
    }
    if (error == model::TaskError::DependencyPredecessorNotEligible
        && !context.conflictingTaskIds.isEmpty()) {
        return QStringLiteral("不能新增已归档前置任务：%1。")
            .arg(formatTasks(context.conflictingTaskIds, QStringLiteral("、")));
    }
    return taskErrorMessage(error);
}

void TaskDependencyViewModel::replaceDraft(
    const model::Task &task,
    QList<model::Task> candidates,
    QSet<model::TaskId> selectedPredecessors,
    QHash<model::TaskId, QString> taskTitles)
{
    beginResetModel();
    m_taskId = task.id();
    m_taskTitle = task.title();
    m_candidates = std::move(candidates);
    m_taskTitles = std::move(taskTitles);
    m_selectedPredecessors = std::move(selectedPredecessors);
    m_originalPredecessors = m_selectedPredecessors;
    endResetModel();

    setErrorMessage({});
    emit contextChanged();
    emit countChanged();
    emit selectionChanged();
    emit formStateChanged();
}

void TaskDependencyViewModel::notifySelectionChanged()
{
    emit selectionChanged();
    emit formStateChanged();
}

void TaskDependencyViewModel::setErrorMessage(const QString &message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorMessageChanged();
}

} // namespace smartmate::viewmodel
