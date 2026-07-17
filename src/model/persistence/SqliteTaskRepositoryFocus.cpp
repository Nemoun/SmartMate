#include "persistence/SqliteTaskRepository.h"

#include "persistence/TaskSqlCodec.h"
#include "persistence/internal/SqliteTaskRepositorySupport.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
#include <QVariant>

namespace smartmate::model::persistence {
namespace {

using sqlite_task_repository_detail::throwDatabaseError;
using sqlite_task_repository_detail::throwPersistenceError;

QString focusStateToSqlText(const FocusSessionState state)
{
    switch (state) {
    case FocusSessionState::Running:
        return QStringLiteral("running");
    case FocusSessionState::Paused:
        return QStringLiteral("paused");
    case FocusSessionState::Completed:
        return QStringLiteral("completed");
    }
    throwPersistenceError(QStringLiteral("Invalid focus session state"));
}

FocusSessionState focusStateFromSqlText(const QString &value)
{
    if (value == QStringLiteral("running")) return FocusSessionState::Running;
    if (value == QStringLiteral("paused")) return FocusSessionState::Paused;
    if (value == QStringLiteral("completed")) return FocusSessionState::Completed;
    throwPersistenceError(QStringLiteral("SQLite contains an invalid focus session state"));
}

QDateTime utcFromMilliseconds(const QVariant &value)
{
    return QDateTime::fromMSecsSinceEpoch(value.toLongLong(), QTimeZone::UTC);
}

std::optional<QDateTime> optionalUtcFromMilliseconds(const QVariant &value)
{
    return value.isNull() ? std::nullopt
                          : std::optional<QDateTime>{utcFromMilliseconds(value)};
}

FocusSession focusSessionFromQuery(const QSqlQuery &query)
{
    FocusSession session;
    session.sessionId = QUuid{query.value(0).toString()};
    session.taskId = QUuid{query.value(1).toString()};
    if (session.sessionId.isNull() || session.taskId.isNull()) {
        throwPersistenceError(QStringLiteral("SQLite contains an invalid focus identity"));
    }
    session.state = focusStateFromSqlText(query.value(2).toString());
    session.startedAtUtc = utcFromMilliseconds(query.value(3));
    session.endedAtUtc = optionalUtcFromMilliseconds(query.value(4));
    session.taskTitleSnapshot = query.value(5).toString();
    if (!query.value(6).isNull()) {
        session.estimatedMinutesSnapshot = query.value(6).toInt();
    }
    if (!query.value(7).isNull()) {
        session.categoryIdSnapshot = QUuid{query.value(7).toString()};
        session.categoryNameSnapshot = query.value(8).toString();
        session.categoryColorSnapshot = detail::taskCategoryColorFromSqlText(
            query.value(9).toString());
        if (session.categoryIdSnapshot->isNull()) {
            throwPersistenceError(
                QStringLiteral("SQLite contains an invalid focus category id"));
        }
    }
    if (!query.value(10).isNull()) {
        session.taskStartEventId = QUuid{query.value(10).toString()};
        if (session.taskStartEventId->isNull()) {
            throwPersistenceError(
                QStringLiteral("SQLite contains an invalid focus start event id"));
        }
    }
    return session;
}

FocusInterval focusIntervalFromQuery(const QSqlQuery &query)
{
    FocusInterval interval;
    interval.sessionId = QUuid{query.value(0).toString()};
    if (interval.sessionId.isNull()) {
        throwPersistenceError(
            QStringLiteral("SQLite contains an invalid focus interval session id"));
    }
    interval.sequence = query.value(1).toInt();
    interval.startedAtUtc = utcFromMilliseconds(query.value(2));
    interval.endedAtUtc = optionalUtcFromMilliseconds(query.value(3));
    interval.checkpointAtUtc = utcFromMilliseconds(query.value(4));
    return interval;
}

QString focusSessionSelectColumns()
{
    return QStringLiteral(
        "SELECT id, task_id, state, started_at_utc_ms, ended_at_utc_ms, "
        "task_title_snapshot, estimated_minutes_snapshot, "
        "category_id_snapshot, category_name_snapshot, category_color_snapshot, "
        "task_start_event_id FROM focus_sessions ");
}

QString focusIntervalSelectColumns()
{
    return QStringLiteral(
        "SELECT session_id, sequence, started_at_utc_ms, ended_at_utc_ms, "
        "checkpoint_at_utc_ms FROM focus_intervals ");
}

void beginTransaction(QSqlDatabase &database)
{
    if (!database.transaction()) {
        throwDatabaseError(QStringLiteral("Cannot start focus session transaction"),
                           database.lastError());
    }
}

void commitTransaction(QSqlDatabase &database)
{
    if (!database.commit()) {
        throwDatabaseError(QStringLiteral("Cannot commit focus session transaction"),
                           database.lastError());
    }
}

FocusSessionWriteResult rollbackWithStatus(
    QSqlDatabase &database,
    const FocusSessionWriteStatus status)
{
    database.rollback();
    return {status};
}

struct StoredSessionState final {
    FocusSessionState state{FocusSessionState::Running};
    qint64 startedAtUtcMs{0};
};

std::optional<StoredSessionState> storedSessionState(
    QSqlDatabase &database,
    const FocusSessionId &sessionId)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT state, started_at_utc_ms FROM focus_sessions WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"),
                    sessionId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("Cannot query focus session state"),
                           query.lastError());
    }
    if (!query.next()) return std::nullopt;
    return StoredSessionState{focusStateFromSqlText(query.value(0).toString()),
                              query.value(1).toLongLong()};
}

