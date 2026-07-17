#include "AppBootstrapper.h"
#include "persistence/SqliteTaskRepository.h"

#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

using namespace smartmate::model;
using smartmate::model::persistence::SqliteTaskRepository;

namespace {

Task inProgressTask(const TaskId &taskId, const QDateTime &nowUtc)
{
    return {taskId,
            QStringLiteral("应用生命周期专注"),
            {},
            TaskPriority::Normal,
            TaskStatus::InProgress,
            std::nullopt,
            std::nullopt,
            30,
            nowUtc.addSecs(-60),
            nowUtc};
}

FocusSession focusSession(const FocusSessionId &sessionId,
                          const TaskId &taskId,
                          const QDateTime &startedAtUtc)
{
    FocusSession session;
    session.sessionId = sessionId;
    session.taskId = taskId;
    session.state = FocusSessionState::Running;
    session.startedAtUtc = startedAtUtc;
    session.taskTitleSnapshot = QStringLiteral("应用生命周期专注");
    session.estimatedMinutesSnapshot = 30;
    return session;
}

} // namespace

class FocusAppLifecycleTest final : public QObject {
    Q_OBJECT

private slots:
    void startupRecoversLongSessionAndDiscardsShortSession();
    void normalShutdownPausesRunningSession();
};

void FocusAppLifecycleTest::startupRecoversLongSessionAndDiscardsShortSession()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();

    const QString longPath = directory.filePath(QStringLiteral("long.db"));
    const TaskId longTaskId = QUuid::createUuid();
    const FocusSessionId longSessionId = QUuid::createUuid();
    {
        SqliteTaskRepository repository(longPath);
        repository.insert(inProgressTask(longTaskId, nowUtc));
        const FocusSession session = focusSession(
            longSessionId, longTaskId, nowUtc.addSecs(-3));
        QVERIFY(repository.startFocusSessionAtomically(session).ok());
        QVERIFY(repository.updateFocusCheckpointAtomically(
            longSessionId,
            FocusSessionState::Running,
            nowUtc.addSecs(-1)).ok());
    }
    {
        smartmate::app::AppBootstrapper bootstrapper(longPath);
        SqliteTaskRepository verification(longPath);
        const auto recovered = verification.findFocusSessionById(longSessionId);
        QVERIFY(recovered.has_value());
        QCOMPARE(recovered->state, FocusSessionState::Paused);
        QCOMPARE(verification.findFocusIntervals(longSessionId).first().endedAtUtc,
                 std::optional{nowUtc.addSecs(-1)});
        bootstrapper.prepareForShutdown();
    }

    const QString shortPath = directory.filePath(QStringLiteral("short.db"));
    const TaskId shortTaskId = QUuid::createUuid();
    const FocusSessionId shortSessionId = QUuid::createUuid();
    {
        SqliteTaskRepository repository(shortPath);
        repository.insert(inProgressTask(shortTaskId, nowUtc));
        QVERIFY(repository.startFocusSessionAtomically(
            focusSession(shortSessionId,
                         shortTaskId,
                         nowUtc.addMSecs(-500))).ok());
    }
    {
        smartmate::app::AppBootstrapper bootstrapper(shortPath);
        SqliteTaskRepository verification(shortPath);
        QVERIFY(!verification.findFocusSessionById(shortSessionId).has_value());
        bootstrapper.prepareForShutdown();
    }
}

void FocusAppLifecycleTest::normalShutdownPausesRunningSession()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("shutdown.db"));
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    const TaskId taskId = QUuid::createUuid();
    const FocusSessionId sessionId = QUuid::createUuid();
    {
        SqliteTaskRepository repository(path);
        repository.insert(inProgressTask(taskId, nowUtc));
    }

    smartmate::app::AppBootstrapper bootstrapper(path);
    {
        SqliteTaskRepository writer(path);
        QVERIFY(writer.startFocusSessionAtomically(
            focusSession(sessionId, taskId, nowUtc.addSecs(-2))).ok());
    }
    bootstrapper.prepareForShutdown();
    {
        SqliteTaskRepository verification(path);
        const auto paused = verification.findFocusSessionById(sessionId);
        QVERIFY(paused.has_value());
        QCOMPARE(paused->state, FocusSessionState::Paused);
        QVERIFY(verification.findFocusIntervals(sessionId).first().endedAtUtc.has_value());
    }
}

QTEST_GUILESS_MAIN(FocusAppLifecycleTest)
#include "tst_FocusAppLifecycle.moc"
