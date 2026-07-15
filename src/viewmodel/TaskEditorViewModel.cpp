#include "TaskEditorViewModel.h"

#include "TaskCategoryPresentation.h"
#include "TaskPresentationFormatter.h"
#include "domain/TaskConstraints.h"

#include <QDateTime>
#include <QStringList>

#include <algorithm>
#include <utility>

namespace smartmate::viewmodel {

TaskEditorViewModel::TaskEditorViewModel(
    model::TaskService &taskService,
    TaskCategoryProjectionSource &categorySource,
    QObject *parent)
    : TaskEditorViewModel(taskService, categorySource,
                          QTimeZone::systemTimeZone(), parent)
{
}

TaskEditorViewModel::TaskEditorViewModel(model::TaskService &taskService,
                                         TaskCategoryProjectionSource &categorySource,
                                         QTimeZone timeZone,
                                         QObject *parent)
    : TaskEditorContract(parent)
    , m_taskService(taskService)
    , m_categorySource(categorySource)
    , m_priorityIndex(taskPriorityIndex(model::TaskPriority::Normal))
    , m_timeZone(std::move(timeZone))
{
    connect(&m_categorySource, &TaskCategoryProjectionSource::categoriesChanged,
            this, &TaskEditorViewModel::applyCategories);
    connect(&m_categorySource, &TaskCategoryProjectionSource::refreshFailed,
            this, [this] {
                setErrorMessage(QStringLiteral("类别数据访问失败，请稍后重试。"));
            });
    connect(&m_categorySource, &TaskCategoryProjectionSource::refreshSucceeded,
            this, [this] {
                if (m_errorMessage
                    == QStringLiteral("类别数据访问失败，请稍后重试。")) {
                    setErrorMessage({});
                }
            });
    applyCategories();
    rememberCurrentDraft();
    updateFormState();
}

int TaskEditorViewModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_predecessorCandidates.size());
}

QVariant TaskEditorViewModel::data(const QModelIndex &index, const int role) const
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= m_predecessorCandidates.size()) {
        return {};
    }

    const model::Task &candidate = m_predecessorCandidates.at(index.row());
    switch (role) {
    case CandidateTaskIdRole:
        return candidate.id().toString(QUuid::WithoutBraces);
    case CandidateShortIdRole:
        return candidate.id().toString(QUuid::WithoutBraces).left(8);
    case CandidateTitleRole:
        return candidate.title();
    case CandidateStatusTextRole:
        return taskStatusText(candidate.status());
    case CandidatePriorityTextRole:
        return taskPriorityText(candidate.priority());
    case CandidateCategoryNameRole: {
        if (!candidate.categoryId().has_value()) return QString{};
        const auto iterator = std::find_if(
            m_categorySource.categories().cbegin(),
            m_categorySource.categories().cend(), [&](const auto &category) {
                return category.id == *candidate.categoryId();
            });
        return iterator == m_categorySource.categories().cend() ? QString{} : iterator->name;
    }
    case CandidateCategoryAccentRole: {
        if (!candidate.categoryId().has_value()) return QString{};
        const auto iterator = std::find_if(
            m_categorySource.categories().cbegin(),
            m_categorySource.categories().cend(), [&](const auto &category) {
                return category.id == *candidate.categoryId();
            });
        return iterator == m_categorySource.categories().cend()
            ? QString{} : taskCategoryAccent(iterator->color);
    }
    case CandidateHasCategoryRole: {
        if (!candidate.categoryId().has_value()) return false;
        return std::any_of(m_categorySource.categories().cbegin(),
                           m_categorySource.categories().cend(),
                           [&](const auto &category) {
                               return category.id == *candidate.categoryId();
                           });
    }
    case CandidateSelectedRole:
        return (m_predecessorPickerActive ? m_pickerPredecessors
                                          : m_selectedCreationPredecessors)
            .contains(candidate.id());
    default:
        return {};
    }
}

