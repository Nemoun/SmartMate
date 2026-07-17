#include "persistence/SqliteTaskRepository.h"

#include "domain/FocusSession.h"
#include "repositories/RepositoryException.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

#include <algorithm>

using namespace smartmate::model;
using smartmate::model::persistence::SqliteTaskRepository;

namespace {

const QDateTime kTaskCreated =
    QDateTime::fromMSecsSinceEpoch(1'752'000'000'000LL, QTimeZone::UTC);
const QDateTime kTaskStarted = kTaskCreated.addSecs(60);
const QDateTime kFocusStarted = kTaskStarted.addSecs(60);

QString connectionName(const QString &prefix)
{
    return QStringLiteral("%1_%2")
        .arg(prefix, QUuid::createUuid().toString(QUuid::WithoutBraces));
}

Task makeTask(const TaskId &id,
              const TaskStatus status = TaskStatus::Todo,
              const std::optional<TaskCategoryId> categoryId = std::nullopt)
{
    return Task{id,
                QStringLiteral("实现专注持久化"),
                QStringLiteral("保存可靠的专注区间"),
                TaskPriority::High,
                status,
                std::nullopt,
                kTaskCreated.addDays(2),
                90,
                kTaskCreated,
                kTaskCreated,
                categoryId};
}

TaskTransitionWrite transitionWrite(const TaskId &taskId,
                                    const TaskStatus from,
                                    const TaskStatus to,
                                    const TaskTransition transition,
                                    const QDateTime &atUtc)
{
    TaskActivityEvent event;
    event.eventId = QUuid::createUuid();
    event.taskId = taskId;
    event.transition = transition;
    event.fromStatus = from;
    event.toStatus = to;
    event.occurredAtUtc = atUtc;
    event.estimatedMinutesSnapshot = 90;
    event.prioritySnapshot = TaskPriority::High;
    return {{taskId, from, to, std::nullopt, atUtc}, event};
}

TaskActivityEvent startTask(SqliteTaskRepository &repository,
                            const TaskId &taskId)
{
    TaskTransitionWrite write = transitionWrite(taskId,
                                                TaskStatus::Todo,
                                                TaskStatus::InProgress,
                                                TaskTransition::Start,
                                                kTaskStarted);
    const auto result = repository.applyTransitionsAtomically({write});
    if (result.updatedTaskCount != 1) {
        qFatal("Unable to prepare an in-progress task for focus testing.");
    }
    return write.event;
}

FocusSession makeSession(const FocusSessionId &sessionId,
                         const TaskId &taskId,
                         const QDateTime &startedAtUtc,
                         const std::optional<TaskActivityEventId> startEventId = std::nullopt,
                         const std::optional<TaskCategory> category = std::nullopt)
{
    FocusSession session;
    session.sessionId = sessionId;
    session.taskId = taskId;
    session.state = FocusSessionState::Running;
    session.startedAtUtc = startedAtUtc;
    session.taskTitleSnapshot = QStringLiteral("实现专注持久化");
    session.estimatedMinutesSnapshot = 90;
    session.taskStartEventId = startEventId;
    if (category.has_value()) {
        session.categoryIdSnapshot = category->id;
        session.categoryNameSnapshot = category->name;
        session.categoryColorSnapshot = category->color;
    }
    return session;
}

void createVersionFourDatabase(const QString &databasePath,
                               const TaskId &taskId)
{
    const QString name = connectionName(QStringLiteral("focus_v4"));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        database.setDatabaseName(databasePath);
        QVERIFY2(database.open(), qPrintable(database.lastError().text()));
        QSqlQuery query(database);
        QVERIFY2(query.exec(QStringLiteral(
                     "CREATE TABLE tasks ("
                     "id TEXT PRIMARY KEY NOT NULL, title TEXT NOT NULL, "
                     "description TEXT NOT NULL, priority TEXT NOT NULL, "
                     "status TEXT NOT NULL, status_before_archive TEXT NULL, "
                     "deadline_utc_ms INTEGER NULL, estimated_minutes INTEGER NULL, "
                     "created_at_utc_ms INTEGER NOT NULL, updated_at_utc_ms INTEGER NOT NULL, "
                     "category_id TEXT NULL)")),
                 qPrintable(query.lastError().text()));
        query.prepare(QStringLiteral(
            "INSERT INTO tasks VALUES (:id, '迁移保留任务', '', 'normal', 'todo', "
            "NULL, NULL, 30, :created, :updated, NULL)"));
        query.bindValue(QStringLiteral(":id"),
                        taskId.toString(QUuid::WithoutBraces));
        query.bindValue(QStringLiteral(":created"),
                        kTaskCreated.toMSecsSinceEpoch());
        query.bindValue(QStringLiteral(":updated"),
                        kTaskCreated.toMSecsSinceEpoch());
        QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
        QVERIFY(query.exec(QStringLiteral("PRAGMA user_version = 4")));
        database.close();
    }
    QSqlDatabase::removeDatabase(name);
}

