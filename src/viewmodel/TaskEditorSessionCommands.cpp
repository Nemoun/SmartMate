#include "TaskEditorViewModel.h"

#include "TaskErrorMapper.h"
#include "TaskPresentationFormatter.h"
#include "domain/TaskCreationRequest.h"
#include "services/TaskService.h"

#include <QDateTime>

#include <algorithm>
#include <utility>

namespace smartmate::viewmodel {

bool TaskEditorViewModel::beginCreate()
{
    // 候选资格由 Model 给出；ViewModel 只保存用于多选弹窗的展示快照。
    const auto candidates = m_taskService.listEligibleCreationPredecessors();
    if (!candidates.ok()) {
        setErrorMessage(taskErrorMessage(candidates.error));
        return false;
    }

    replaceCandidates(*candidates.value);
    replaceDraft(defaultSnapshot(), {}, false, model::TaskStatus::Todo);
    setSessionActive(true);
    return true;
}

bool TaskEditorViewModel::beginEdit(const QString &taskId)
{
    const auto id = QUuid::fromString(taskId.trimmed());
    if (id.isNull()) {
        setErrorMessage(taskErrorMessage(model::TaskError::NotFound));
        return false;
    }

    // 编辑资格由Model返回结构化结果；ViewModel只映射错误并决定是否建立草稿。
    const auto result = m_taskService.findEditableTask(id);
    if (!result.ok()) {
        setErrorMessage(taskErrorMessage(result.error));
        return false;
    }

    const auto &task = *result.value;
    Snapshot draft;
    draft.title = task.title();
    draft.description = task.description();
    draft.priorityIndex = taskPriorityIndex(task.priority());
    draft.deadline = task.deadline();
    draft.estimatedMinutes = task.estimatedMinutes();
    draft.categoryId = task.categoryId();

    replaceCandidates({});
    replaceDraft(draft, task.id().toString(QUuid::WithoutBraces), true,
                 task.status());
    setSessionActive(true);
    return true;
}

bool TaskEditorViewModel::setDeadlineSelection(const int year,
                                               const int month,
                                               const int day,
                                               const int hour,
                                               const int minute)
{
    // Reject 同时拒绝 DST 跳时产生的不存在时间与回拨产生的歧义时间，
    // 避免界面选择在保存时悄悄变成另一个时刻。
    const QDateTime selected{QDate{year, month, day},
                             QTime{hour, minute},
                             m_timeZone,
                             QDateTime::TransitionResolution::Reject};
    if (!selected.isValid()) {
        setErrorMessage(QStringLiteral("所选截止时间无效，请重新选择。"));
        return false;
    }

    if (m_deadline != selected) {
        m_deadline = selected;
        emit deadlineChanged();
        updateFormState();
    }
    setErrorMessage({});
    return true;
}

void TaskEditorViewModel::clearDeadline()
{
    if (!m_deadline.has_value()) {
        setErrorMessage({});
        return;
    }
    m_deadline.reset();
    emit deadlineChanged();
    setErrorMessage({});
    updateFormState();
}

bool TaskEditorViewModel::setEstimatedDuration(const int days,
                                               const int hours,
                                               const int minutes)
{
    // 组件范围属于选择器协议；总分钟的最终领域边界仍由 TaskService 校验。
    if (days < 0 || days > 365 || hours < 0 || hours > 23
        || minutes < 0 || minutes > 59) {
        setErrorMessage(taskErrorMessage(model::TaskError::InvalidEstimate));
        return false;
    }

    const int totalMinutes = days * 24 * 60 + hours * 60 + minutes;
    if (!m_taskService.validateEstimatedMinutes(totalMinutes).ok()) {
        setErrorMessage(taskErrorMessage(model::TaskError::InvalidEstimate));
        return false;
    }

    if (m_estimatedMinutes != totalMinutes) {
        m_estimatedMinutes = totalMinutes;
        emit estimatedDurationChanged();
        updateFormState();
    }
    setErrorMessage({});
    return true;
}

void TaskEditorViewModel::clearEstimatedDuration()
{
    if (!m_estimatedMinutes.has_value()) {
        setErrorMessage({});
        return;
    }
    m_estimatedMinutes.reset();
    emit estimatedDurationChanged();
    setErrorMessage({});
    updateFormState();
}

void TaskEditorViewModel::beginPredecessorSelection()
{
    if (!canConfigurePredecessors()) {
        return;
    }
    // 复制检查点形成可撤销子会话；弹窗取消不会污染主创建草稿。
    m_pickerPredecessors = m_selectedCreationPredecessors;
    m_predecessorPickerActive = true;
    notifyCandidateSelectionChanged();
}

bool TaskEditorViewModel::setCreationPredecessorSelected(const QString &taskId,
                                                         const bool selected)
{
    if (!m_predecessorPickerActive) {
        return false;
    }

    const model::TaskId id = QUuid::fromString(taskId.trimmed());
    const int row = candidateRow(id);
    if (id.isNull() || row < 0) {
        return false;
    }

    const bool alreadySelected = m_pickerPredecessors.contains(id);
    const bool changed = alreadySelected != selected;
    if (selected) {
        m_pickerPredecessors.insert(id);
    } else {
        m_pickerPredecessors.remove(id);
    }
    if (changed) {
        emit dataChanged(index(row), index(row), {CandidateSelectedRole});
        emit predecessorSelectionChanged();
    }
    return true;
}

void TaskEditorViewModel::acceptPredecessorSelection()
{
    if (!m_predecessorPickerActive) {
        return;
    }

    m_predecessorPickerActive = false;
    if (m_selectedCreationPredecessors == m_pickerPredecessors) {
        return;
    }
    m_selectedCreationPredecessors = m_pickerPredecessors;
    emit predecessorSelectionChanged();
    setErrorMessage({});
    updateFormState();
}

void TaskEditorViewModel::cancelPredecessorSelection()
{
    if (!m_predecessorPickerActive) {
        return;
    }
    const bool selectionChanged = m_pickerPredecessors != m_selectedCreationPredecessors;
    m_pickerPredecessors = m_selectedCreationPredecessors;
    m_predecessorPickerActive = false;
    if (selectionChanged) {
        notifyCandidateSelectionChanged();
        emit predecessorSelectionChanged();
    }
}

void TaskEditorViewModel::clearCreationPredecessors()
{
    if (m_predecessorPickerActive) {
        if (m_pickerPredecessors.isEmpty()) {
            return;
        }
        m_pickerPredecessors.clear();
        emit predecessorSelectionChanged();
        notifyCandidateSelectionChanged();
        return;
    }
    if (m_selectedCreationPredecessors.isEmpty()) {
        return;
    }
    m_selectedCreationPredecessors.clear();
    m_pickerPredecessors.clear();
    emit predecessorSelectionChanged();
    notifyCandidateSelectionChanged();
    setErrorMessage({});
    updateFormState();
}

bool TaskEditorViewModel::save()
{
    // 命令入口自身也必须守住状态，不能只依赖 Widget 将保存按钮设为不可用。
    updateFormState();
    if (!m_dirty) {
        setErrorMessage(QStringLiteral("没有需要保存的更改。"));
        return false;
    }
    if (!m_canSave) {
        setErrorMessage(m_validationMessage);
        return false;
    }

    const auto draft = buildTaskDraft();
    if (!draft.has_value()) {
        return false;
    }

    model::TaskResult result;
    if (m_editMode) {
        const auto id = QUuid::fromString(m_taskId);
        if (id.isNull()) {
            setErrorMessage(taskErrorMessage(model::TaskError::NotFound));
            return false;
        }
        result = m_taskService.updateTask(id, *draft);
    } else {
        // 创建草稿与全部前置稳定 ID 一次交给 Service，禁止逐边保存产生部分成功。
        QList<model::TaskId> predecessorIds = m_selectedCreationPredecessors.values();
        std::sort(predecessorIds.begin(), predecessorIds.end(),
                  [](const model::TaskId &left, const model::TaskId &right) {
                      return left.toString(QUuid::WithoutBraces)
                          < right.toString(QUuid::WithoutBraces);
                  });
        result = m_taskService.createTask(model::TaskCreationRequest{
            *draft,
            std::move(predecessorIds),
        });
    }

    if (!result.ok()) {
        setErrorMessage(taskErrorMessage(result.error));
        return false;
    }

    // 成功后以 Service 返回快照建立新检查点；先关闭会话状态，再发送 saved 流程信号。
    m_taskId = result.value->id().toString(QUuid::WithoutBraces);
    m_editMode = true;
    if (m_currentStatus != result.value->status()) {
        m_currentStatus = result.value->status();
        emit currentStatusTextChanged();
    }
    emit modeChanged();
    rememberCurrentDraft();
    updateFormState();
    setErrorMessage({});
    setSessionActive(false);
    emit saved(m_taskId);
    return true;
}

void TaskEditorViewModel::cancel()
{
    setErrorMessage({});
    // 主表单取消后立即丢弃字段、候选与已接受依赖，下一次打开不会继承旧草稿。
    replaceCandidates({});
    replaceDraft(defaultSnapshot(), {}, false, model::TaskStatus::Todo);
    setSessionActive(false);
    emit cancelled();
}

} // namespace smartmate::viewmodel