QHash<int, QByteArray> TaskEditorViewModel::roleNames() const
{
    return {
        {CandidateTaskIdRole, "candidateTaskId"},
        {CandidateShortIdRole, "candidateShortId"},
        {CandidateTitleRole, "candidateTitle"},
        {CandidateStatusTextRole, "candidateStatusText"},
        {CandidatePriorityTextRole, "candidatePriorityText"},
        {CandidateCategoryNameRole, "candidateCategoryName"},
        {CandidateCategoryAccentRole, "candidateCategoryAccent"},
        {CandidateHasCategoryRole, "candidateHasCategory"},
        {CandidateSelectedRole, "candidateSelected"},
    };
}

QString TaskEditorViewModel::taskId() const
{
    return m_taskId;
}

bool TaskEditorViewModel::editMode() const noexcept
{
    return m_editMode;
}

bool TaskEditorViewModel::sessionActive() const noexcept
{
    return m_sessionActive;
}

QString TaskEditorViewModel::title() const
{
    return m_title;
}

void TaskEditorViewModel::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    // 字段通知先让 Widget 同步文本，随后 formStateChanged 更新校验和保存资格。
    emit titleChanged();
    setErrorMessage({});
    updateFormState();
}

QString TaskEditorViewModel::description() const
{
    return m_description;
}

void TaskEditorViewModel::setDescription(const QString &description)
{
    if (m_description == description) {
        return;
    }
    m_description = description;
    emit descriptionChanged();
    setErrorMessage({});
    updateFormState();
}

QString TaskEditorViewModel::currentStatusText() const
{
    return taskStatusText(m_currentStatus);
}

int TaskEditorViewModel::priorityIndex() const noexcept
{
    return m_priorityIndex;
}

void TaskEditorViewModel::setPriorityIndex(const int priorityIndex)
{
    if (m_priorityIndex == priorityIndex) {
        return;
    }
    m_priorityIndex = priorityIndex;
    emit priorityIndexChanged();
    setErrorMessage({});
    updateFormState();
}

bool TaskEditorViewModel::hasDeadline() const noexcept
{
    return m_deadline.has_value();
}

QString TaskEditorViewModel::deadlineDisplayText() const
{
    const auto deadline = displayedDeadline();
    return taskDateTimeText(deadline.value_or(QDateTime{}),
                            QStringLiteral("未设置"));
}

int TaskEditorViewModel::deadlineYear() const
{
    const auto deadline = displayedDeadline();
    return deadline.has_value() ? deadline->date().year() : 0;
}

int TaskEditorViewModel::deadlineMonth() const
{
    const auto deadline = displayedDeadline();
    return deadline.has_value() ? deadline->date().month() : 0;
}

int TaskEditorViewModel::deadlineDay() const
{
    const auto deadline = displayedDeadline();
    return deadline.has_value() ? deadline->date().day() : 0;
}

int TaskEditorViewModel::deadlineHour() const
{
    const auto deadline = displayedDeadline();
    return deadline.has_value() ? deadline->time().hour() : 0;
}

int TaskEditorViewModel::deadlineMinute() const
{
    const auto deadline = displayedDeadline();
    return deadline.has_value() ? deadline->time().minute() : 0;
}

bool TaskEditorViewModel::hasEstimatedDuration() const noexcept
{
    return m_estimatedMinutes.has_value();
}

QString TaskEditorViewModel::estimatedDurationDisplayText() const
{
    if (!m_estimatedMinutes.has_value()) {
        return QStringLiteral("未设置");
    }

    QStringList parts;
    if (estimatedDays() > 0) {
        parts.append(QStringLiteral("%1天").arg(estimatedDays()));
    }
    if (estimatedHours() > 0) {
        parts.append(QStringLiteral("%1小时").arg(estimatedHours()));
    }
    if (estimatedMinutePart() > 0) {
        parts.append(QStringLiteral("%1分钟").arg(estimatedMinutePart()));
    }
    return parts.join(QLatin1Char(' '));
}

int TaskEditorViewModel::estimatedDays() const noexcept
{
    return m_estimatedMinutes.value_or(0) / (24 * 60);
}

int TaskEditorViewModel::estimatedHours() const noexcept
{
    return (m_estimatedMinutes.value_or(0) % (24 * 60)) / 60;
}

int TaskEditorViewModel::estimatedMinutePart() const noexcept
{
    return m_estimatedMinutes.value_or(0) % 60;
}

int TaskEditorViewModel::minimumEstimatedMinutes() const noexcept
{
    return model::TaskConstraints::minimumEstimatedMinutes;
}