int scalarCount(const QString &databasePath, const QString &tableName)
{
    const QString name = connectionName(QStringLiteral("focus_count"));
    int count = -1;
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        database.setDatabaseName(databasePath);
        if (!database.open()) return -1;
        QSqlQuery query(database);
        if (query.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(tableName))
            && query.next()) {
            count = query.value(0).toInt();
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(name);
    return count;
}

} // namespace

class SqliteFocusSessionRepositoryTest final : public QObject {
    Q_OBJECT

private slots:
    void migratesVersionFourAtomicallyAndRejectsVersionSix();
    void rollsBackFailedVersionFiveMigration();
    void roundTripsSnapshotsIntervalsAndLatestStart();
    void enforcesTaskEligibilityAndSingleActiveSession();
    void checkpointsPausesResumesAndCompletesRunningSession();
    void completesPausedSessionAndAbandonsWithoutHistory();
    void preservesStateOnConflictsAndSqlFailure();
    void recoversInterruptedSessionAndOrdersRecentHistory();
    void keepsFocusFactsAfterDatabaseReopen();
    void cascadesFocusFactsWhenArchivedTaskIsPermanentlyDeleted();
};

void SqliteFocusSessionRepositoryTest::migratesVersionFourAtomicallyAndRejectsVersionSix()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("v4.db"));
    const TaskId taskId = QUuid::createUuid();
    createVersionFourDatabase(databasePath, taskId);

    {
        SqliteTaskRepository repository(databasePath);
        QVERIFY(repository.findById(taskId).has_value());
        QVERIFY(!repository.findActiveFocusSession().has_value());
        QVERIFY(repository.findRecentCompletedFocusSessions(50).isEmpty());
        QVERIFY(!repository.findLatestStartForTaskBefore(
                    taskId, kFocusStarted).has_value());
    }
    const QString verifyName = connectionName(QStringLiteral("focus_v5_verify"));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), verifyName);
        database.setDatabaseName(databasePath);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral("PRAGMA user_version")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 5);
        database.close();
    }
    QSqlDatabase::removeDatabase(verifyName);

    const QString futurePath = directory.filePath(QStringLiteral("future.db"));
    const QString futureName = connectionName(QStringLiteral("focus_future"));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), futureName);
        database.setDatabaseName(futurePath);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral("PRAGMA user_version = 6")));
        database.close();
    }
    QSqlDatabase::removeDatabase(futureName);
    QVERIFY_EXCEPTION_THROWN(SqliteTaskRepository{futurePath}, RepositoryException);
}

