#include "FocusViewModel.h"

#include "FocusErrorMapper.h"
#include "services/FocusService.h"

#include <QDateTime>

#include <algorithm>

namespace smartmate::viewmodel {
namespace {

QString localDateTimeText(const QDateTime &utc, const QTimeZone &timeZone)
{
    return utc.isValid()
        ? utc.toTimeZone(timeZone).toString(QStringLiteral("yyyy/M/d HH:mm"))
        : QString{};
}

} // namespace

FocusViewModel::FocusViewModel(model::FocusService &focusService,
                               QObject *parent)
    : FocusViewModel(focusService,
                     [] { return QTimeZone::systemTimeZone(); },
                     defaultDisplayTickMs,
                     parent)
{
}

FocusViewModel::FocusViewModel(model::FocusService &focusService,
                               TimeZoneProvider timeZoneProvider,
                               const int displayTickMs,
                               QObject *parent)
    : FocusContract(parent)
    , m_focusService(focusService)
    , m_timeZoneProvider(std::move(timeZoneProvider))
    , m_historyModel(new FocusHistoryListModel(this))
    , m_invalidationTimer(new QTimer(this))
    , m_displayTimer(new QTimer(this))
{
    m_invalidationTimer->setObjectName(QStringLiteral("focusInvalidationTimer"));
    m_invalidationTimer->setSingleShot(true);
    m_invalidationTimer->setInterval(0);
    m_displayTimer->setObjectName(QStringLiteral("focusDisplayTimer"));
    m_displayTimer->setSingleShot(true);
    m_displayTimer->setInterval(std::max(1, displayTickMs));

    connect(m_invalidationTimer, &QTimer::timeout,
            this, &FocusViewModel::reload);
    connect(m_displayTimer, &QTimer::timeout,
            this, &FocusViewModel::refreshActiveClock);
    connect(&m_focusService, &model::FocusService::focusChanged,
            this, &FocusViewModel::scheduleInvalidationReload);
    connect(&m_focusService, &model::FocusService::historyChanged,
            this, &FocusViewModel::scheduleInvalidationReload);
    connect(&m_focusService,
            &model::FocusService::backgroundFailureRaised,
            this,
            [this](model::FocusError, const QString &) {
                handleBackgroundFailure();
            });
    connect(&m_focusService,
            &model::FocusService::backgroundFailureCleared,
            this,
            &FocusViewModel::clearBackgroundFailure);

    reload();
}

QAbstractItemModel *FocusViewModel::history() noexcept { return m_historyModel; }

FocusContract::PageState FocusViewModel::pageState() const noexcept
{
    if (m_activeRecord.has_value()) {
        return m_activeRecord->session.state == model::FocusSessionState::Running
            ? Running : Paused;
    }
    return m_startCandidate.has_value() ? ReadyToStart : NoInProgressTask;
}

QString FocusViewModel::taskId() const
{
    if (m_activeRecord) return stableId(m_activeRecord->session.taskId);
    return m_startCandidate ? stableId(m_startCandidate->taskId) : QString{};
}

QString FocusViewModel::sessionId() const
{
    return m_activeRecord
        ? stableId(m_activeRecord->session.sessionId) : QString{};
}

QString FocusViewModel::taskTitle() const
{
    if (m_activeRecord) return m_activeRecord->session.taskTitleSnapshot;
    return m_startCandidate ? m_startCandidate->taskTitle : QString{};
}

bool FocusViewModel::hasCategory() const noexcept
{
    if (m_activeRecord) {
        return m_activeRecord->session.categoryNameSnapshot.has_value();
    }
    return m_startCandidate
        && m_startCandidate->categoryName.has_value();
}

QString FocusViewModel::categoryName() const
{
    if (m_activeRecord) {
        return m_activeRecord->session.categoryNameSnapshot
            .value_or(QStringLiteral("未分类"));
    }
    return m_startCandidate
        ? m_startCandidate->categoryName.value_or(QStringLiteral("未分类"))
        : QStringLiteral("未分类");
}

FocusContract::CategoryColor FocusViewModel::categoryColor() const noexcept
{
    if (m_activeRecord
        && m_activeRecord->session.categoryColorSnapshot.has_value()) {
        return mapCategoryColor(*m_activeRecord->session.categoryColorSnapshot);
    }
    if (m_startCandidate && m_startCandidate->categoryColor.has_value()) {
        return mapCategoryColor(*m_startCandidate->categoryColor);
    }
    return Unclassified;
}

bool FocusViewModel::hasEstimatedMinutes() const noexcept
{
    if (m_activeRecord) {
        return m_activeRecord->session.estimatedMinutesSnapshot.has_value();
    }
    return m_startCandidate
        && m_startCandidate->estimatedMinutes.has_value();
}

int FocusViewModel::estimatedMinutes() const noexcept
{
    if (m_activeRecord) {
        return m_activeRecord->session.estimatedMinutesSnapshot.value_or(0);
    }
    return m_startCandidate
        ? m_startCandidate->estimatedMinutes.value_or(0) : 0;
}

QString FocusViewModel::estimatedText() const
{
    return hasEstimatedMinutes()
        ? QStringLiteral("预计 %1 分钟").arg(estimatedMinutes())
        : QStringLiteral("未设置预计用时");
}

QString FocusViewModel::startedAtText() const
{
    return m_activeRecord
        ? localDateTimeText(m_activeRecord->session.startedAtUtc,
                            currentTimeZone())
        : QString{};
}

qint64 FocusViewModel::elapsedMilliseconds() const noexcept
{
    return m_activeRecord ? m_activeRecord->focusedMilliseconds : 0;
}

QString FocusViewModel::elapsedText() const
{
    return formatDuration(elapsedMilliseconds());
}

QString FocusViewModel::stateText() const
{
    switch (pageState()) {
    case NoInProgressTask: return QStringLiteral("暂无进行中的任务");
    case ReadyToStart: return QStringLiteral("可以开始专注");
    case Running: return QStringLiteral("专注中");
    case Paused: return QStringLiteral("已暂停");
    }
    return {};
}

bool FocusViewModel::canStart() const noexcept { return m_availability.canStart; }
bool FocusViewModel::canPause() const noexcept { return m_availability.canPause; }
bool FocusViewModel::canResume() const noexcept { return m_availability.canResume; }
bool FocusViewModel::canComplete() const noexcept { return m_availability.canComplete; }
bool FocusViewModel::canAbandon() const noexcept { return m_availability.canAbandon; }
int FocusViewModel::historyCount() const noexcept { return m_historyModel->rowCount(); }
bool FocusViewModel::hasHistory() const noexcept { return historyCount() > 0; }

QString FocusViewModel::historyEmptyStateText() const
{
    return hasHistory()
        ? QString{} : QStringLiteral("完成一次专注后，这里会显示记录。");
}

QString FocusViewModel::emptyStateText() const
{
    return pageState() == NoInProgressTask
        ? QStringLiteral("请先在任务页开始一项任务。") : QString{};
}

bool FocusViewModel::hasStorageWarning() const noexcept
{
    return m_readFailureActive || m_backgroundFailureActive;
}

QString FocusViewModel::storageWarningText() const
{
    if (m_backgroundFailureActive) {
        return QStringLiteral("专注记录暂时无法保存，正在重试。");
    }
    if (m_readFailureActive) {
        return QStringLiteral("专注数据暂时无法读取，正在重试。");
    }
    return {};
}

bool FocusViewModel::startFocus(const QString &taskIdText)
{
    const auto id = parseStableId(taskIdText);
    if (!id.has_value()) {
        handleCommandFailure(model::FocusError::TaskNotFound);
        return false;
    }
    const model::FocusRecordResult result = m_focusService.startFocus(*id);
    if (!result.ok()) {
        handleCommandFailure(result.error);
        return false;
    }
    m_invalidationTimer->stop();
    reload();
    return true;
}

bool FocusViewModel::pauseFocus(const QString &sessionIdText)
{
    return executeSessionCommand(sessionIdText,
                                 &model::FocusService::pauseFocus);
}

bool FocusViewModel::resumeFocus(const QString &sessionIdText)
{
    return executeSessionCommand(sessionIdText,
                                 &model::FocusService::resumeFocus);
}

bool FocusViewModel::completeFocus(const QString &sessionIdText)
{
    return executeSessionCommand(sessionIdText,
                                 &model::FocusService::completeFocus);
}

bool FocusViewModel::abandonFocus(const QString &sessionIdText)
{
    return executeSessionCommand(sessionIdText,
                                 &model::FocusService::abandonFocus);
}

void FocusViewModel::reload()
{
    m_invalidationTimer->stop();
    const model::FocusSnapshotResult result = m_focusService.snapshot(50);
    if (!result.ok()) {
        handleReadFailure();
        scheduleDisplayTick();
        return;
    }
    clearReadFailure();
    const bool pageChanged = applyPageSnapshot(*result.value);
    const bool historyRowsChanged =
        m_historyModel->replaceRows(makeHistoryRows(
            result.value->recentCompletedRecords));
    if (pageChanged) emit focusChanged();
    if (historyRowsChanged) emit historyChanged();
    scheduleDisplayTick();
}

bool FocusViewModel::executeSessionCommand(const QString &sessionIdText,
                                           const RecordCommand command)
{
    const auto id = parseStableId(sessionIdText);
    if (!id.has_value()) {
        handleCommandFailure(model::FocusError::SessionNotFound);
        return false;
    }
    const model::FocusRecordResult result = (m_focusService.*command)(*id);
    if (!result.ok()) {
        handleCommandFailure(result.error);
        return false;
    }
    m_invalidationTimer->stop();
    reload();
    return true;
}

bool FocusViewModel::applyPageSnapshot(const model::FocusSnapshot &snapshot)
{
    if (m_startCandidate == snapshot.startCandidate
        && m_activeRecord == snapshot.activeRecord
        && m_availability == snapshot.availability) {
        return false;
    }
    m_startCandidate = snapshot.startCandidate;
    m_activeRecord = snapshot.activeRecord;
    m_availability = snapshot.availability;
    return true;
}

QList<FocusHistoryRow> FocusViewModel::makeHistoryRows(
    const QList<model::FocusRecord> &records) const
{
    const QTimeZone zone = currentTimeZone();
    QList<FocusHistoryRow> rows;
    rows.reserve(records.size());
    for (const model::FocusRecord &record : records) {
        const auto &session = record.session;
        const QString duration = formatDuration(record.focusedMilliseconds);
        const QString started = localDateTimeText(session.startedAtUtc, zone);
        const QString completed = session.endedAtUtc.has_value()
            ? localDateTimeText(*session.endedAtUtc, zone) : QString{};
        const QString category = session.categoryNameSnapshot
            .value_or(QStringLiteral("未分类"));
        const CategoryColor color = session.categoryColorSnapshot.has_value()
            ? mapCategoryColor(*session.categoryColorSnapshot) : Unclassified;
        rows.append({stableId(session.sessionId),
                     stableId(session.taskId),
                     session.taskTitleSnapshot,
                     record.focusedMilliseconds,
                     duration,
                     started,
                     completed,
                     category,
                     color,
                     QStringLiteral("%1\n%2 至 %3\n累计专注 %4")
                         .arg(session.taskTitleSnapshot, started, completed, duration),
                     QStringLiteral("%1，%2 完成，累计专注 %3，类别 %4。")
                         .arg(session.taskTitleSnapshot, completed, duration, category)});
    }
    return rows;
}

void FocusViewModel::refreshActiveClock()
{
    const model::FocusSnapshotResult result = m_focusService.snapshot(0);
    if (!result.ok()) {
        handleReadFailure();
        scheduleDisplayTick();
        return;
    }
    clearReadFailure();
    if (applyPageSnapshot(*result.value)) emit focusChanged();
    scheduleDisplayTick();
}

void FocusViewModel::scheduleInvalidationReload()
{
    if (!m_invalidationTimer->isActive()) m_invalidationTimer->start();
}

void FocusViewModel::scheduleDisplayTick()
{
    m_displayTimer->stop();
    if (pageState() == Running) m_displayTimer->start();
}

void FocusViewModel::handleCommandFailure(const model::FocusError error)
{
    if (error == model::FocusError::SessionNotFound
        || error == model::FocusError::StateConflict
        || error == model::FocusError::ActiveSessionExists
        || error == model::FocusError::TaskNotInProgress) {
        reload();
    }
    emit notificationRaised({common::UiSeverity::Error,
                             QStringLiteral("专注操作失败"),
                             focusErrorMessage(error)});
}

void FocusViewModel::handleReadFailure()
{
    const bool oldVisible = hasStorageWarning();
    const QString oldText = storageWarningText();
    if (!m_readFailureActive) {
        m_readFailureActive = true;
        emit notificationRaised({common::UiSeverity::Warning,
                                 QStringLiteral("专注加载失败"),
                                 QStringLiteral("暂时无法读取专注数据，正在重试。")});
    }
    emitStorageWarningIfChanged(oldVisible, oldText);
}

void FocusViewModel::clearReadFailure()
{
    if (!m_readFailureActive) return;
    const bool oldVisible = hasStorageWarning();
    const QString oldText = storageWarningText();
    m_readFailureActive = false;
    emitStorageWarningIfChanged(oldVisible, oldText);
    emit notificationRaised({common::UiSeverity::Information,
                             QStringLiteral("专注读取已恢复"),
                             QStringLiteral("专注数据已经可以正常读取。")});
}

void FocusViewModel::handleBackgroundFailure()
{
    const bool oldVisible = hasStorageWarning();
    const QString oldText = storageWarningText();
    if (!m_backgroundFailureActive) {
        m_backgroundFailureActive = true;
        emit notificationRaised({common::UiSeverity::Warning,
                                 QStringLiteral("专注保存失败"),
                                 QStringLiteral("专注记录暂时无法保存，正在重试。")});
    }
    emitStorageWarningIfChanged(oldVisible, oldText);
}

void FocusViewModel::clearBackgroundFailure()
{
    if (!m_backgroundFailureActive) return;
    const bool oldVisible = hasStorageWarning();
    const QString oldText = storageWarningText();
    m_backgroundFailureActive = false;
    emitStorageWarningIfChanged(oldVisible, oldText);
    emit notificationRaised({common::UiSeverity::Information,
                             QStringLiteral("专注保存已恢复"),
                             QStringLiteral("专注记录已经恢复正常保存。")});
}

void FocusViewModel::emitStorageWarningIfChanged(const bool oldVisible,
                                                 const QString &oldText)
{
    if (oldVisible != hasStorageWarning() || oldText != storageWarningText()) {
        emit storageWarningChanged();
    }
}

QTimeZone FocusViewModel::currentTimeZone() const
{
    const QTimeZone zone = m_timeZoneProvider
        ? m_timeZoneProvider() : QTimeZone{};
    return zone.isValid() ? zone : QTimeZone{QTimeZone::UTC};
}

QString FocusViewModel::stableId(const QUuid &id)
{
    return id.toString(QUuid::WithoutBraces);
}

std::optional<QUuid> FocusViewModel::parseStableId(const QString &id)
{
    const QUuid parsed{id};
    return parsed.isNull() ? std::nullopt : std::optional{parsed};
}

QString FocusViewModel::formatDuration(const qint64 milliseconds)
{
    const qint64 totalSeconds = std::max<qint64>(0, milliseconds) / 1'000;
    const qint64 hours = totalSeconds / 3'600;
    const qint64 minutes = (totalSeconds / 60) % 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(hours)
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

FocusContract::CategoryColor FocusViewModel::mapCategoryColor(
    const model::TaskCategoryColor color) noexcept
{
    using model::TaskCategoryColor;
    switch (color) {
    case TaskCategoryColor::Blue: return Blue;
    case TaskCategoryColor::Teal: return Teal;
    case TaskCategoryColor::Green: return Green;
    case TaskCategoryColor::Amber: return Amber;
    case TaskCategoryColor::Orange: return Orange;
    case TaskCategoryColor::Rose: return Rose;
    case TaskCategoryColor::Violet: return Violet;
    case TaskCategoryColor::Slate: return Slate;
    }
    return Unclassified;
}

} // namespace smartmate::viewmodel