struct OpenIntervalState final {
    int sequence{0};
    qint64 startedAtUtcMs{0};
    qint64 checkpointAtUtcMs{0};
};

std::optional<OpenIntervalState> openIntervalState(
    QSqlDatabase &database,
    const FocusSessionId &sessionId)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT sequence, started_at_utc_ms, checkpoint_at_utc_ms "
        "FROM focus_intervals WHERE session_id = :session_id "
        "AND ended_at_utc_ms IS NULL ORDER BY sequence DESC LIMIT 1"));
    query.bindValue(QStringLiteral(":session_id"),
                    sessionId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("Cannot query open focus interval"),
                           query.lastError());
    }
    if (!query.next()) return std::nullopt;
    return OpenIntervalState{query.value(0).toInt(),
                             query.value(1).toLongLong(),
                             query.value(2).toLongLong()};
}

std::optional<qint64> latestClosedIntervalEnd(
    QSqlDatabase &database,
    const FocusSessionId &sessionId)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT ended_at_utc_ms FROM focus_intervals "
        "WHERE session_id = :session_id ORDER BY sequence DESC LIMIT 1"));
    query.bindValue(QStringLiteral(":session_id"),
                    sessionId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("Cannot query latest focus interval"),
                           query.lastError());
    }
    if (!query.next() || query.value(0).isNull()) return std::nullopt;
    return query.value(0).toLongLong();
}

bool validCategorySnapshot(const FocusSession &session)
{
    const bool hasId = session.categoryIdSnapshot.has_value();
    const bool hasName = session.categoryNameSnapshot.has_value();
    const bool hasColor = session.categoryColorSnapshot.has_value();
    return (!hasId && !hasName && !hasColor)
        || (hasId && hasName && hasColor
            && !session.categoryIdSnapshot->isNull()
            && !session.categoryNameSnapshot->trimmed().isEmpty());
}

bool validNewSession(const FocusSession &session)
{
    return !session.sessionId.isNull()
        && !session.taskId.isNull()
        && session.state == FocusSessionState::Running
        && !session.endedAtUtc.has_value()
        && session.startedAtUtc.isValid()
        && session.startedAtUtc.toUTC().toMSecsSinceEpoch() >= 0
        && !session.taskTitleSnapshot.trimmed().isEmpty()
        && (!session.estimatedMinutesSnapshot.has_value()
            || (*session.estimatedMinutesSnapshot >= 1
                && *session.estimatedMinutesSnapshot <= 525600))
        && validCategorySnapshot(session)
        && (!session.taskStartEventId.has_value()
            || !session.taskStartEventId->isNull());
}

QVariant optionalUuidValue(const std::optional<QUuid> &id)
{
    return id.has_value()
        ? QVariant{id->toString(QUuid::WithoutBraces)} : QVariant{};
}

} // namespace

std::optional<FocusSession> SqliteTaskRepository::findActiveFocusSession() const
{
    auto database = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(database);
    query.prepare(focusSessionSelectColumns()
                  + QStringLiteral(
                      "WHERE active_marker = 1 ORDER BY started_at_utc_ms, id LIMIT 1"));
    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("Cannot query active focus session"),
                           query.lastError());
    }
    if (!query.next()) return std::nullopt;
    return focusSessionFromQuery(query);
}