void SqliteFocusSessionRepositoryTest::rollsBackFailedVersionFiveMigration()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("broken.db"));
    const TaskId taskId = QUuid::createUuid();
    createVersionFourDatabase(databasePath, taskId);

    const QString sabotageName = connectionName(QStringLiteral("focus_sabotage"));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), sabotageName);
        database.setDatabaseName(databasePath);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral("CREATE TABLE focus_sessions (unexpected TEXT)")));
        database.close();
    }
    QSqlDatabase::removeDatabase(sabotageName);

    QVERIFY_EXCEPTION_THROWN(SqliteTaskRepository{databasePath}, RepositoryException);
    const QString verifyName = connectionName(QStringLiteral("focus_rollback_verify"));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), verifyName);
        database.setDatabaseName(databasePath);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral("PRAGMA user_version")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 4);
        const QString taskStatement = QStringLiteral(
            "SELECT title FROM tasks WHERE id = '%1'")
            .arg(taskId.toString(QUuid::WithoutBraces));
        QVERIFY(query.exec(taskStatement));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("迁移保留任务"));
        QVERIFY(!query.exec(QStringLiteral("SELECT * FROM focus_intervals")));
        database.close();
    }
    QSqlDatabase::removeDatabase(verifyName);
}

void SqliteFocusSessionRepositoryTest::roundTripsSnapshotsIntervalsAndLatestStart()
{
    SqliteTaskRepository repository(QStringLiteral(":memory:"));
    const TaskCategory category{QUuid::createUuid(),
                                QStringLiteral("开发"),
                                TaskCategoryColor::Violet,
                                kTaskCreated,
                                kTaskCreated};
    repository.insertCategory(category);
    const TaskId taskId = QUuid::createUuid();
    repository.insert(makeTask(taskId, TaskStatus::Todo, category.id));
    const TaskActivityEvent startEvent = startTask(repository, taskId);
    QCOMPARE(repository.findLatestStartForTaskBefore(taskId, kFocusStarted),
             std::optional{startEvent});

    const FocusSession expected = makeSession(QUuid::createUuid(),
                                              taskId,
                                              kFocusStarted,
                                              startEvent.eventId,
                                              category);
    QVERIFY(repository.startFocusSessionAtomically(expected).ok());
    QCOMPARE(repository.findActiveFocusSession(), std::optional{expected});
    QCOMPARE(repository.findFocusSessionById(expected.sessionId),
             std::optional{expected});
    const QList<FocusInterval> intervals =
        repository.findFocusIntervals(expected.sessionId);
    const QList<FocusInterval> expectedIntervals{
        {expected.sessionId, 0, kFocusStarted, std::nullopt, kFocusStarted}};
    QCOMPARE(intervals, expectedIntervals);
}

void SqliteFocusSessionRepositoryTest::enforcesTaskEligibilityAndSingleActiveSession()
{
    SqliteTaskRepository repository(QStringLiteral(":memory:"));
    const TaskId taskId = QUuid::createUuid();
    repository.insert(makeTask(taskId));
    const FocusSession first = makeSession(QUuid::createUuid(), taskId, kFocusStarted);
    QCOMPARE(repository.startFocusSessionAtomically(first).status,
             FocusSessionWriteStatus::TaskNotInProgress);
    QVERIFY(!repository.findActiveFocusSession().has_value());

    startTask(repository, taskId);
    QVERIFY(repository.startFocusSessionAtomically(first).ok());
    const FocusSession second = makeSession(QUuid::createUuid(),
                                            taskId,
                                            kFocusStarted.addSecs(1));
    QCOMPARE(repository.startFocusSessionAtomically(second).status,
             FocusSessionWriteStatus::ActiveSessionExists);
    QVERIFY(!repository.findFocusSessionById(second.sessionId).has_value());
}

