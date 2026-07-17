#pragma once

#include "common/presentation/UiNotification.h"

#include <QAbstractListModel>
#include <QObject>
#include <QString>

namespace smartmate::viewmodel {

/// 最近完成专注的只读列表契约；Role 只包含稳定身份和展示投影。
class FocusHistoryContract : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        SessionIdRole = Qt::UserRole + 1,
        TaskIdRole,
        TaskTitleRole,
        DurationMillisecondsRole,
        DurationTextRole,
        StartedAtTextRole,
        CompletedAtTextRole,
        CategoryNameRole,
        CategoryColorRole,
        TooltipRole,
        AccessibleTextRole,
    };
    Q_ENUM(Role)

    ~FocusHistoryContract() override = default;

protected:
    explicit FocusHistoryContract(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
};

/// 专注页面级抽象契约；只暴露展示状态、Model 资格和稳定 ID 命令。
class FocusContract : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *history READ history CONSTANT)
    Q_PROPERTY(PageState pageState READ pageState NOTIFY focusChanged)
    Q_PROPERTY(QString taskId READ taskId NOTIFY focusChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY focusChanged)
    Q_PROPERTY(QString taskTitle READ taskTitle NOTIFY focusChanged)
    Q_PROPERTY(bool hasCategory READ hasCategory NOTIFY focusChanged)
    Q_PROPERTY(QString categoryName READ categoryName NOTIFY focusChanged)
    Q_PROPERTY(CategoryColor categoryColor READ categoryColor NOTIFY focusChanged)
    Q_PROPERTY(bool hasEstimatedMinutes READ hasEstimatedMinutes NOTIFY focusChanged)
    Q_PROPERTY(int estimatedMinutes READ estimatedMinutes NOTIFY focusChanged)
    Q_PROPERTY(QString estimatedText READ estimatedText NOTIFY focusChanged)
    Q_PROPERTY(QString startedAtText READ startedAtText NOTIFY focusChanged)
    Q_PROPERTY(qint64 elapsedMilliseconds READ elapsedMilliseconds NOTIFY focusChanged)
    Q_PROPERTY(QString elapsedText READ elapsedText NOTIFY focusChanged)
    Q_PROPERTY(QString stateText READ stateText NOTIFY focusChanged)
    Q_PROPERTY(bool canStart READ canStart NOTIFY focusChanged)
    Q_PROPERTY(bool canPause READ canPause NOTIFY focusChanged)
    Q_PROPERTY(bool canResume READ canResume NOTIFY focusChanged)
    Q_PROPERTY(bool canComplete READ canComplete NOTIFY focusChanged)
    Q_PROPERTY(bool canAbandon READ canAbandon NOTIFY focusChanged)
    Q_PROPERTY(int historyCount READ historyCount NOTIFY historyChanged)
    Q_PROPERTY(bool hasHistory READ hasHistory NOTIFY historyChanged)
    Q_PROPERTY(QString historyEmptyStateText READ historyEmptyStateText
                   NOTIFY historyChanged)
    Q_PROPERTY(QString emptyStateText READ emptyStateText NOTIFY focusChanged)
    Q_PROPERTY(bool hasStorageWarning READ hasStorageWarning
                   NOTIFY storageWarningChanged)
    Q_PROPERTY(QString storageWarningText READ storageWarningText
                   NOTIFY storageWarningChanged)

public:
    enum PageState {
        NoInProgressTask = 0,
        ReadyToStart,
        Running,
        Paused,
    };
    Q_ENUM(PageState)

    /// 与既有任务类别调色板保持一致；Unclassified 使用稳定中性色。
    enum CategoryColor {
        Blue = 0,
        Teal,
        Green,
        Amber,
        Orange,
        Rose,
        Violet,
        Slate,
        Unclassified,
    };
    Q_ENUM(CategoryColor)

    ~FocusContract() override = default;

    [[nodiscard]] virtual QAbstractItemModel *history() noexcept = 0;
    [[nodiscard]] virtual PageState pageState() const noexcept = 0;
    [[nodiscard]] virtual QString taskId() const = 0;
    [[nodiscard]] virtual QString sessionId() const = 0;
    [[nodiscard]] virtual QString taskTitle() const = 0;
    [[nodiscard]] virtual bool hasCategory() const noexcept = 0;
    [[nodiscard]] virtual QString categoryName() const = 0;
    [[nodiscard]] virtual CategoryColor categoryColor() const noexcept = 0;
    [[nodiscard]] virtual bool hasEstimatedMinutes() const noexcept = 0;
    [[nodiscard]] virtual int estimatedMinutes() const noexcept = 0;
    [[nodiscard]] virtual QString estimatedText() const = 0;
    [[nodiscard]] virtual QString startedAtText() const = 0;
    [[nodiscard]] virtual qint64 elapsedMilliseconds() const noexcept = 0;
    [[nodiscard]] virtual QString elapsedText() const = 0;
    [[nodiscard]] virtual QString stateText() const = 0;
    [[nodiscard]] virtual bool canStart() const noexcept = 0;
    [[nodiscard]] virtual bool canPause() const noexcept = 0;
    [[nodiscard]] virtual bool canResume() const noexcept = 0;
    [[nodiscard]] virtual bool canComplete() const noexcept = 0;
    [[nodiscard]] virtual bool canAbandon() const noexcept = 0;
    [[nodiscard]] virtual int historyCount() const noexcept = 0;
    [[nodiscard]] virtual bool hasHistory() const noexcept = 0;
    [[nodiscard]] virtual QString historyEmptyStateText() const = 0;
    [[nodiscard]] virtual QString emptyStateText() const = 0;
    [[nodiscard]] virtual bool hasStorageWarning() const noexcept = 0;
    [[nodiscard]] virtual QString storageWarningText() const = 0;

public slots:
    virtual bool startFocus(const QString &taskId) = 0;
    virtual bool pauseFocus(const QString &sessionId) = 0;
    virtual bool resumeFocus(const QString &sessionId) = 0;
    virtual bool completeFocus(const QString &sessionId) = 0;
    virtual bool abandonFocus(const QString &sessionId) = 0;
    virtual void reload() = 0;

signals:
    void focusChanged();
    void historyChanged();
    void storageWarningChanged();
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    explicit FocusContract(QObject *parent = nullptr) : QObject(parent) {}
};

} // namespace smartmate::viewmodel
