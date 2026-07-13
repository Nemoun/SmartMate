#pragma once

#include "common/presentation/UiNotification.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>

namespace smartmate::viewmodel {

/// 任务创建和编辑草稿的抽象展示契约。
class TaskEditorContract : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString taskId READ taskId NOTIFY modeChanged)
    Q_PROPERTY(bool editMode READ editMode NOTIFY modeChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QString currentStatusText READ currentStatusText NOTIFY currentStatusTextChanged)
    Q_PROPERTY(int priorityIndex READ priorityIndex WRITE setPriorityIndex NOTIFY priorityIndexChanged)
    Q_PROPERTY(bool hasDeadline READ hasDeadline NOTIFY deadlineChanged)
    Q_PROPERTY(QString deadlineDisplayText READ deadlineDisplayText NOTIFY deadlineChanged)
    Q_PROPERTY(int deadlineYear READ deadlineYear NOTIFY deadlineChanged)
    Q_PROPERTY(int deadlineMonth READ deadlineMonth NOTIFY deadlineChanged)
    Q_PROPERTY(int deadlineDay READ deadlineDay NOTIFY deadlineChanged)
    Q_PROPERTY(int deadlineHour READ deadlineHour NOTIFY deadlineChanged)
    Q_PROPERTY(int deadlineMinute READ deadlineMinute NOTIFY deadlineChanged)
    Q_PROPERTY(bool hasEstimatedDuration READ hasEstimatedDuration NOTIFY estimatedDurationChanged)
    Q_PROPERTY(QString estimatedDurationDisplayText READ estimatedDurationDisplayText
                   NOTIFY estimatedDurationChanged)
    Q_PROPERTY(int estimatedDays READ estimatedDays NOTIFY estimatedDurationChanged)
    Q_PROPERTY(int estimatedHours READ estimatedHours NOTIFY estimatedDurationChanged)
    Q_PROPERTY(int estimatedMinutePart READ estimatedMinutePart NOTIFY estimatedDurationChanged)
    Q_PROPERTY(int minimumEstimatedMinutes READ minimumEstimatedMinutes CONSTANT)
    Q_PROPERTY(int maximumEstimatedMinutes READ maximumEstimatedMinutes CONSTANT)
    Q_PROPERTY(QStringList priorityOptions READ priorityOptions CONSTANT)
    Q_PROPERTY(QVariantList categoryOptions READ categoryOptions NOTIFY categoryOptionsChanged)
    Q_PROPERTY(QString selectedCategoryId READ selectedCategoryId WRITE setSelectedCategoryId
                   NOTIFY categoryChanged)
    Q_PROPERTY(QString selectedCategoryName READ selectedCategoryName NOTIFY categoryChanged)
    Q_PROPERTY(QString selectedCategoryAccent READ selectedCategoryAccent NOTIFY categoryChanged)
    Q_PROPERTY(bool hasCategory READ hasCategory NOTIFY categoryChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY formStateChanged)
    Q_PROPERTY(bool canSave READ canSave NOTIFY formStateChanged)
    Q_PROPERTY(QString validationMessage READ validationMessage NOTIFY formStateChanged)
    Q_PROPERTY(int predecessorCandidateCount READ predecessorCandidateCount
                   NOTIFY predecessorCandidatesChanged)
    Q_PROPERTY(int selectedPredecessorCount READ selectedPredecessorCount
                   NOTIFY predecessorSelectionChanged)
    Q_PROPERTY(QString predecessorSummaryText READ predecessorSummaryText
                   NOTIFY predecessorSelectionChanged)
    Q_PROPERTY(bool canConfigurePredecessors READ canConfigurePredecessors NOTIFY modeChanged)

