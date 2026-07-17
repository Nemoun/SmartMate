#include "fakes/FakeFocusSessionRepository.h"
#include "fakes/FakeTaskActivityRepository.h"
#include "fakes/FakeTaskCategoryRepository.h"
#include "fakes/FakeTaskRepository.h"

#include "services/FocusService.h"

#include <QSignalSpy>
#include <QTest>
#include <QTimeZone>

using namespace smartmate::model;
using namespace smartmate::tests;

namespace {

const QDateTime kBase =
    QDateTime::fromMSecsSinceEpoch(1'760'000'000'000LL, QTimeZone::UTC);

Task makeTask(const TaskId &id,
              const TaskStatus status,
              const std::optional<TaskCategoryId> categoryId = std::nullopt)
{
    return {id,
            QStringLiteral("编写专注服务"),
            QStringLiteral("验证自由正计时规则"),
            TaskPriority::High,
            status,
            std::nullopt,
            kBase.addDays(1),
            45,
            kBase.addSecs(-60),
            kBase,
            categoryId};
}

TaskActivityEvent startEvent(const TaskId &taskId,
                             const TaskActivityEventId &eventId)
{
    TaskActivityEvent event;
    event.eventId = eventId;
    event.taskId = taskId;
    event.transition = TaskTransition::Start;
    event.fromStatus = TaskStatus::Todo;
    event.toStatus = TaskStatus::InProgress;
    event.occurredAtUtc = kBase.addSecs(-1);
    event.prioritySnapshot = TaskPriority::High;
    return event;
}

FocusSession seededSession(const TaskId &taskId,
                           const FocusSessionState state,
                           const QDateTime &startedAt,
                           const std::optional<QDateTime> endedAt = std::nullopt)
{
    FocusSession session;
    session.sessionId = QUuid::createUuid();
    session.taskId = taskId;
    session.state = state;
    session.startedAtUtc = startedAt;
    session.endedAtUtc = endedAt;
    session.taskTitleSnapshot = QStringLiteral("编写专注服务");
    session.estimatedMinutesSnapshot = 45;
    return session;
}

} // namespace

class FocusServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void requiresInitializationAndInProgressTask();
    void snapshotProvidesStartCandidateAndAuthoritativeAvailability();
    void snapshotRejectsMultipleInProgressTasks();
    void snapshotsTaskCategoryAndStartEvent();
    void pausesShortIntervalAndRequiresOneSecondToComplete();
    void sumsMultipleIntervalsAndCompletesPausedSession();
    void abandonsWithoutAddingHistory();
    void recoversInterruptedAndDiscardsShortSessions();
    void retriesCheckpointFailureAndCoalescesNotification();
    void preparesShutdownByPausingOrDiscarding();
    void rejectsClockRollbackBehindCheckpoint();
};

void FocusServiceTest::requiresInitializationAndInProgressTask()
{
    const TaskId todoId = QUuid::createUuid();
    FakeTaskRepository tasks{{makeTask(todoId, TaskStatus::Todo)}};
    FakeTaskCategoryRepository categories;
    FakeTaskActivityRepository activities;
    FakeFocusSessionRepository focuses;
    QDateTime now = kBase;
    FocusService service{tasks, categories, activities, focuses,
                         [&now] { return now; }, 10};

    QCOMPARE(service.snapshot().error, FocusError::NotInitialized);
    QVERIFY(service.initialize().ok());
    QCOMPARE(service.startFocus(QUuid::createUuid()).error,
             FocusError::TaskNotFound);
    QCOMPARE(service.startFocus(todoId).error,
             FocusError::TaskNotInProgress);
}