std::optional<FocusSession> SqliteTaskRepository::findFocusSessionById(
    const FocusSessionId &sessionId) const
{
    auto database = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(database);
    query.prepare(focusSessionSelectColumns() + QStringLiteral("WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"),
                    sessionId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("Cannot query focus session"),
                           query.lastError());
    }
    if (!query.next()) return std::nullopt;
    return focusSessionFromQuery(query);
}

QList<FocusInterval> SqliteTaskRepository::findFocusIntervals(
    const FocusSessionId &sessionId) const
{
    auto database = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(database);
    query.prepare(focusIntervalSelectColumns()
                  + QStringLiteral(
                      "WHERE session_id = :session_id ORDER BY sequence"));
    query.bindValue(QStringLiteral(":session_id"),
                    sessionId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("Cannot query focus intervals"),
                           query.lastError());
    }
    QList<FocusInterval> intervals;
    while (query.next()) intervals.append(focusIntervalFromQuery(query));
    return intervals;
}

QList<FocusSession> SqliteTaskRepository::findRecentCompletedFocusSessions(
    const int limit) const
{
    if (limit <= 0) return {};
    auto database = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(database);
    query.prepare(focusSessionSelectColumns()
                  + QStringLiteral(
                      "WHERE state = 'completed' "
                      "ORDER BY ended_at_utc_ms DESC, id DESC LIMIT :limit"));
    query.bindValue(QStringLiteral(":limit"), limit);
    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("Cannot query completed focus sessions"),
                           query.lastError());
    }
    QList<FocusSession> sessions;
    while (query.next()) sessions.append(focusSessionFromQuery(query));
    return sessions;
}

