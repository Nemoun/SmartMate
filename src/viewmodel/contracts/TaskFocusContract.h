#pragma once

#include "common/presentation/UiNotification.h"

#include <QObject>
#include <QString>

namespace smartmate::viewmodel {

/// “现在做”区域的抽象展示契约；只提供 Model 计划结果的焦点投影。
class TaskFocusContract : public QObject {
    Q_OBJECT
    Q_PROPERTY(FocusState focusState READ focusState NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusTaskId READ focusTaskId NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusTitle READ focusTitle NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusDescription READ focusDescription NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusStatusText READ focusStatusText NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusPriorityText READ focusPriorityText NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusDeadlineText READ focusDeadlineText NOTIFY focusTaskChanged)
    Q_PROPERTY(int focusEstimatedMinutes READ focusEstimatedMinutes NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusReasonText READ focusReasonText NOTIFY focusTaskChanged)
    Q_PROPERTY(bool focusOverdue READ focusOverdue NOTIFY focusTaskChanged)
    Q_PROPERTY(bool focusCanStart READ focusCanStart NOTIFY focusTaskChanged)
    Q_PROPERTY(bool focusCanComplete READ focusCanComplete NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusCategoryName READ focusCategoryName NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusCategoryAccent READ focusCategoryAccent NOTIFY focusTaskChanged)
    Q_PROPERTY(bool focusHasCategory READ focusHasCategory NOTIFY focusTaskChanged)

public:
    enum class FocusState {
        NoTasks = 0,
        Suggested = 1,
        InProgress = 2,
        AllBlocked = 3,
    };
    Q_ENUM(FocusState)

    ~TaskFocusContract() override = default;

    [[nodiscard]] virtual FocusState focusState() const noexcept = 0;
    [[nodiscard]] virtual QString focusTaskId() const = 0;
    [[nodiscard]] virtual QString focusTitle() const = 0;
    [[nodiscard]] virtual QString focusDescription() const = 0;
    [[nodiscard]] virtual QString focusStatusText() const = 0;
    [[nodiscard]] virtual QString focusPriorityText() const = 0;
    [[nodiscard]] virtual QString focusDeadlineText() const = 0;
    [[nodiscard]] virtual int focusEstimatedMinutes() const noexcept = 0;
    [[nodiscard]] virtual QString focusReasonText() const = 0;
    [[nodiscard]] virtual bool focusOverdue() const noexcept = 0;
    [[nodiscard]] virtual bool focusCanStart() const noexcept = 0;
    [[nodiscard]] virtual bool focusCanComplete() const noexcept = 0;
    [[nodiscard]] virtual QString focusCategoryName() const = 0;
    [[nodiscard]] virtual QString focusCategoryAccent() const = 0;
    [[nodiscard]] virtual bool focusHasCategory() const noexcept = 0;

signals:
    void focusTaskChanged();
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    explicit TaskFocusContract(QObject *parent = nullptr) : QObject(parent) {}
};

} // namespace smartmate::viewmodel