void SqliteFocusSessionRepositoryTest::checkpointsPausesResumesAndCompletesRunningSession()
{
    SqliteTaskRepository repository(QStringLiteral(":memory:"));
    const TaskId taskId = QUuid::createUuid();
    repository.insert(makeTask(taskId));
    startTask(repository, taskId);
    const FocusSession session = makeSession(QUuid::createUuid(), taskId, kFocusStarted);
    QVERIFY(repository.startFocusSessionAtomically(session).ok());

    QVERIFY(repository.updateFocusCheckpointAtomically(
        session.sessionId, FocusSessionState::Running, kFocusStarted.addSecs(10)).ok());
    QVERIFY(repository.pauseFocusSessionAtomically(
        session.sessionId, FocusSessionState::Running, kFocusStarted.addSecs(20)).ok());
    QCOMPARE(repository.findActiveFocusSession()->state, FocusSessionState::Paused);
    QCOMPARE(repository.resumeFocusSessionAtomically(
                 session.sessionId,
                 FocusSessionState::Paused,
                 kFocusStarted.addSecs(19)).status,
             FocusSessionWriteStatus::StateConflict);
    QVERIFY(repository.resumeFocusSessionAtomically(
        session.sessionId, FocusSessionState::Paused, kFocusStarted.addSecs(30)).ok());
    QVERIFY(repository.updateFocusCheckpointAtomically(
        session.sessionId, FocusSessionState::Running, kFocusStarted.addSecs(40)).ok());
    QVERIFY(repository.completeFocusSessionAtomically(
        session.sessionId, FocusSessionState::Running, kFocusStarted.addSecs(50)).ok());

    const auto completed = repository.findFocusSessionById(session.sessionId);
    QVERIFY(completed.has_value());
    QCOMPARE(completed->state, FocusSessionState::Completed);
    QCOMPARE(completed->endedAtUtc,
             std::optional{kFocusStarted.addSecs(50)});
    QVERIFY(!repository.findActiveFocusSession().has_value());
    const auto intervals = repository.findFocusIntervals(session.sessionId);
    QCOMPARE(intervals.size(), 2);
    QCOMPARE(intervals.at(0).sequence, 0);
    QCOMPARE(intervals.at(0).endedAtUtc,
             std::optional{kFocusStarted.addSecs(20)});
    QCOMPARE(intervals.at(1).sequence, 1);
    QCOMPARE(intervals.at(1).startedAtUtc, kFocusStarted.addSecs(30));
    QCOMPARE(intervals.at(1).checkpointAtUtc, kFocusStarted.addSecs(50));
    QCOMPARE(intervals.at(1).endedAtUtc,
             std::optional{kFocusStarted.addSecs(50)});
}

void SqliteFocusSessionRepositoryTest::completesPausedSessionAndAbandonsWithoutHistory()
{
    SqliteTaskRepository repository(QStringLiteral(":memory:"));
    const TaskId taskId = QUuid::createUuid();
    repository.insert(makeTask(taskId));
    startTask(repository, taskId);

    const FocusSession completed = makeSession(QUuid::createUuid(), taskId, kFocusStarted);
    QVERIFY(repository.startFocusSessionAtomically(completed).ok());
    QVERIFY(repository.pauseFocusSessionAtomically(
        completed.sessionId, FocusSessionState::Running, kFocusStarted.addSecs(10)).ok());
    QCOMPARE(repository.completeFocusSessionAtomically(
                 completed.sessionId,
                 FocusSessionState::Paused,
                 kFocusStarted.addSecs(9)).status,
             FocusSessionWriteStatus::StateConflict);
    QVERIFY(repository.completeFocusSessionAtomically(
        completed.sessionId, FocusSessionState::Paused, kFocusStarted.addSecs(15)).ok());

    const FocusSession abandoned = makeSession(QUuid::createUuid(),
                                               taskId,
                                               kFocusStarted.addSecs(20));
    QVERIFY(repository.startFocusSessionAtomically(abandoned).ok());
    QVERIFY(repository.abandonFocusSessionAtomically(
        abandoned.sessionId, FocusSessionState::Running).ok());
    QVERIFY(!repository.findFocusSessionById(abandoned.sessionId).has_value());
    QVERIFY(repository.findFocusIntervals(abandoned.sessionId).isEmpty());
    QCOMPARE(repository.findRecentCompletedFocusSessions(50).size(), 1);
    QVERIFY(repository.findRecentCompletedFocusSessions(0).isEmpty());
}