FocusSessionWriteResult SqliteTaskRepository::startFocusSessionAtomically(
    const FocusSession &session)
{
    if (!validNewSession(session)) {
        return {FocusSessionWriteStatus::StateConflict};
    }
    auto database = QSqlDatabase::database(m_connectionName, false);
    beginTransaction(database);
    try {
        QSqlQuery taskQuery(database);
        taskQuery.prepare(QStringLiteral("SELECT status FROM tasks WHERE id = :task_id"));
        taskQuery.bindValue(QStringLiteral(":task_id"),
                            session.taskId.toString(QUuid::WithoutBraces));
        if (!taskQuery.exec()) {
            throwDatabaseError(QStringLiteral("Cannot validate focus task"),
                               taskQuery.lastError());
        }
        if (!taskQuery.next()
            || taskQuery.value(0).toString() != QStringLiteral("in_progress")) {
            return rollbackWithStatus(database,
                                      FocusSessionWriteStatus::TaskNotInProgress);
        }

        QSqlQuery activeQuery(database);
        if (!activeQuery.exec(QStringLiteral(
                "SELECT 1 FROM focus_sessions WHERE active_marker = 1 LIMIT 1"))) {
            throwDatabaseError(QStringLiteral("Cannot validate active focus session"),
                               activeQuery.lastError());
        }
        if (activeQuery.next()) {
            return rollbackWithStatus(database,
                                      FocusSessionWriteStatus::ActiveSessionExists);
        }

        if (session.taskStartEventId.has_value()) {
            QSqlQuery eventQuery(database);
            eventQuery.prepare(QStringLiteral(
                "SELECT 1 FROM task_activity_events WHERE id = :event_id "
                "AND task_id = :task_id AND transition = 'start' "
                "AND occurred_at_utc_ms <= :started_at LIMIT 1"));
            eventQuery.bindValue(QStringLiteral(":event_id"),
                                 session.taskStartEventId->toString(QUuid::WithoutBraces));
            eventQuery.bindValue(QStringLiteral(":task_id"),
                                 session.taskId.toString(QUuid::WithoutBraces));
            eventQuery.bindValue(QStringLiteral(":started_at"),
                                 session.startedAtUtc.toUTC().toMSecsSinceEpoch());
            if (!eventQuery.exec()) {
                throwDatabaseError(QStringLiteral("Cannot validate focus start event"),
                                   eventQuery.lastError());
            }
            if (!eventQuery.next()) {
                return rollbackWithStatus(database,
                                          FocusSessionWriteStatus::StateConflict);
            }
        }

        QSqlQuery sessionQuery(database);
        sessionQuery.prepare(QStringLiteral(
            "INSERT INTO focus_sessions ("
            "id, task_id, state, active_marker, started_at_utc_ms, ended_at_utc_ms, "
            "task_title_snapshot, estimated_minutes_snapshot, category_id_snapshot, "
            "category_name_snapshot, category_color_snapshot, task_start_event_id"
            ") VALUES ("
            ":id, :task_id, 'running', 1, :started_at, NULL, :title, :estimate, "
            ":category_id, :category_name, :category_color, :start_event_id)"));
        sessionQuery.bindValue(QStringLiteral(":id"),
                               session.sessionId.toString(QUuid::WithoutBraces));
        sessionQuery.bindValue(QStringLiteral(":task_id"),
                               session.taskId.toString(QUuid::WithoutBraces));
        sessionQuery.bindValue(QStringLiteral(":started_at"),
                               session.startedAtUtc.toUTC().toMSecsSinceEpoch());
        sessionQuery.bindValue(QStringLiteral(":title"), session.taskTitleSnapshot);
        sessionQuery.bindValue(QStringLiteral(":estimate"),
                               session.estimatedMinutesSnapshot.has_value()
                                   ? QVariant{*session.estimatedMinutesSnapshot}
                                   : QVariant{});
        sessionQuery.bindValue(QStringLiteral(":category_id"),
                               optionalUuidValue(session.categoryIdSnapshot));
        sessionQuery.bindValue(QStringLiteral(":category_name"),
                               session.categoryNameSnapshot.has_value()
                                   ? QVariant{*session.categoryNameSnapshot}
                                   : QVariant{});
        sessionQuery.bindValue(QStringLiteral(":category_color"),
                               session.categoryColorSnapshot.has_value()
                                   ? QVariant{detail::taskCategoryColorToSqlText(
                                         *session.categoryColorSnapshot)}
                                   : QVariant{});
        sessionQuery.bindValue(QStringLiteral(":start_event_id"),
                               optionalUuidValue(session.taskStartEventId));
        if (!sessionQuery.exec()) {
            throwDatabaseError(QStringLiteral("Cannot insert focus session"),
                               sessionQuery.lastError());
        }

        QSqlQuery intervalQuery(database);
        intervalQuery.prepare(QStringLiteral(
            "INSERT INTO focus_intervals ("
            "session_id, sequence, started_at_utc_ms, ended_at_utc_ms, "
            "checkpoint_at_utc_ms) VALUES ("
            ":session_id, 0, :started_at, NULL, :started_at)"));
        intervalQuery.bindValue(QStringLiteral(":session_id"),
                                session.sessionId.toString(QUuid::WithoutBraces));
        intervalQuery.bindValue(QStringLiteral(":started_at"),
                                session.startedAtUtc.toUTC().toMSecsSinceEpoch());
        if (!intervalQuery.exec()) {
            throwDatabaseError(QStringLiteral("Cannot insert first focus interval"),
                               intervalQuery.lastError());
        }
        commitTransaction(database);
        return {};
    } catch (...) {
        database.rollback();
        throw;
    }
}

