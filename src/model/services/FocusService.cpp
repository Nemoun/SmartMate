#include "services/FocusService.h"

#include "repositories/RepositoryException.h"

#include <QDateTime>

#include <algorithm>
#include <limits>

namespace smartmate::model {
namespace {

FocusOperationResult persistenceFailure(const RepositoryException &exception)
{
    return FocusOperationResult::failure(
        FocusError::PersistenceFailure, QString::fromUtf8(exception.what()));
}

FocusOperationResult unexpectedPersistenceFailure()
{
    return FocusOperationResult::failure(
        FocusError::PersistenceFailure,
        QStringLiteral("Unexpected focus persistence failure."));
}

template<typename T>
FocusValueResult<T> persistenceValueFailure(const RepositoryException &exception)
{
    return FocusValueResult<T>::failure(
        FocusError::PersistenceFailure, QString::fromUtf8(exception.what()));
}

template<typename T>
FocusValueResult<T> unexpectedValueFailure()
{
    return FocusValueResult<T>::failure(
        FocusError::PersistenceFailure,
        QStringLiteral("Unexpected focus persistence failure."));
}

} // namespace

FocusService::FocusService(ITaskRepository &taskRepository,
                           ITaskCategoryRepository &categoryRepository,
                           ITaskActivityRepository &activityRepository,
                           IFocusSessionRepository &focusRepository,
                           QObject *parent)
    : FocusService(taskRepository,
                   categoryRepository,
                   activityRepository,
                   focusRepository,
                   [] { return QDateTime::currentDateTimeUtc(); },
                   defaultCheckpointIntervalMs,
                   parent)
{
}

FocusService::FocusService(ITaskRepository &taskRepository,
                           ITaskCategoryRepository &categoryRepository,
                           ITaskActivityRepository &activityRepository,
                           IFocusSessionRepository &focusRepository,
                           NowProvider nowProvider,
                           const int checkpointIntervalMs,
                           QObject *parent)
    : QObject(parent)
    , m_taskRepository(taskRepository)
    , m_categoryRepository(categoryRepository)
    , m_activityRepository(activityRepository)
    , m_focusRepository(focusRepository)
    , m_nowProvider(std::move(nowProvider))
{
    m_checkpointTimer.setInterval(std::max(1, checkpointIntervalMs));
    m_checkpointTimer.setSingleShot(false);
    connect(&m_checkpointTimer,
            &QTimer::timeout,
            this,
            &FocusService::handleCheckpointTimeout);
}

FocusOperationResult FocusService::initialize()
{
    if (m_initialized) return FocusOperationResult::success();
    try {
        bool changed = false;
        auto active = m_focusRepository.findActiveFocusSession();
        if (active.has_value() && active->state == FocusSessionState::Running) {
            const FocusSessionWriteResult recovered =
                m_focusRepository.recoverRunningFocusSessionAtomically();
            if (!recovered.ok()) {
                return FocusOperationResult::failure(
                    mapWriteStatus(recovered.status),
                    QStringLiteral("Unable to recover the interrupted focus session."));
            }
            changed = true;
            active = m_focusRepository.findActiveFocusSession();
        }

        const QDateTime currentUtc = nowUtc();
        if (!currentUtc.isValid()) {
            return FocusOperationResult::failure(
                FocusError::StateConflict,
                QStringLiteral("Focus clock returned an invalid UTC time."));
        }
        if (active.has_value()) {
            const FocusRecord record = loadRecord(*active, currentUtc);
            if (record.focusedMilliseconds < minimumCompletedDurationMs) {
                const FocusSessionWriteResult abandoned =
                    m_focusRepository.abandonFocusSessionAtomically(
                        active->sessionId, active->state);
                if (!abandoned.ok()) {
                    return FocusOperationResult::failure(
                        mapWriteStatus(abandoned.status),
                        QStringLiteral("Unable to discard a short recovered focus session."));
                }
                changed = true;
                active.reset();
            }
        }

        m_initialized = true;
        m_shutdownPrepared = false;
        if (active.has_value() && active->state == FocusSessionState::Running) {
            startCheckpointTimer(active->sessionId);
        }
        if (changed) emit focusChanged();
        return FocusOperationResult::success();
    } catch (const RepositoryException &exception) {
        return persistenceFailure(exception);
    } catch (...) {
        return unexpectedPersistenceFailure();
    }
}

FocusSnapshotResult FocusService::snapshot(const int limit) const
{
    if (const auto ready = requireInitialized(); !ready.ok()) {
        return FocusSnapshotResult::failure(ready.error, ready.detail);
    }
    try {
        const QDateTime currentUtc = nowUtc();
        if (!currentUtc.isValid()) {
            return FocusSnapshotResult::failure(
                FocusError::StateConflict,
                QStringLiteral("Focus clock returned an invalid UTC time."));
        }
        FocusSnapshot result;
        const QList<Task> tasks = m_taskRepository.findAll();
        const auto inProgressCount = std::count_if(
            tasks.cbegin(), tasks.cend(), [](const Task &task) {
                return task.status() == TaskStatus::InProgress;
            });
        if (inProgressCount > 1) {
            return FocusSnapshotResult::failure(
                FocusError::StateConflict,
                QStringLiteral("Multiple in-progress tasks prevent focus selection."));
        }
        if (const auto active = m_focusRepository.findActiveFocusSession()) {
            result.activeRecord = loadRecord(*active, currentUtc);
            result.availability.canPause =
                active->state == FocusSessionState::Running;
            result.availability.canResume =
                active->state == FocusSessionState::Paused;
            result.availability.canComplete =
                result.activeRecord->focusedMilliseconds
                >= minimumCompletedDurationMs;
            result.availability.canAbandon =
                active->state == FocusSessionState::Running
                || active->state == FocusSessionState::Paused;
        } else {
            const auto candidate = std::find_if(
                tasks.cbegin(), tasks.cend(), [](const Task &task) {
                    return task.status() == TaskStatus::InProgress;
                });
            if (candidate != tasks.cend()) {
                FocusStartCandidate projected;
                projected.taskId = candidate->id();
                projected.taskTitle = candidate->title();
                projected.estimatedMinutes = candidate->estimatedMinutes();
                if (candidate->categoryId().has_value()) {
                    const auto category = m_categoryRepository.findCategoryById(
                        *candidate->categoryId());
                    if (!category.has_value()) {
                        return FocusSnapshotResult::failure(
                            FocusError::StateConflict,
                            QStringLiteral("Focus candidate references a missing category."));
                    }
                    projected.categoryId = category->id;
                    projected.categoryName = category->name;
                    projected.categoryColor = category->color;
                }
                result.startCandidate = std::move(projected);
                result.availability.canStart = true;
            }
        }
        const QList<FocusSession> completed =
            m_focusRepository.findRecentCompletedFocusSessions(limit);
        result.recentCompletedRecords.reserve(completed.size());
        for (const FocusSession &session : completed) {
            result.recentCompletedRecords.append(loadRecord(session, currentUtc));
        }
        return FocusSnapshotResult::success(std::move(result));
    } catch (const RepositoryException &exception) {
        return persistenceValueFailure<FocusSnapshot>(exception);
    } catch (...) {
        return unexpectedValueFailure<FocusSnapshot>();
    }
}

FocusRecordResult FocusService::startFocus(const TaskId &taskId)
{
    if (const auto ready = requireInitialized(); !ready.ok()) {
        return FocusRecordResult::failure(ready.error, ready.detail);
    }
    try {
        const auto task = m_taskRepository.findById(taskId);
        if (!task.has_value()) {
            return FocusRecordResult::failure(
                FocusError::TaskNotFound, QStringLiteral("Focus task was not found."));
        }
        if (task->status() != TaskStatus::InProgress) {
            return FocusRecordResult::failure(
                FocusError::TaskNotInProgress,
                QStringLiteral("Only an in-progress task can start focus."));
        }
        if (m_focusRepository.findActiveFocusSession().has_value()) {
            return FocusRecordResult::failure(
                FocusError::ActiveSessionExists,
                QStringLiteral("Another focus session is already active."));
        }

        const QDateTime startedAtUtc = nowUtc();
        if (!startedAtUtc.isValid()) {
            return FocusRecordResult::failure(
                FocusError::StateConflict,
                QStringLiteral("Focus clock returned an invalid UTC time."));
        }
        FocusSession session;
        session.sessionId = QUuid::createUuid();
        session.taskId = task->id();
        session.state = FocusSessionState::Running;
        session.startedAtUtc = startedAtUtc;
        session.taskTitleSnapshot = task->title();
        session.estimatedMinutesSnapshot = task->estimatedMinutes();
        if (task->categoryId().has_value()) {
            const auto category =
                m_categoryRepository.findCategoryById(*task->categoryId());
            if (!category.has_value()) {
                return FocusRecordResult::failure(
                    FocusError::StateConflict,
                    QStringLiteral("Focus task references a missing category."));
            }
            session.categoryIdSnapshot = category->id;
            session.categoryNameSnapshot = category->name;
            session.categoryColorSnapshot = category->color;
        }
        const QDateTime startLookupBoundary = startedAtUtc.addMSecs(1);
        if (startLookupBoundary.isValid()) {
            const auto startEvent = m_activityRepository.findLatestStartForTaskBefore(
                taskId, startLookupBoundary);
            if (startEvent.has_value()) session.taskStartEventId = startEvent->eventId;
        }

        const FocusSessionWriteResult writeResult =
            m_focusRepository.startFocusSessionAtomically(session);
        if (!writeResult.ok()) {
            return FocusRecordResult::failure(
                mapWriteStatus(writeResult.status),
                QStringLiteral("Focus session could not be started atomically."));
        }
        m_shutdownPrepared = false;
        startCheckpointTimer(session.sessionId);
        clearBackgroundFailure();
        emit focusChanged();
        const FocusRecord record = loadRecord(session, startedAtUtc);
        return FocusRecordResult::success(record);
    } catch (const RepositoryException &exception) {
        return persistenceValueFailure<FocusRecord>(exception);
    } catch (...) {
        return unexpectedValueFailure<FocusRecord>();
    }
}

FocusRecordResult FocusService::pauseFocus(const FocusSessionId &sessionId)
{
    if (const auto ready = requireInitialized(); !ready.ok()) {
        return FocusRecordResult::failure(ready.error, ready.detail);
    }
    try {
        const QDateTime pausedAtUtc = nowUtc();
        const auto current = m_focusRepository.findFocusSessionById(sessionId);
        if (!current.has_value()) {
            return FocusRecordResult::failure(FocusError::SessionNotFound);
        }
        if (current->state != FocusSessionState::Running || !pausedAtUtc.isValid()) {
            return FocusRecordResult::failure(FocusError::StateConflict);
        }
        const FocusSessionWriteResult writeResult =
            m_focusRepository.pauseFocusSessionAtomically(
                sessionId, FocusSessionState::Running, pausedAtUtc);
        if (!writeResult.ok()) {
            return FocusRecordResult::failure(mapWriteStatus(writeResult.status));
        }
        stopCheckpointTimer();
        clearBackgroundFailure();
        emit focusChanged();
        const auto stored = m_focusRepository.findFocusSessionById(sessionId);
        if (!stored.has_value()) {
            return FocusRecordResult::failure(FocusError::SessionNotFound);
        }
        const FocusRecord record = loadRecord(*stored, pausedAtUtc);
        return FocusRecordResult::success(record);
    } catch (const RepositoryException &exception) {
        return persistenceValueFailure<FocusRecord>(exception);
    } catch (...) {
        return unexpectedValueFailure<FocusRecord>();
    }
}

FocusRecordResult FocusService::resumeFocus(const FocusSessionId &sessionId)
{
    if (const auto ready = requireInitialized(); !ready.ok()) {
        return FocusRecordResult::failure(ready.error, ready.detail);
    }
    try {
        const QDateTime resumedAtUtc = nowUtc();
        const auto current = m_focusRepository.findFocusSessionById(sessionId);
        if (!current.has_value()) {
            return FocusRecordResult::failure(FocusError::SessionNotFound);
        }
        if (current->state != FocusSessionState::Paused || !resumedAtUtc.isValid()) {
            return FocusRecordResult::failure(FocusError::StateConflict);
        }
        const FocusSessionWriteResult writeResult =
            m_focusRepository.resumeFocusSessionAtomically(
                sessionId, FocusSessionState::Paused, resumedAtUtc);
        if (!writeResult.ok()) {
            return FocusRecordResult::failure(mapWriteStatus(writeResult.status));
        }
        m_shutdownPrepared = false;
        startCheckpointTimer(sessionId);
        clearBackgroundFailure();
        emit focusChanged();
        const auto stored = m_focusRepository.findFocusSessionById(sessionId);
        if (!stored.has_value()) {
            return FocusRecordResult::failure(FocusError::SessionNotFound);
        }
        const FocusRecord record = loadRecord(*stored, resumedAtUtc);
        return FocusRecordResult::success(record);
    } catch (const RepositoryException &exception) {
        return persistenceValueFailure<FocusRecord>(exception);
    } catch (...) {
        return unexpectedValueFailure<FocusRecord>();
    }
}

FocusRecordResult FocusService::completeFocus(const FocusSessionId &sessionId)
{
    if (const auto ready = requireInitialized(); !ready.ok()) {
        return FocusRecordResult::failure(ready.error, ready.detail);
    }
    try {
        const QDateTime completedAtUtc = nowUtc();
        const auto current = m_focusRepository.findFocusSessionById(sessionId);
        if (!current.has_value()) {
            return FocusRecordResult::failure(FocusError::SessionNotFound);
        }
        if ((current->state != FocusSessionState::Running
             && current->state != FocusSessionState::Paused)
            || !completedAtUtc.isValid()) {
            return FocusRecordResult::failure(FocusError::StateConflict);
        }
        const FocusRecord before = loadRecord(*current, completedAtUtc);
        if (current->state == FocusSessionState::Running
            && !before.intervals.isEmpty()
            && completedAtUtc < before.intervals.constLast().checkpointAtUtc) {
            return FocusRecordResult::failure(FocusError::StateConflict);
        }
        if (before.focusedMilliseconds < minimumCompletedDurationMs) {
            return FocusRecordResult::failure(
                FocusError::MinimumDurationNotReached,
                QStringLiteral("A completed focus session requires one second."));
        }
        const FocusSessionWriteResult writeResult =
            m_focusRepository.completeFocusSessionAtomically(
                sessionId, current->state, completedAtUtc);
        if (!writeResult.ok()) {
            return FocusRecordResult::failure(mapWriteStatus(writeResult.status));
        }
        stopCheckpointTimer();
        clearBackgroundFailure();
        emit focusChanged();
        emit historyChanged();
        const auto stored = m_focusRepository.findFocusSessionById(sessionId);
        if (!stored.has_value()) {
            return FocusRecordResult::failure(FocusError::SessionNotFound);
        }
        const FocusRecord record = loadRecord(*stored, completedAtUtc);
        return FocusRecordResult::success(record);
    } catch (const RepositoryException &exception) {
        return persistenceValueFailure<FocusRecord>(exception);
    } catch (...) {
        return unexpectedValueFailure<FocusRecord>();
    }
}

FocusRecordResult FocusService::abandonFocus(const FocusSessionId &sessionId)
{
    if (const auto ready = requireInitialized(); !ready.ok()) {
        return FocusRecordResult::failure(ready.error, ready.detail);
    }
    try {
        const QDateTime abandonedAtUtc = nowUtc();
        const auto current = m_focusRepository.findFocusSessionById(sessionId);
        if (!current.has_value()) {
            return FocusRecordResult::failure(FocusError::SessionNotFound);
        }
        if (current->state != FocusSessionState::Running
            && current->state != FocusSessionState::Paused) {
            return FocusRecordResult::failure(FocusError::StateConflict);
        }
        const FocusRecord before = loadRecord(*current, abandonedAtUtc);
        const FocusSessionWriteResult writeResult =
            m_focusRepository.abandonFocusSessionAtomically(sessionId, current->state);
        if (!writeResult.ok()) {
            return FocusRecordResult::failure(mapWriteStatus(writeResult.status));
        }
        stopCheckpointTimer();
        clearBackgroundFailure();
        emit focusChanged();
        return FocusRecordResult::success(before);
    } catch (const RepositoryException &exception) {
        return persistenceValueFailure<FocusRecord>(exception);
    } catch (...) {
        return unexpectedValueFailure<FocusRecord>();
    }
}

FocusOperationResult FocusService::prepareForShutdown()
{
    if (m_shutdownPrepared) return FocusOperationResult::success();
    if (const auto ready = requireInitialized(); !ready.ok()) return ready;
    stopCheckpointTimer();
    try {
        const auto active = m_focusRepository.findActiveFocusSession();
        if (!active.has_value()) {
            m_shutdownPrepared = true;
            return FocusOperationResult::success();
        }
        const QDateTime shutdownAtUtc = nowUtc();
        if (!shutdownAtUtc.isValid()) {
            return FocusOperationResult::failure(FocusError::StateConflict);
        }
        const FocusRecord record = loadRecord(*active, shutdownAtUtc);
        FocusSessionWriteResult writeResult;
        bool changed = false;
        if (record.focusedMilliseconds < minimumCompletedDurationMs) {
            writeResult = m_focusRepository.abandonFocusSessionAtomically(
                active->sessionId, active->state);
            changed = true;
        } else if (active->state == FocusSessionState::Running) {
            writeResult = m_focusRepository.pauseFocusSessionAtomically(
                active->sessionId, FocusSessionState::Running, shutdownAtUtc);
            changed = true;
        }
        if (changed && !writeResult.ok()) {
            return FocusOperationResult::failure(mapWriteStatus(writeResult.status));
        }
        clearBackgroundFailure();
        m_shutdownPrepared = true;
        if (changed) emit focusChanged();
        return FocusOperationResult::success();
    } catch (const RepositoryException &exception) {
        return persistenceFailure(exception);
    } catch (...) {
        return unexpectedPersistenceFailure();
    }
}

FocusOperationResult FocusService::writeCheckpoint()
{
    if (const auto ready = requireInitialized(); !ready.ok()) return ready;
    if (m_runningSessionId.isNull()) {
        return FocusOperationResult::failure(
            FocusError::StateConflict,
            QStringLiteral("No Running focus session is owned by the timer."));
    }
    try {
        const QDateTime checkpointAtUtc = nowUtc();
        if (!checkpointAtUtc.isValid()) {
            return FocusOperationResult::failure(FocusError::StateConflict);
        }
        const FocusSessionWriteResult writeResult =
            m_focusRepository.updateFocusCheckpointAtomically(
                m_runningSessionId,
                FocusSessionState::Running,
                checkpointAtUtc);
        if (!writeResult.ok()) {
            return FocusOperationResult::failure(mapWriteStatus(writeResult.status));
        }
        clearBackgroundFailure();
        return FocusOperationResult::success();
    } catch (const RepositoryException &exception) {
        return persistenceFailure(exception);
    } catch (...) {
        return unexpectedPersistenceFailure();
    }
}

void FocusService::handleCheckpointTimeout()
{
    const FocusOperationResult result = writeCheckpoint();
    if (result.ok()) return;
    raiseBackgroundFailure(result.error, result.detail);
    if (result.error == FocusError::SessionNotFound
        || result.error == FocusError::StateConflict) {
        stopCheckpointTimer();
        emit focusChanged();
    }
}

FocusRecord FocusService::loadRecord(const FocusSession &session,
                                     const QDateTime &currentUtc) const
{
    FocusRecord record;
    record.session = session;
    record.intervals = m_focusRepository.findFocusIntervals(session.sessionId);
    if (record.intervals.isEmpty()) {
        throw RepositoryException("Focus session has no intervals.");
    }

    qint64 total = 0;
    for (qsizetype index = 0; index < record.intervals.size(); ++index) {
        const FocusInterval &interval = record.intervals.at(index);
        if (interval.sessionId != session.sessionId
            || interval.sequence != index
            || !interval.startedAtUtc.isValid()
            || !interval.checkpointAtUtc.isValid()) {
            throw RepositoryException("Focus intervals are inconsistent.");
        }
        QDateTime effectiveEnd;
        if (interval.endedAtUtc.has_value()) {
            effectiveEnd = *interval.endedAtUtc;
        } else {
            if (session.state != FocusSessionState::Running
                || index + 1 != record.intervals.size()) {
                throw RepositoryException("Only Running focus can have an open interval.");
            }
            effectiveEnd = std::max(currentUtc, interval.checkpointAtUtc);
        }
        const qint64 duration = interval.startedAtUtc.msecsTo(effectiveEnd);
        if (duration < 0
            || total > std::numeric_limits<qint64>::max() - duration) {
            throw RepositoryException("Focus duration is invalid.");
        }
        total += duration;
    }
    record.focusedMilliseconds = total;
    return record;
}

FocusOperationResult FocusService::requireInitialized() const
{
    return m_initialized
        ? FocusOperationResult::success()
        : FocusOperationResult::failure(
              FocusError::NotInitialized,
              QStringLiteral("FocusService must be initialized before use."));
}

QDateTime FocusService::nowUtc() const
{
    return m_nowProvider ? m_nowProvider().toUTC() : QDateTime{};
}

FocusError FocusService::mapWriteStatus(const FocusSessionWriteStatus status) const
{
    switch (status) {
    case FocusSessionWriteStatus::Success:
        return FocusError::None;
    case FocusSessionWriteStatus::TaskNotInProgress:
        return FocusError::TaskNotInProgress;
    case FocusSessionWriteStatus::ActiveSessionExists:
        return FocusError::ActiveSessionExists;
    case FocusSessionWriteStatus::SessionNotFound:
        return FocusError::SessionNotFound;
    case FocusSessionWriteStatus::StateConflict:
        return FocusError::StateConflict;
    }
    return FocusError::PersistenceFailure;
}

void FocusService::startCheckpointTimer(const FocusSessionId &sessionId)
{
    m_runningSessionId = sessionId;
    m_checkpointTimer.start();
}

void FocusService::stopCheckpointTimer()
{
    m_checkpointTimer.stop();
    m_runningSessionId = FocusSessionId{};
}

void FocusService::raiseBackgroundFailure(const FocusError error,
                                          const QString &detail)
{
    if (m_backgroundFailureActive) return;
    m_backgroundFailureActive = true;
    emit backgroundFailureRaised(error, detail);
}

void FocusService::clearBackgroundFailure()
{
    if (!m_backgroundFailureActive) return;
    m_backgroundFailureActive = false;
    emit backgroundFailureCleared();
}

} // namespace smartmate::model
