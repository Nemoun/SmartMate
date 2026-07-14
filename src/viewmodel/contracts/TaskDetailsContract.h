#pragma once

#include "common/presentation/UiNotification.h"

#include <QObject>
#include <QString>

namespace smartmate::viewmodel {

/// 稳定 TaskId 驱动的只读详情会话契约。
class TaskDetailsContract : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString selectedTaskId READ selectedTaskId NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDescription READ selectedDescription NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedStatusText READ selectedStatusText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedPriorityText READ selectedPriorityText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDeadlineText READ selectedDeadlineText NOTIFY selectionChanged)
    Q_PROPERTY(int selectedEstimatedMinutes READ selectedEstimatedMinutes NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCreatedAtText READ selectedCreatedAtText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedUpdatedAtText READ selectedUpdatedAtText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedReasonText READ selectedReasonText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedBlockingReasonText READ selectedBlockingReasonText NOTIFY selectionChanged)
    Q_PROPERTY(int selectedPredecessorCount READ selectedPredecessorCount NOTIFY selectionChanged)
    Q_PROPERTY(int selectedUnlockCount READ selectedUnlockCount NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedCanEditTask READ selectedCanEditTask NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedCanEditDependencies READ selectedCanEditDependencies NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCategoryName READ selectedCategoryName NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCategoryAccent READ selectedCategoryAccent NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedHasCategory READ selectedHasCategory NOTIFY selectionChanged)

public:
    ~TaskDetailsContract() override = default;

    [[nodiscard]] virtual QString selectedTaskId() const = 0;
    [[nodiscard]] virtual QString selectedTitle() const = 0;
    [[nodiscard]] virtual QString selectedDescription() const = 0;
    [[nodiscard]] virtual QString selectedStatusText() const = 0;
    [[nodiscard]] virtual QString selectedPriorityText() const = 0;
    [[nodiscard]] virtual QString selectedDeadlineText() const = 0;
    [[nodiscard]] virtual int selectedEstimatedMinutes() const noexcept = 0;
    [[nodiscard]] virtual QString selectedCreatedAtText() const = 0;
    [[nodiscard]] virtual QString selectedUpdatedAtText() const = 0;
    [[nodiscard]] virtual QString selectedReasonText() const = 0;
    [[nodiscard]] virtual QString selectedBlockingReasonText() const = 0;
    [[nodiscard]] virtual int selectedPredecessorCount() const noexcept = 0;
    [[nodiscard]] virtual int selectedUnlockCount() const noexcept = 0;
    [[nodiscard]] virtual bool selectedCanEditTask() const noexcept = 0;
    [[nodiscard]] virtual bool selectedCanEditDependencies() const noexcept = 0;
    [[nodiscard]] virtual QString selectedCategoryName() const = 0;
    [[nodiscard]] virtual QString selectedCategoryAccent() const = 0;
    [[nodiscard]] virtual bool selectedHasCategory() const noexcept = 0;

public slots:
    virtual bool selectTask(const QString &taskId) = 0;
    virtual void clearSelection() = 0;

signals:
    void selectionChanged();
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    explicit TaskDetailsContract(QObject *parent = nullptr) : QObject(parent) {}
};

} // namespace smartmate::viewmodel