FocusSessionWriteResult SqliteTaskRepository::pauseFocusSessionAtomically(
    const FocusSessionId &sessionId,
    const FocusSessionState expectedState,
    const QDateTime &pausedAtUtc)
{
    auto database = QSqlDatabase::database(m_connectionName, false);
    beginTransaction(database);
    try {
        const auto stored = storedSessionState(database, sessionId);
        if (!stored.has_value()) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::SessionNotFound);
        }
        if (expectedState != FocusSessionState::Running
            || stored->state != expectedState || !pausedAtUtc.isValid()) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }
        const auto interval = openIntervalState(database, sessionId);
        const qint64 pausedAtMs = pausedAtUtc.toUTC().toMSecsSinceEpoch();
        if (!interval.has_value() || pausedAtMs < interval->checkpointAtUtcMs) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }

        QSqlQuery intervalQuery(database);
        intervalQuery.prepare(QStringLiteral(
            "UPDATE focus_intervals SET ended_at_utc_ms = :at, "
            "checkpoint_at_utc_ms = :at WHERE session_id = :session_id "
            "AND sequence = :sequence AND ended_at_utc_ms IS NULL"));
        intervalQuery.bindValue(QStringLiteral(":at"), pausedAtMs);
        intervalQuery.bindValue(QStringLiteral(":session_id"),
                                sessionId.toString(QUuid::WithoutBraces));
        intervalQuery.bindValue(QStringLiteral(":sequence"), interval->sequence);
        if (!intervalQuery.exec()) {
            throwDatabaseError(QStringLiteral("Cannot close focus interval for pause"),
                               intervalQuery.lastError());
        }
        if (intervalQuery.numRowsAffected() != 1) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }

        QSqlQuery sessionQuery(database);
        sessionQuery.prepare(QStringLiteral(
            "UPDATE focus_sessions SET state = 'paused' WHERE id = :id "
            "AND state = 'running' AND active_marker = 1"));
        sessionQuery.bindValue(QStringLiteral(":id"),
                               sessionId.toString(QUuid::WithoutBraces));
        if (!sessionQuery.exec()) {
            throwDatabaseError(QStringLiteral("Cannot pause focus session"),
                               sessionQuery.lastError());
        }
        if (sessionQuery.numRowsAffected() != 1) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }
        commitTransaction(database);
        return {};
    } catch (...) {
        database.rollback();
        throw;
    }
}

FocusSessionWriteResult SqliteTaskRepository::resumeFocusSessionAtomically(
    const FocusSessionId &sessionId,
    const FocusSessionState expectedState,
    const QDateTime &resumedAtUtc)
{
    auto database = QSqlDatabase::database(m_connectionName, false);
    beginTransaction(database);
    try {
        const auto stored = storedSessionState(database, sessionId);
        if (!stored.has_value()) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::SessionNotFound);
        }
        const qint64 resumedAtMs = resumedAtUtc.toUTC().toMSecsSinceEpoch();
        const auto lastEndedAtMs = latestClosedIntervalEnd(database, sessionId);
        if (expectedState != FocusSessionState::Paused
            || stored->state != expectedState || !resumedAtUtc.isValid()
            || resumedAtMs < stored->startedAtUtcMs
            || !lastEndedAtMs.has_value() || resumedAtMs < *lastEndedAtMs
            || openIntervalState(database, sessionId).has_value()) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }

        QSqlQuery sequenceQuery(database);
        sequenceQuery.prepare(QStringLiteral(
            "SELECT COALESCE(MAX(sequence), -1) + 1 FROM focus_intervals "
            "WHERE session_id = :session_id"));
        sequenceQuery.bindValue(QStringLiteral(":session_id"),
                                sessionId.toString(QUuid::WithoutBraces));
        if (!sequenceQuery.exec() || !sequenceQuery.next()) {
            throwDatabaseError(QStringLiteral("Cannot determine focus interval sequence"),
                               sequenceQuery.lastError());
        }
        const int nextSequence = sequenceQuery.value(0).toInt();

        QSqlQuery sessionQuery(database);
        sessionQuery.prepare(QStringLiteral(
            "UPDATE focus_sessions SET state = 'running' WHERE id = :id "
            "AND state = 'paused' AND active_marker = 1"));
        sessionQuery.bindValue(QStringLiteral(":id"),
                               sessionId.toString(QUuid::WithoutBraces));
        if (!sessionQuery.exec()) {
            throwDatabaseError(QStringLiteral("Cannot resume focus session"),
                               sessionQuery.lastError());
        }
        if (sessionQuery.numRowsAffected() != 1) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }

        QSqlQuery intervalQuery(database);
        intervalQuery.prepare(QStringLiteral(
            "INSERT INTO focus_intervals (session_id, sequence, started_at_utc_ms, "
            "ended_at_utc_ms, checkpoint_at_utc_ms) VALUES ("
            ":session_id, :sequence, :at, NULL, :at)"));
        intervalQuery.bindValue(QStringLiteral(":session_id"),
                                sessionId.toString(QUuid::WithoutBraces));
        intervalQuery.bindValue(QStringLiteral(":sequence"), nextSequence);
        intervalQuery.bindValue(QStringLiteral(":at"), resumedAtMs);
        if (!intervalQuery.exec()) {
            throwDatabaseError(QStringLiteral("Cannot insert resumed focus interval"),
                               intervalQuery.lastError());
        }
        commitTransaction(database);
        return {};
    } catch (...) {
        database.rollback();
        throw;
    }
}

