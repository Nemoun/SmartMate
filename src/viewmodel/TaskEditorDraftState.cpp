#include "TaskEditorViewModel.h"

#include "TaskErrorMapper.h"
#include "TaskPresentationFormatter.h"
#include "services/TaskService.h"

#include <QDateTime>

#include <algorithm>
#include <utility>

namespace smartmate::viewmodel {

namespace {
// 优先级索引只在展示层转换为领域枚举；生成的草稿仍须由 TaskService
// 执行完整业务校验。任务状态不属于编辑草稿，必须经由显式状态命令修改。
[[nodiscard]] model::TaskDraft makeDraft(const QString &title,
                                         const QString &description,
                                         const model::TaskPriority priority,
                                         std::optional<QDateTime> deadline,
                                         std::optional<int> estimatedMinutes,
                                         std::optional<model::TaskCategoryId> categoryId)
{
    model::TaskDraft draft;
    draft.title = title;
    draft.description = description;
    draft.priority = priority;
    draft.deadline = std::move(deadline);
    draft.estimatedMinutes = estimatedMinutes;
    draft.categoryId = categoryId;
    return draft;
}
}

void TaskEditorViewModel::setSessionActive(const bool active)
{
    if (m_sessionActive == active) {
        return;
    }
    m_sessionActive = active;
    emit sessionActiveChanged();
}

TaskEditorViewModel::Snapshot TaskEditorViewModel::currentSnapshot() const
{
    return {
        m_title,
        m_description,
        m_priorityIndex,
        m_deadline,
        m_estimatedMinutes,
        m_categoryId,
        m_selectedCreationPredecessors,
    };
}

TaskEditorViewModel::Snapshot TaskEditorViewModel::defaultSnapshot()
{
    Snapshot snapshot;
    snapshot.priorityIndex = taskPriorityIndex(model::TaskPriority::Normal);
    return snapshot;
}

void TaskEditorViewModel::replaceDraft(const Snapshot &draft, const QString &taskId,
                                       const bool editMode,
                                       const model::TaskStatus currentStatus)
{
    // 一次替换全部字段后集中通知，Widget 可用 QSignalBlocker 做程序性回填。
    m_taskId = taskId;
    m_editMode = editMode;
    m_title = draft.title;
    m_description = draft.description;
    m_currentStatus = currentStatus;
    m_priorityIndex = draft.priorityIndex;
    m_deadline = draft.deadline;
    m_estimatedMinutes = draft.estimatedMinutes;
    m_categoryId = draft.categoryId;
    m_selectedCreationPredecessors = draft.predecessorIds;
    m_pickerPredecessors = m_selectedCreationPredecessors;
    m_predecessorPickerActive = false;
    m_original = draft;

    emit modeChanged();
    emit titleChanged();
    emit descriptionChanged();
    emit currentStatusTextChanged();
    emit priorityIndexChanged();
    emit deadlineChanged();
    emit estimatedDurationChanged();
    emit categoryChanged();
    emit predecessorSelectionChanged();
    notifyCandidateSelectionChanged();
    setErrorMessage({});
    updateFormState();
}

void TaskEditorViewModel::replaceCandidates(QList<model::Task> candidates)
{
    // 候选集合整体变化使用 reset；单项勾选只用 dataChanged。
    beginResetModel();
    m_predecessorCandidates = std::move(candidates);
    endResetModel();
    emit predecessorCandidatesChanged();
}

void TaskEditorViewModel::notifyCandidateSelectionChanged()
{
    if (!m_predecessorCandidates.isEmpty()) {
        emit dataChanged(index(0), index(m_predecessorCandidates.size() - 1),
                         {CandidateSelectedRole});
    }
}

void TaskEditorViewModel::rememberCurrentDraft()
{
    m_original = currentSnapshot();
}

void TaskEditorViewModel::updateFormState()
{
    // 类型化选择器已经消除了格式解析；标题、预计范围等业务规则仍由
    // TaskService::validateDraft 统一给出结论。
    QString validationMessage;
    const auto priority = taskPriorityFromIndex(m_priorityIndex);
    if (!priority.has_value()) {
        validationMessage = taskErrorMessage(model::TaskError::InvalidPriority);
    } else {
        const auto validation = m_taskService.validateDraft(
            makeDraft(m_title, m_description, *priority,
                      m_deadline, m_estimatedMinutes, m_categoryId));
        if (!validation.ok()) {
            validationMessage = taskErrorMessage(validation.error);
        }
    }
    const bool dirty = currentSnapshot() != m_original;
    const bool canSave = dirty && validationMessage.isEmpty();
    if (m_dirty == dirty && m_canSave == canSave
        && m_validationMessage == validationMessage) {
        return;
    }

    m_dirty = dirty;
    m_canSave = canSave;
    m_validationMessage = validationMessage;
    // 一个聚合通知覆盖三个派生 getter，保证 dirty/canSave/文案来自同一次计算。
    emit formStateChanged();
}

void TaskEditorViewModel::setErrorMessage(const QString &message)
{
    // 一次性错误通知与 errorMessage 可观察属性分开，重复属性值不重复通知绑定。
    if (!message.isEmpty()) {
        emit notificationRaised({smartmate::common::UiSeverity::Error,
                                 QStringLiteral("任务编辑失败"),
                                 message});
    }
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorMessageChanged();
}

std::optional<model::TaskDraft> TaskEditorViewModel::buildTaskDraft()
{
    updateFormState();
    if (!m_validationMessage.isEmpty()) {
        setErrorMessage(m_validationMessage);
        return std::nullopt;
    }

    const auto priority = taskPriorityFromIndex(m_priorityIndex);
    if (!priority.has_value()) {
        setErrorMessage(taskErrorMessage(model::TaskError::InvalidPriority));
        return std::nullopt;
    }
    return makeDraft(m_title, m_description, *priority,
                     m_deadline, m_estimatedMinutes, m_categoryId);
}

std::optional<QDateTime> TaskEditorViewModel::displayedDeadline() const
{
    if (!m_deadline.has_value()) {
        return std::nullopt;
    }
    return m_deadline->toTimeZone(m_timeZone);
}

int TaskEditorViewModel::candidateRow(const model::TaskId &taskId) const
{
    for (int row = 0; row < m_predecessorCandidates.size(); ++row) {
        if (m_predecessorCandidates.at(row).id() == taskId) {
            return row;
        }
    }
    return -1;
}

void TaskEditorViewModel::applyCategories()
{
    emit categoryOptionsChanged();

    // 若数据库中的原类别已被管理命令删除，持久化快照已经自动转为未分类；
    // 同步原始草稿可避免把该外部变化误判为用户编辑。
    if (m_editMode && m_original.categoryId.has_value()) {
        const auto originalId = *m_original.categoryId;
        const bool originalStillExists = std::any_of(
            m_categorySource.categories().cbegin(),
            m_categorySource.categories().cend(),
            [&originalId](const auto &category) {
                return category.id == originalId;
            });
        if (!originalStillExists) {
            m_original.categoryId.reset();
        }
    }

    // 删除类别是管理操作例外；若当前草稿引用了被删除类别，只清空类别字段，
    // 其他输入仍留在本地，避免用户丢失尚未保存的编辑内容。
    if (m_categoryId.has_value() && selectedCategory() == nullptr) {
        m_categoryId.reset();
        emit categoryChanged();
        updateFormState();
        setErrorMessage(QStringLiteral("原选择的类别已删除，任务将保存为未分类。"));
    }
    if (!m_predecessorCandidates.isEmpty()) {
        emit dataChanged(index(0), index(m_predecessorCandidates.size() - 1),
                         {CandidateCategoryNameRole, CandidateCategoryAccentRole,
                          CandidateHasCategoryRole});
    }
}

} // namespace smartmate::viewmodel