void FocusServiceTest::snapshotProvidesStartCandidateAndAuthoritativeAvailability()
{
    const TaskId taskId = QUuid::createUuid();
    const TaskCategory category{QUuid::createUuid(),
                                QStringLiteral("开发"),
                                TaskCategoryColor::Teal,
                                kBase,
                                kBase};
    FakeTaskRepository tasks{{makeTask(taskId, TaskStatus::InProgress,
                                       category.id)}};
    FakeTaskCategoryRepository categories{{category}};
    FakeTaskActivityRepository activities;
    FakeFocusSessionRepository focuses;
    QDateTime now = kBase;
    FocusService service{tasks, categories, activities, focuses,
                         [&now] { return now; }, 1000};
    QVERIFY(service.initialize().ok());

    const auto ready = service.snapshot(0);
    QVERIFY(ready.ok());
    QVERIFY(ready.value->startCandidate.has_value());
    QCOMPARE(ready.value->startCandidate->taskId, taskId);
    QCOMPARE(ready.value->startCandidate->taskTitle,
             QStringLiteral("编写专注服务"));
    QCOMPARE(ready.value->startCandidate->estimatedMinutes,
             std::optional{45});
    QCOMPARE(ready.value->startCandidate->categoryName,
             std::optional{category.name});
    QVERIFY(ready.value->availability.canStart);
    QVERIFY(!ready.value->availability.canPause);
    QVERIFY(!ready.value->availability.canComplete);

    const FocusSessionId sessionId =
        service.startFocus(taskId).value->session.sessionId;
    now = kBase.addMSecs(500);
    const auto shortRunning = service.snapshot(0);
    QVERIFY(shortRunning.ok());
    QVERIFY(!shortRunning.value->startCandidate.has_value());
    QVERIFY(shortRunning.value->availability.canPause);
    QVERIFY(shortRunning.value->availability.canAbandon);
    QVERIFY(!shortRunning.value->availability.canComplete);

    now = kBase.addMSecs(1'500);
    const auto completable = service.snapshot(0);
    QVERIFY(completable.value->availability.canComplete);
    QVERIFY(service.pauseFocus(sessionId).ok());
    const auto paused = service.snapshot(0);
    QVERIFY(paused.value->availability.canResume);
    QVERIFY(paused.value->availability.canComplete);
    QVERIFY(!paused.value->availability.canPause);
}

void FocusServiceTest::snapshotRejectsMultipleInProgressTasks()
{
    const TaskId firstId = QUuid::createUuid();
    const TaskId secondId = QUuid::createUuid();
    FakeTaskRepository tasks{{makeTask(firstId, TaskStatus::InProgress),
                              makeTask(secondId, TaskStatus::InProgress)}};
    FakeTaskCategoryRepository categories;
    FakeTaskActivityRepository activities;
    FakeFocusSessionRepository focuses;
    QDateTime now = kBase;
    FocusService service{tasks, categories, activities, focuses,
                         [&now] { return now; }, 1000};
    QVERIFY(service.initialize().ok());

    const auto snapshot = service.snapshot();
    QCOMPARE(snapshot.error, FocusError::StateConflict);
    QVERIFY(!snapshot.value.has_value());
}

void FocusServiceTest::snapshotsTaskCategoryAndStartEvent()
{
    const TaskId taskId = QUuid::createUuid();
    const TaskCategory category{QUuid::createUuid(),
                                QStringLiteral("开发"),
                                TaskCategoryColor::Violet,
                                kBase,
                                kBase};
    const TaskActivityEventId eventId = QUuid::createUuid();
    FakeTaskRepository tasks{{makeTask(taskId, TaskStatus::InProgress, category.id)}};
    FakeTaskCategoryRepository categories{{category}};
    FakeTaskActivityRepository activities{{startEvent(taskId, eventId)}};
    FakeFocusSessionRepository focuses;
    QDateTime now = kBase;
    FocusService service{tasks, categories, activities, focuses,
                         [&now] { return now; }, 10};
    QSignalSpy changed{&service, &FocusService::focusChanged};
    QVERIFY(service.initialize().ok());

    const auto started = service.startFocus(taskId);
    QVERIFY(started.ok());
    QCOMPARE(started.value->session.taskId, taskId);
    QCOMPARE(started.value->session.taskTitleSnapshot,
             QStringLiteral("编写专注服务"));
    QCOMPARE(started.value->session.estimatedMinutesSnapshot,
             std::optional{45});
    QCOMPARE(started.value->session.categoryIdSnapshot,
             std::optional{category.id});
    QCOMPARE(started.value->session.categoryNameSnapshot,
             std::optional{category.name});
    QCOMPARE(started.value->session.categoryColorSnapshot,
             std::optional{category.color});
    QCOMPARE(started.value->session.taskStartEventId,
             std::optional{eventId});
    QCOMPARE(changed.count(), 1);
    QCOMPARE(service.startFocus(taskId).error, FocusError::ActiveSessionExists);
}

void FocusServiceTest::pausesShortIntervalAndRequiresOneSecondToComplete()
{
    const TaskId taskId = QUuid::createUuid();
    FakeTaskRepository tasks{{makeTask(taskId, TaskStatus::InProgress)}};
    FakeTaskCategoryRepository categories;
    FakeTaskActivityRepository activities;
    FakeFocusSessionRepository focuses;
    QDateTime now = kBase;
    FocusService service{tasks, categories, activities, focuses,
                         [&now] { return now; }, 1000};
    QVERIFY(service.initialize().ok());
    const auto started = service.startFocus(taskId);
    QVERIFY(started.ok());
    const FocusSessionId id = started.value->session.sessionId;

    now = kBase.addMSecs(400);
    const auto paused = service.pauseFocus(id);
    QVERIFY(paused.ok());
    QCOMPARE(paused.value->focusedMilliseconds, 400);
    QCOMPARE(service.completeFocus(id).error,
             FocusError::MinimumDurationNotReached);
    QCOMPARE(focuses.completeCalls(), 0);
}

void FocusServiceTest::sumsMultipleIntervalsAndCompletesPausedSession()
{
    const TaskId taskId = QUuid::createUuid();
    FakeTaskRepository tasks{{makeTask(taskId, TaskStatus::InProgress)}};
    FakeTaskCategoryRepository categories;
    FakeTaskActivityRepository activities;
    FakeFocusSessionRepository focuses;
    QDateTime now = kBase;
    FocusService service{tasks, categories, activities, focuses,
                         [&now] { return now; }, 1000};
    QSignalSpy history{&service, &FocusService::historyChanged};
    QVERIFY(service.initialize().ok());
    const FocusSessionId id = service.startFocus(taskId).value->session.sessionId;
    now = kBase.addMSecs(600);
    QVERIFY(service.pauseFocus(id).ok());
    now = kBase.addMSecs(900);
    QVERIFY(service.resumeFocus(id).ok());
    now = kBase.addMSecs(1'500);
    const auto paused = service.pauseFocus(id);
    QVERIFY(paused.ok());
    QCOMPARE(paused.value->focusedMilliseconds, 1'200);
    now = kBase.addMSecs(2'000);
    const auto completed = service.completeFocus(id);
    QVERIFY(completed.ok());
    QCOMPARE(completed.value->session.state, FocusSessionState::Completed);
    QCOMPARE(completed.value->focusedMilliseconds, 1'200);
    QCOMPARE(history.count(), 1);
    QCOMPARE(service.snapshot().value->recentCompletedRecords.size(), 1);
}

void FocusServiceTest::abandonsWithoutAddingHistory()
{
    const TaskId taskId = QUuid::createUuid();
    FakeTaskRepository tasks{{makeTask(taskId, TaskStatus::InProgress)}};
    FakeTaskCategoryRepository categories;
    FakeTaskActivityRepository activities;
    FakeFocusSessionRepository focuses;
    QDateTime now = kBase;
    FocusService service{tasks, categories, activities, focuses,
                         [&now] { return now; }, 1000};
    QSignalSpy history{&service, &FocusService::historyChanged};
    QVERIFY(service.initialize().ok());
    const FocusSessionId id = service.startFocus(taskId).value->session.sessionId;
    now = kBase.addSecs(2);
    const auto abandoned = service.abandonFocus(id);
    QVERIFY(abandoned.ok());
    QVERIFY(!service.snapshot().value->activeRecord.has_value());
    QVERIFY(service.snapshot().value->recentCompletedRecords.isEmpty());
    QCOMPARE(history.count(), 0);
}

void FocusServiceTest::recoversInterruptedAndDiscardsShortSessions()
{
    const TaskId taskId = QUuid::createUuid();
    FakeTaskRepository tasks{{makeTask(taskId, TaskStatus::InProgress)}};
    FakeTaskCategoryRepository categories;
    FakeTaskActivityRepository activities;
    QDateTime now = kBase.addSecs(10);

    FakeFocusSessionRepository retainedFocus;
    FocusSession retained = seededSession(taskId, FocusSessionState::Running, kBase);
    retainedFocus.seed(retained,
                       {{retained.sessionId, 0, kBase, std::nullopt,
                         kBase.addSecs(2)}});
    FocusService retainedService{tasks, categories, activities, retainedFocus,
                                 [&now] { return now; }, 10};
    QVERIFY(retainedService.initialize().ok());
    QCOMPARE(retainedFocus.recoverCalls(), 1);
    const auto retainedSnapshot = retainedService.snapshot();
    QVERIFY(retainedSnapshot.value->activeRecord.has_value());
    QCOMPARE(retainedSnapshot.value->activeRecord->session.state,
             FocusSessionState::Paused);
    QCOMPARE(retainedSnapshot.value->activeRecord->focusedMilliseconds, 2'000);

    FakeFocusSessionRepository shortFocus;
    FocusSession shortSession = seededSession(taskId, FocusSessionState::Running, kBase);
    shortFocus.seed(shortSession,
                    {{shortSession.sessionId, 0, kBase, std::nullopt,
                      kBase.addMSecs(500)}});
    FocusService shortService{tasks, categories, activities, shortFocus,
                              [&now] { return now; }, 10};
    QVERIFY(shortService.initialize().ok());
    QVERIFY(!shortService.snapshot().value->activeRecord.has_value());
    QCOMPARE(shortFocus.abandonCalls(), 1);
}

void FocusServiceTest::retriesCheckpointFailureAndCoalescesNotification()
{
    const TaskId taskId = QUuid::createUuid();
    FakeTaskRepository tasks{{makeTask(taskId, TaskStatus::InProgress)}};
    FakeTaskCategoryRepository categories;
    FakeTaskActivityRepository activities;
    FakeFocusSessionRepository focuses;
    QDateTime now = kBase;
    FocusService service{tasks, categories, activities, focuses,
                         [&now] { return now; }, 5};
    QSignalSpy failure{&service, &FocusService::backgroundFailureRaised};
    QSignalSpy cleared{&service, &FocusService::backgroundFailureCleared};
    QVERIFY(service.initialize().ok());
    QVERIFY(service.startFocus(taskId).ok());
    focuses.setCheckpointFailures(2);
    now = kBase.addSecs(1);

    QTRY_VERIFY_WITH_TIMEOUT(focuses.checkpointCalls() >= 3, 300);
    QCOMPARE(failure.count(), 1);
    QCOMPARE(cleared.count(), 1);
}

void FocusServiceTest::preparesShutdownByPausingOrDiscarding()
{
    const TaskId taskId = QUuid::createUuid();
    FakeTaskRepository tasks{{makeTask(taskId, TaskStatus::InProgress)}};
    FakeTaskCategoryRepository categories;
    FakeTaskActivityRepository activities;
    QDateTime now = kBase;

    FakeFocusSessionRepository retained;
    FocusService retainedService{tasks, categories, activities, retained,
                                 [&now] { return now; }, 1000};
    QVERIFY(retainedService.initialize().ok());
    const auto retainedId = retainedService.startFocus(taskId).value->session.sessionId;
    now = kBase.addMSecs(1'500);
    QVERIFY(retainedService.prepareForShutdown().ok());
    QCOMPARE(retained.findFocusSessionById(retainedId)->state,
             FocusSessionState::Paused);
    QCOMPARE(retained.pauseCalls(), 1);
    QVERIFY(retainedService.prepareForShutdown().ok());
    QCOMPARE(retained.pauseCalls(), 1);

    now = kBase;
    FakeFocusSessionRepository discarded;
    FocusService discardedService{tasks, categories, activities, discarded,
                                  [&now] { return now; }, 1000};
    QVERIFY(discardedService.initialize().ok());
    const auto discardedId =
        discardedService.startFocus(taskId).value->session.sessionId;
    now = kBase.addMSecs(500);
    QVERIFY(discardedService.pauseFocus(discardedId).ok());
    QVERIFY(discardedService.prepareForShutdown().ok());
    QVERIFY(!discarded.findFocusSessionById(discardedId).has_value());
}

void FocusServiceTest::rejectsClockRollbackBehindCheckpoint()
{
    const TaskId taskId = QUuid::createUuid();
    FakeTaskRepository tasks{{makeTask(taskId, TaskStatus::InProgress)}};
    FakeTaskCategoryRepository categories;
    FakeTaskActivityRepository activities;
    FakeFocusSessionRepository focuses;
    QDateTime now = kBase;
    FocusService service{tasks, categories, activities, focuses,
                         [&now] { return now; }, 1000};
    QVERIFY(service.initialize().ok());
    const auto id = service.startFocus(taskId).value->session.sessionId;
    now = kBase.addSecs(2);
    QVERIFY(service.writeCheckpoint().ok());
    now = kBase.addSecs(1);
    QCOMPARE(service.completeFocus(id).error, FocusError::StateConflict);
}

QTEST_MAIN(FocusServiceTest)
#include "tst_FocusService.moc"