FocusSessionWriteResult SqliteTaskRepository::completeFocusSessionAtomically(
    const FocusSessionId &sessionId,
    const FocusSessionState expectedState,
    const QDateTime &completedAtUtc)
{
    auto database = QSqlDatabase::database(m_connectionName, false);
    beginTransaction(database);
    try {
        const auto stored = storedSessionState(database, sessionId);
        if (!stored.has_value()) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::SessionNotFound);
        }
        const qint64 completedAtMs = completedAtUtc.toUTC().toMSecsSinceEpoch();
        if ((expectedState != FocusSessionState::Running
             && expectedState != FocusSessionState::Paused)
            || stored->state != expectedState || !completedAtUtc.isValid()
            || completedAtMs < stored->startedAtUtcMs) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }

        const auto interval = openIntervalState(database, sessionId);
        if (expectedState == FocusSessionState::Running) {
            if (!interval.has_value() || completedAtMs < interval->checkpointAtUtcMs) {
                return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
            }
            QSqlQuery intervalQuery(database);
            intervalQuery.prepare(QStringLiteral(
                "UPDATE focus_intervals SET ended_at_utc_ms = :at, "
                "checkpoint_at_utc_ms = :at WHERE session_id = :session_id "
                "AND sequence = :sequence AND ended_at_utc_ms IS NULL"));
            intervalQuery.bindValue(QStringLiteral(":at"), completedAtMs);
            intervalQuery.bindValue(QStringLiteral(":session_id"),
                                    sessionId.toString(QUuid::WithoutBraces));
            intervalQuery.bindValue(QStringLiteral(":sequence"), interval->sequence);
            if (!intervalQuery.exec()) {
                throwDatabaseError(QStringLiteral("Cannot close completed focus interval"),
                                   intervalQuery.lastError());
            }
            if (intervalQuery.numRowsAffected() != 1) {
                return rollbackWithStatus(database,
                                          FocusSessionWriteStatus::StateConflict);
            }
        } else {
            const auto lastEndedAtMs = latestClosedIntervalEnd(database, sessionId);
            if (interval.has_value() || !lastEndedAtMs.has_value()
                || completedAtMs < *lastEndedAtMs) {
                return rollbackWithStatus(database,
                                          FocusSessionWriteStatus::StateConflict);
            }
        }

        QSqlQuery sessionQuery(database);
        sessionQuery.prepare(QStringLiteral(
            "UPDATE focus_sessions SET state = 'completed', active_marker = NULL, "
            "ended_at_utc_ms = :at WHERE id = :id AND state = :expected "
            "AND active_marker = 1"));
        sessionQuery.bindValue(QStringLiteral(":at"), completedAtMs);
        sessionQuery.bindValue(QStringLiteral(":id"),
                               sessionId.toString(QUuid::WithoutBraces));
        sessionQuery.bindValue(QStringLiteral(":expected"),
                               focusStateToSqlText(expectedState));
        if (!sessionQuery.exec()) {
            throwDatabaseError(QStringLiteral("Cannot complete focus session"),
                               sessionQuery.lastError());
        }
        if (sessionQuery.numRowsAffected() != 1) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }
        commitTransaction(database);
        return {};
    } catch (...) {
        database.rollback();
        throw;
    }
}

FocusSessionWriteResult SqliteTaskRepository::abandonFocusSessionAtomically(
    const FocusSessionId &sessionId,
    const FocusSessionState expectedState)
{
    auto database = QSqlDatabase::database(m_connectionName, false);
    beginTransaction(database);
    try {
        const auto stored = storedSessionState(database, sessionId);
        if (!stored.has_value()) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::SessionNotFound);
        }
        if ((expectedState != FocusSessionState::Running
             && expectedState != FocusSessionState::Paused)
            || stored->state != expectedState) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }
        QSqlQuery query(database);
        query.prepare(QStringLiteral(
            "DELETE FROM focus_sessions WHERE id = :id AND state = :expected "
            "AND active_marker = 1"));
        query.bindValue(QStringLiteral(":id"),
                        sessionId.toString(QUuid::WithoutBraces));
        query.bindValue(QStringLiteral(":expected"),
                        focusStateToSqlText(expectedState));
        if (!query.exec()) {
            throwDatabaseError(QStringLiteral("Cannot abandon focus session"),
                               query.lastError());
        }
        if (query.numRowsAffected() != 1) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }
        commitTransaction(database);
        return {};
    } catch (...) {
        database.rollback();
        throw;
    }
}

