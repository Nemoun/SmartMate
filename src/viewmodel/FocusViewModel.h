#pragma once

#include "FocusProjectionModels.h"
#include "services/FocusResult.h"
#include "viewmodel/contracts/FocusContract.h"

#include <QTimeZone>
#include <QTimer>

#include <functional>
#include <optional>

namespace smartmate::model {
class FocusService;
}

namespace smartmate::viewmodel {

/// 将 FocusService 权威快照投影为秒级页面状态；不重新累计时间或推导资格。
class FocusViewModel final : public FocusContract {
    Q_OBJECT

public:
    using TimeZoneProvider = std::function<QTimeZone()>;
    static constexpr int defaultDisplayTickMs = 1'000;

    explicit FocusViewModel(model::FocusService &focusService,
                            QObject *parent = nullptr);
    /// 可注入时区和刷新周期的构造仅用于确定性测试。
    FocusViewModel(model::FocusService &focusService,
                   TimeZoneProvider timeZoneProvider,
                   int displayTickMs,
                   QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *history() noexcept override;
    [[nodiscard]] PageState pageState() const noexcept override;
    [[nodiscard]] QString taskId() const override;
    [[nodiscard]] QString sessionId() const override;
    [[nodiscard]] QString taskTitle() const override;
    [[nodiscard]] bool hasCategory() const noexcept override;
    [[nodiscard]] QString categoryName() const override;
    [[nodiscard]] CategoryColor categoryColor() const noexcept override;
    [[nodiscard]] bool hasEstimatedMinutes() const noexcept override;
    [[nodiscard]] int estimatedMinutes() const noexcept override;
    [[nodiscard]] QString estimatedText() const override;
    [[nodiscard]] QString startedAtText() const override;
    [[nodiscard]] qint64 elapsedMilliseconds() const noexcept override;
    [[nodiscard]] QString elapsedText() const override;
    [[nodiscard]] QString stateText() const override;
    [[nodiscard]] bool canStart() const noexcept override;
    [[nodiscard]] bool canPause() const noexcept override;
    [[nodiscard]] bool canResume() const noexcept override;
    [[nodiscard]] bool canComplete() const noexcept override;
    [[nodiscard]] bool canAbandon() const noexcept override;
    [[nodiscard]] int historyCount() const noexcept override;
    [[nodiscard]] bool hasHistory() const noexcept override;
    [[nodiscard]] QString historyEmptyStateText() const override;
    [[nodiscard]] QString emptyStateText() const override;
    [[nodiscard]] bool hasStorageWarning() const noexcept override;
    [[nodiscard]] QString storageWarningText() const override;

public slots:
    bool startFocus(const QString &taskId) override;
    bool pauseFocus(const QString &sessionId) override;
    bool resumeFocus(const QString &sessionId) override;
    bool completeFocus(const QString &sessionId) override;
    bool abandonFocus(const QString &sessionId) override;
    void reload() override;

private:
    using RecordCommand = model::FocusRecordResult
        (model::FocusService::*)(const model::FocusSessionId &);

    [[nodiscard]] bool executeSessionCommand(const QString &sessionId,
                                             RecordCommand command);
    [[nodiscard]] bool applyPageSnapshot(const model::FocusSnapshot &snapshot);
    [[nodiscard]] QList<FocusHistoryRow> makeHistoryRows(
        const QList<model::FocusRecord> &records) const;
    void refreshActiveClock();
    void scheduleInvalidationReload();
    void scheduleDisplayTick();
    void handleCommandFailure(model::FocusError error);
    void handleReadFailure();
    void clearReadFailure();
    void handleBackgroundFailure();
    void clearBackgroundFailure();
    void emitStorageWarningIfChanged(bool oldVisible, const QString &oldText);

    [[nodiscard]] QTimeZone currentTimeZone() const;
    [[nodiscard]] static QString stableId(const QUuid &id);
    [[nodiscard]] static std::optional<QUuid> parseStableId(const QString &id);
    [[nodiscard]] static QString formatDuration(qint64 milliseconds);
    [[nodiscard]] static CategoryColor mapCategoryColor(
        model::TaskCategoryColor color) noexcept;

    model::FocusService &m_focusService;
    TimeZoneProvider m_timeZoneProvider;
    FocusHistoryListModel *m_historyModel{nullptr};
    QTimer *m_invalidationTimer{nullptr};
    QTimer *m_displayTimer{nullptr};
    std::optional<model::FocusStartCandidate> m_startCandidate;
    std::optional<model::FocusRecord> m_activeRecord;
    model::FocusCommandAvailability m_availability;
    bool m_readFailureActive{false};
    bool m_backgroundFailureActive{false};
};

} // namespace smartmate::viewmodel