void SqliteFocusSessionRepositoryTest::preservesStateOnConflictsAndSqlFailure()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("rollback.db"));
    SqliteTaskRepository repository(databasePath);
    const TaskId taskId = QUuid::createUuid();
    repository.insert(makeTask(taskId));
    startTask(repository, taskId);
    const FocusSession session = makeSession(QUuid::createUuid(), taskId, kFocusStarted);
    QVERIFY(repository.startFocusSessionAtomically(session).ok());

    QCOMPARE(repository.pauseFocusSessionAtomically(
                 session.sessionId,
                 FocusSessionState::Paused,
                 kFocusStarted.addSecs(10)).status,
             FocusSessionWriteStatus::StateConflict);
    QCOMPARE(repository.pauseFocusSessionAtomically(
                 QUuid::createUuid(),
                 FocusSessionState::Running,
                 kFocusStarted.addSecs(10)).status,
             FocusSessionWriteStatus::SessionNotFound);
    QCOMPARE(repository.findFocusIntervals(session.sessionId).first().endedAtUtc,
             std::nullopt);

    const QString triggerName = connectionName(QStringLiteral("focus_failure"));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), triggerName);
        database.setDatabaseName(databasePath);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TRIGGER fail_focus_pause BEFORE UPDATE OF state ON focus_sessions "
            "WHEN NEW.state = 'paused' BEGIN "
            "SELECT RAISE(ABORT, 'injected focus failure'); END")));
        database.close();
    }
    QSqlDatabase::removeDatabase(triggerName);

    bool threw = false;
    try {
        const auto ignored = repository.pauseFocusSessionAtomically(
            session.sessionId,
            FocusSessionState::Running,
            kFocusStarted.addSecs(20));
        Q_UNUSED(ignored);
    } catch (const RepositoryException &) {
        threw = true;
    }
    QVERIFY(threw);
    QCOMPARE(repository.findActiveFocusSession()->state, FocusSessionState::Running);
    QCOMPARE(repository.findFocusIntervals(session.sessionId).first().endedAtUtc,
             std::nullopt);
}

void SqliteFocusSessionRepositoryTest::recoversInterruptedSessionAndOrdersRecentHistory()
{
    SqliteTaskRepository repository(QStringLiteral(":memory:"));
    const TaskId taskId = QUuid::createUuid();
    repository.insert(makeTask(taskId));
    startTask(repository, taskId);

    const FocusSession interrupted = makeSession(QUuid::createUuid(), taskId, kFocusStarted);
    QVERIFY(repository.startFocusSessionAtomically(interrupted).ok());
    const QDateTime checkpoint = kFocusStarted.addSecs(12);
    QVERIFY(repository.updateFocusCheckpointAtomically(
        interrupted.sessionId, FocusSessionState::Running, checkpoint).ok());
    QVERIFY(repository.recoverRunningFocusSessionAtomically().ok());
    QCOMPARE(repository.findActiveFocusSession()->state, FocusSessionState::Paused);
    QCOMPARE(repository.findFocusIntervals(interrupted.sessionId).first().endedAtUtc,
             std::optional{checkpoint});
    QVERIFY(repository.completeFocusSessionAtomically(
        interrupted.sessionId, FocusSessionState::Paused, checkpoint.addSecs(1)).ok());
    QVERIFY(repository.recoverRunningFocusSessionAtomically().ok());

    QList<QPair<QDateTime, FocusSessionId>> expected;
    expected.append({checkpoint.addSecs(1), interrupted.sessionId});
    for (int index = 0; index < 52; ++index) {
        const QDateTime startedAt = kFocusStarted.addSecs(100 + index * 10);
        const FocusSession session = makeSession(QUuid::createUuid(), taskId, startedAt);
        QVERIFY(repository.startFocusSessionAtomically(session).ok());
        const QDateTime endedAt = startedAt.addSecs(5);
        QVERIFY(repository.completeFocusSessionAtomically(
            session.sessionId, FocusSessionState::Running, endedAt).ok());
        expected.append({endedAt, session.sessionId});
    }
    std::sort(expected.begin(), expected.end(), [](const auto &left, const auto &right) {
        return left.first == right.first
            ? left.second.toString(QUuid::WithoutBraces)
                  > right.second.toString(QUuid::WithoutBraces)
            : left.first > right.first;
    });
    const auto recent = repository.findRecentCompletedFocusSessions(50);
    QCOMPARE(recent.size(), 50);
    for (int index = 0; index < recent.size(); ++index) {
        QCOMPARE(recent.at(index).sessionId, expected.at(index).second);
    }
}