FocusSessionWriteResult SqliteTaskRepository::updateFocusCheckpointAtomically(
    const FocusSessionId &sessionId,
    const FocusSessionState expectedState,
    const QDateTime &checkpointAtUtc)
{
    auto database = QSqlDatabase::database(m_connectionName, false);
    beginTransaction(database);
    try {
        const auto stored = storedSessionState(database, sessionId);
        if (!stored.has_value()) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::SessionNotFound);
        }
        const auto interval = openIntervalState(database, sessionId);
        const qint64 checkpointMs = checkpointAtUtc.toUTC().toMSecsSinceEpoch();
        if (expectedState != FocusSessionState::Running
            || stored->state != expectedState || !checkpointAtUtc.isValid()
            || !interval.has_value()
            || checkpointMs < interval->checkpointAtUtcMs) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }
        QSqlQuery query(database);
        query.prepare(QStringLiteral(
            "UPDATE focus_intervals SET checkpoint_at_utc_ms = :at "
            "WHERE session_id = :session_id AND sequence = :sequence "
            "AND ended_at_utc_ms IS NULL"));
        query.bindValue(QStringLiteral(":at"), checkpointMs);
        query.bindValue(QStringLiteral(":session_id"),
                        sessionId.toString(QUuid::WithoutBraces));
        query.bindValue(QStringLiteral(":sequence"), interval->sequence);
        if (!query.exec()) {
            throwDatabaseError(QStringLiteral("Cannot update focus checkpoint"),
                               query.lastError());
        }
        if (query.numRowsAffected() != 1) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }
        commitTransaction(database);
        return {};
    } catch (...) {
        database.rollback();
        throw;
    }
}

FocusSessionWriteResult SqliteTaskRepository::recoverRunningFocusSessionAtomically()
{
    auto database = QSqlDatabase::database(m_connectionName, false);
    beginTransaction(database);
    try {
        QSqlQuery sessionLookup(database);
        if (!sessionLookup.exec(QStringLiteral(
                "SELECT id FROM focus_sessions WHERE state = 'running' "
                "AND active_marker = 1 LIMIT 1"))) {
            throwDatabaseError(QStringLiteral("Cannot query interrupted focus session"),
                               sessionLookup.lastError());
        }
        if (!sessionLookup.next()) {
            commitTransaction(database);
            return {};
        }
        const FocusSessionId sessionId{sessionLookup.value(0).toString()};
        if (sessionId.isNull()) {
            throwPersistenceError(
                QStringLiteral("SQLite contains an invalid interrupted focus id"));
        }
        const auto interval = openIntervalState(database, sessionId);
        if (!interval.has_value()) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }

        QSqlQuery intervalQuery(database);
        intervalQuery.prepare(QStringLiteral(
            "UPDATE focus_intervals SET ended_at_utc_ms = checkpoint_at_utc_ms "
            "WHERE session_id = :session_id AND sequence = :sequence "
            "AND ended_at_utc_ms IS NULL"));
        intervalQuery.bindValue(QStringLiteral(":session_id"),
                                sessionId.toString(QUuid::WithoutBraces));
        intervalQuery.bindValue(QStringLiteral(":sequence"), interval->sequence);
        if (!intervalQuery.exec()) {
            throwDatabaseError(QStringLiteral("Cannot close interrupted focus interval"),
                               intervalQuery.lastError());
        }
        if (intervalQuery.numRowsAffected() != 1) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }

        QSqlQuery sessionQuery(database);
        sessionQuery.prepare(QStringLiteral(
            "UPDATE focus_sessions SET state = 'paused' WHERE id = :id "
            "AND state = 'running' AND active_marker = 1"));
        sessionQuery.bindValue(QStringLiteral(":id"),
                               sessionId.toString(QUuid::WithoutBraces));
        if (!sessionQuery.exec()) {
            throwDatabaseError(QStringLiteral("Cannot recover interrupted focus session"),
                               sessionQuery.lastError());
        }
        if (sessionQuery.numRowsAffected() != 1) {
            return rollbackWithStatus(database, FocusSessionWriteStatus::StateConflict);
        }
        commitTransaction(database);
        return {};
    } catch (...) {
        database.rollback();
        throw;
    }
}

} // namespace smartmate::model::persistence