int TaskEditorViewModel::maximumEstimatedMinutes() const noexcept
{
    return model::TaskConstraints::maximumEstimatedMinutes;
}

QStringList TaskEditorViewModel::priorityOptions() const
{
    return taskPriorityOptions();
}

QVariantList TaskEditorViewModel::categoryOptions() const
{
    QVariantList options;
    options.reserve(m_categorySource.categories().size() + 1);
    options.append(QVariantMap{{QStringLiteral("categoryId"), QString{}},
                               {QStringLiteral("name"), QStringLiteral("未分类")},
                               {QStringLiteral("accent"), taskUncategorizedAccent()}});
    for (const auto &category : m_categorySource.categories()) {
        options.append(QVariantMap{
            {QStringLiteral("categoryId"), category.id.toString(QUuid::WithoutBraces)},
            {QStringLiteral("name"), category.name},
            {QStringLiteral("accent"), taskCategoryAccent(category.color)}});
    }
    return options;
}

QString TaskEditorViewModel::selectedCategoryId() const
{
    return m_categoryId.has_value()
        ? m_categoryId->toString(QUuid::WithoutBraces) : QString{};
}

void TaskEditorViewModel::setSelectedCategoryId(const QString &categoryId)
{
    const QString trimmed = categoryId.trimmed();
    std::optional<model::TaskCategoryId> selected;
    if (!trimmed.isEmpty()) {
        const model::TaskCategoryId id{QUuid::fromString(trimmed)};
        const bool exists = std::any_of(
            m_categorySource.categories().cbegin(),
            m_categorySource.categories().cend(),
            [&id](const auto &category) { return category.id == id; });
        if (id.isNull() || !exists) {
            setErrorMessage(QStringLiteral("所选类别不存在或已被删除。"));
            return;
        }
        selected = id;
    }
    if (m_categoryId == selected) return;
    m_categoryId = selected;
    emit categoryChanged();
    updateFormState();
    setErrorMessage({});
}

const model::TaskCategory *TaskEditorViewModel::selectedCategory() const
{
    if (!m_categoryId.has_value()) return nullptr;
    const auto iterator = std::find_if(
        m_categorySource.categories().cbegin(),
        m_categorySource.categories().cend(), [&](const auto &category) {
            return category.id == *m_categoryId;
        });
    return iterator == m_categorySource.categories().cend() ? nullptr : &*iterator;
}

QString TaskEditorViewModel::selectedCategoryName() const
{
    const auto *category = selectedCategory();
    return category ? category->name : QStringLiteral("未分类");
}

QString TaskEditorViewModel::selectedCategoryAccent() const
{
    const auto *category = selectedCategory();
    return category ? taskCategoryAccent(category->color)
                    : taskUncategorizedAccent();
}

bool TaskEditorViewModel::hasCategory() const noexcept
{
    return m_categoryId.has_value();
}

bool TaskEditorViewModel::dirty() const noexcept
{
    return m_dirty;
}

bool TaskEditorViewModel::canSave() const noexcept
{
    return m_canSave;
}

QString TaskEditorViewModel::validationMessage() const
{
    return m_validationMessage;
}

QString TaskEditorViewModel::errorMessage() const
{
    return m_errorMessage;
}

int TaskEditorViewModel::predecessorCandidateCount() const noexcept
{
    return static_cast<int>(m_predecessorCandidates.size());
}

int TaskEditorViewModel::selectedPredecessorCount() const noexcept
{
    return static_cast<int>((m_predecessorPickerActive
                                 ? m_pickerPredecessors
                                 : m_selectedCreationPredecessors)
                                .size());
}

QString TaskEditorViewModel::predecessorSummaryText() const
{
    const QSet<model::TaskId> &selection = m_predecessorPickerActive
        ? m_pickerPredecessors
        : m_selectedCreationPredecessors;
    if (selection.isEmpty()) {
        return QStringLiteral("未设置");
    }
    return QStringLiteral("已选择 %1 项").arg(selection.size());
}

bool TaskEditorViewModel::canConfigurePredecessors() const noexcept
{
    return !m_editMode;
}

} // namespace smartmate::viewmodel