public:
    enum Role {
        CandidateTaskIdRole = Qt::UserRole + 1,
        CandidateShortIdRole,
        CandidateTitleRole,
        CandidateStatusTextRole,
        CandidatePriorityTextRole,
        CandidateCategoryNameRole,
        CandidateCategoryAccentRole,
        CandidateHasCategoryRole,
        CandidateSelectedRole,
    };
    Q_ENUM(Role)

    ~TaskEditorContract() override = default;

    [[nodiscard]] virtual QString taskId() const = 0;
    [[nodiscard]] virtual bool editMode() const noexcept = 0;
    [[nodiscard]] virtual QString title() const = 0;
    virtual void setTitle(const QString &title) = 0;
    [[nodiscard]] virtual QString description() const = 0;
    virtual void setDescription(const QString &description) = 0;
    [[nodiscard]] virtual QString currentStatusText() const = 0;
    [[nodiscard]] virtual int priorityIndex() const noexcept = 0;
    virtual void setPriorityIndex(int priorityIndex) = 0;
    [[nodiscard]] virtual bool hasDeadline() const noexcept = 0;
    [[nodiscard]] virtual QString deadlineDisplayText() const = 0;
    [[nodiscard]] virtual int deadlineYear() const = 0;
    [[nodiscard]] virtual int deadlineMonth() const = 0;
    [[nodiscard]] virtual int deadlineDay() const = 0;
    [[nodiscard]] virtual int deadlineHour() const = 0;
    [[nodiscard]] virtual int deadlineMinute() const = 0;
    [[nodiscard]] virtual bool hasEstimatedDuration() const noexcept = 0;
    [[nodiscard]] virtual QString estimatedDurationDisplayText() const = 0;
    [[nodiscard]] virtual int estimatedDays() const noexcept = 0;
    [[nodiscard]] virtual int estimatedHours() const noexcept = 0;
    [[nodiscard]] virtual int estimatedMinutePart() const noexcept = 0;
    [[nodiscard]] virtual int minimumEstimatedMinutes() const noexcept = 0;
    [[nodiscard]] virtual int maximumEstimatedMinutes() const noexcept = 0;
    [[nodiscard]] virtual QStringList priorityOptions() const = 0;
    [[nodiscard]] virtual QVariantList categoryOptions() const = 0;
    [[nodiscard]] virtual QString selectedCategoryId() const = 0;
    virtual void setSelectedCategoryId(const QString &categoryId) = 0;
    [[nodiscard]] virtual QString selectedCategoryName() const = 0;
    [[nodiscard]] virtual QString selectedCategoryAccent() const = 0;
    [[nodiscard]] virtual bool hasCategory() const noexcept = 0;
    [[nodiscard]] virtual bool dirty() const noexcept = 0;
    [[nodiscard]] virtual bool canSave() const noexcept = 0;
    [[nodiscard]] virtual QString validationMessage() const = 0;
    [[nodiscard]] virtual int predecessorCandidateCount() const noexcept = 0;
    [[nodiscard]] virtual int selectedPredecessorCount() const noexcept = 0;
    [[nodiscard]] virtual QString predecessorSummaryText() const = 0;
    [[nodiscard]] virtual bool canConfigurePredecessors() const noexcept = 0;

public slots:
    virtual bool beginCreate() = 0;
    virtual bool beginEdit(const QString &taskId) = 0;
    virtual bool setDeadlineSelection(int year, int month, int day, int hour, int minute) = 0;
    virtual void clearDeadline() = 0;
    virtual bool setEstimatedDuration(int days, int hours, int minutes) = 0;
    virtual void clearEstimatedDuration() = 0;
    virtual void beginPredecessorSelection() = 0;
    virtual bool setCreationPredecessorSelected(const QString &taskId, bool selected) = 0;
    virtual void acceptPredecessorSelection() = 0;
    virtual void cancelPredecessorSelection() = 0;
    virtual void clearCreationPredecessors() = 0;
    virtual bool save() = 0;
    virtual void cancel() = 0;

signals:
    void modeChanged();
    void titleChanged();
    void descriptionChanged();
    void currentStatusTextChanged();
    void priorityIndexChanged();
    void deadlineChanged();
    void estimatedDurationChanged();
    void formStateChanged();
    void predecessorCandidatesChanged();
    void predecessorSelectionChanged();
    void categoryOptionsChanged();
    void categoryChanged();
    void saved(const QString &taskId);
    void cancelled();
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    explicit TaskEditorContract(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
};

} // namespace smartmate::viewmodel