void SqliteFocusSessionRepositoryTest::keepsFocusFactsAfterDatabaseReopen()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("reopen.db"));
    const TaskId taskId = QUuid::createUuid();
    const FocusSessionId sessionId = QUuid::createUuid();
    const QDateTime completedAt = kFocusStarted.addSecs(30);
    {
        SqliteTaskRepository repository(databasePath);
        repository.insert(makeTask(taskId));
        const TaskActivityEvent startEvent = startTask(repository, taskId);
        const FocusSession session = makeSession(sessionId,
                                                 taskId,
                                                 kFocusStarted,
                                                 startEvent.eventId);
        QVERIFY(repository.startFocusSessionAtomically(session).ok());
        QVERIFY(repository.completeFocusSessionAtomically(
            sessionId, FocusSessionState::Running, completedAt).ok());
    }

    {
        SqliteTaskRepository reopened(databasePath);
        const auto session = reopened.findFocusSessionById(sessionId);
        QVERIFY(session.has_value());
        QCOMPARE(session->state, FocusSessionState::Completed);
        QCOMPARE(session->endedAtUtc, std::optional{completedAt});
        QVERIFY(session->taskStartEventId.has_value());
        const auto intervals = reopened.findFocusIntervals(sessionId);
        QCOMPARE(intervals.size(), 1);
        QCOMPARE(intervals.first().endedAtUtc, std::optional{completedAt});
        QCOMPARE(reopened.findRecentCompletedFocusSessions(50).size(), 1);
    }
}

void SqliteFocusSessionRepositoryTest::cascadesFocusFactsWhenArchivedTaskIsPermanentlyDeleted()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("cascade.db"));
    const TaskId taskId = QUuid::createUuid();
    FocusSessionId sessionId;
    {
        SqliteTaskRepository repository(databasePath);
        repository.insert(makeTask(taskId));
        startTask(repository, taskId);
        sessionId = QUuid::createUuid();
        const FocusSession session = makeSession(sessionId, taskId, kFocusStarted);
        QVERIFY(repository.startFocusSessionAtomically(session).ok());
        QVERIFY(repository.completeFocusSessionAtomically(
            sessionId, FocusSessionState::Running, kFocusStarted.addSecs(30)).ok());

        const auto complete = transitionWrite(taskId,
                                              TaskStatus::InProgress,
                                              TaskStatus::Done,
                                              TaskTransition::Complete,
                                              kFocusStarted.addSecs(40));
        QCOMPARE(repository.applyTransitionsAtomically({complete}).updatedTaskCount, 1);
        TaskTransitionWrite archive = transitionWrite(taskId,
                                                      TaskStatus::Done,
                                                      TaskStatus::Archived,
                                                      TaskTransition::Archive,
                                                      kFocusStarted.addSecs(50));
        archive.stateChange.statusBeforeArchive = TaskStatus::Done;
        QCOMPARE(repository.applyTransitionsAtomically({archive}).updatedTaskCount, 1);
        QCOMPARE(repository.deleteArchivedTasksWithDependencies({taskId}).deletedTaskCount, 1);
        QVERIFY(!repository.findFocusSessionById(sessionId).has_value());
        QVERIFY(repository.findFocusIntervals(sessionId).isEmpty());
    }
    QCOMPARE(scalarCount(databasePath, QStringLiteral("focus_sessions")), 0);
    QCOMPARE(scalarCount(databasePath, QStringLiteral("focus_intervals")), 0);
}

QTEST_MAIN(SqliteFocusSessionRepositoryTest)
#include "tst_SqliteFocusSessionRepository.moc"
