#include "AppViewModel.h"
#include "FocusProjectionModels.h"
#include "FocusViewModel.h"
#include "fakes/FakeFocusSessionRepository.h"
#include "fakes/FakeTaskActivityRepository.h"
#include "fakes/FakeTaskBatchTransitionRepository.h"
#include "fakes/FakeTaskCategoryRepository.h"
#include "fakes/FakeTaskCreationRepository.h"
#include "fakes/FakeTaskDeletionRepository.h"
#include "fakes/FakeTaskDependencyRepository.h"
#include "fakes/FakeTaskRepository.h"
#include "services/FocusService.h"
#include "services/TaskService.h"
#include "viewmodel/contracts/FocusContract.h"
#include "viewmodel/contracts/TaskListContract.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QTimeZone>

using namespace smartmate;

namespace {

const QDateTime kBase =
    QDateTime::fromMSecsSinceEpoch(1'760'000'000'000LL, QTimeZone::UTC);

model::Task makeInProgressTask(const model::TaskId &id,
                               const model::TaskCategoryId &categoryId)
{
    return {id,
            QStringLiteral("完成专注页面"),
            QStringLiteral("实现 ViewModel 投影"),
            model::TaskPriority::High,
            model::TaskStatus::InProgress,
            std::nullopt,
            kBase.addDays(1),
            45,
            kBase.addSecs(-30),
            kBase,
            categoryId};
}

struct Fixture final {
    QDateTime now{kBase};
    model::TaskId taskId{QUuid::createUuid()};
    model::TaskCategory category{QUuid::createUuid(),
                                 QStringLiteral("开发"),
                                 model::TaskCategoryColor::Violet,
                                 kBase,
                                 kBase};
    tests::FakeTaskRepository tasks{{makeInProgressTask(taskId, category.id)}};
    tests::FakeTaskDependencyRepository dependencies;
    tests::FakeTaskCreationRepository creation{tasks, dependencies};
    tests::FakeTaskBatchTransitionRepository transitions{tasks};
    tests::FakeTaskDeletionRepository deletion{tasks, dependencies};
    tests::FakeTaskCategoryRepository categories{{category}};
    tests::FakeTaskActivityRepository activities;
    tests::FakeFocusSessionRepository focuses;
    model::TaskService taskService{tasks, dependencies, creation, transitions,
                                   deletion, categories, &focuses};
    model::FocusService focusService{tasks, categories, activities, focuses,
                                     [this] { return now; }, 5};

    Fixture()
    {
        QVERIFY(focusService.initialize().ok());
    }

    void seedCompleted(qint64 durationMs = 2'000)
    {
        model::FocusSession session;
        session.sessionId = QUuid::createUuid();
        session.taskId = taskId;
        session.state = model::FocusSessionState::Completed;
        session.startedAtUtc = kBase.addDays(-1);
        session.endedAtUtc = session.startedAtUtc.addMSecs(durationMs);
        session.taskTitleSnapshot = QStringLiteral("历史专注任务");
        session.estimatedMinutesSnapshot = 30;
        session.categoryIdSnapshot = category.id;
        session.categoryNameSnapshot = category.name;
        session.categoryColorSnapshot = category.color;
        focuses.seed(session,
                     {{session.sessionId, 0, session.startedAtUtc,
                       session.endedAtUtc, *session.endedAtUtc}});
    }
};

} // namespace

class FocusViewModelTest final : public QObject {
    Q_OBJECT

private slots:
    void projectsNoTaskAndUnclassifiedCandidate();
    void projectsStatesAndExecutesStableIdCommands();
    void updatesRunningClockEachSecondWithoutWrappingAtOneDay();
    void projectsStableHistoryRolesInLocalTime();
    void limitsHistoryAndAvoidsRedundantNotifications();
    void coalescesReadAndCheckpointFailureNotifications();
    void appViewModelOwnsFocusAndRefreshesTaskAvailability();
};

void FocusViewModelTest::projectsNoTaskAndUnclassifiedCandidate()
{
    tests::FakeTaskRepository emptyTasks;
    tests::FakeTaskCategoryRepository categories;
    tests::FakeTaskActivityRepository activities;
    tests::FakeFocusSessionRepository focuses;
    QDateTime now = kBase;
    model::FocusService emptyService{emptyTasks, categories, activities, focuses,
                                     [&now] { return now; }, 1000};
    QVERIFY(emptyService.initialize().ok());
    viewmodel::FocusViewModel emptyViewModel{emptyService};
    QCOMPARE(emptyViewModel.pageState(),
             viewmodel::FocusContract::NoInProgressTask);
    QCOMPARE(emptyViewModel.emptyStateText(),
             QStringLiteral("请先在任务页开始一项任务。"));
    QCOMPARE(emptyViewModel.historyEmptyStateText(),
             QStringLiteral("完成一次专注后，这里会显示记录。"));

    const model::TaskId taskId = QUuid::createUuid();
    model::Task bareTask{taskId,
                         QStringLiteral("未分类任务"),
                         QString{},
                         model::TaskPriority::Normal,
                         model::TaskStatus::InProgress,
                         std::nullopt,
                         std::nullopt,
                         std::nullopt,
                         kBase,
                         kBase,
                         std::nullopt};
    tests::FakeTaskRepository bareTasks{{bareTask}};
    tests::FakeFocusSessionRepository bareFocuses;
    model::FocusService bareService{bareTasks, categories, activities,
                                    bareFocuses, [&now] { return now; }, 1000};
    QVERIFY(bareService.initialize().ok());
    viewmodel::FocusViewModel bareViewModel{bareService};
    QCOMPARE(bareViewModel.pageState(), viewmodel::FocusContract::ReadyToStart);
    QVERIFY(!bareViewModel.hasCategory());
    QCOMPARE(bareViewModel.categoryName(), QStringLiteral("未分类"));
    QCOMPARE(bareViewModel.categoryColor(),
             viewmodel::FocusContract::Unclassified);
    QVERIFY(!bareViewModel.hasEstimatedMinutes());
    QCOMPARE(bareViewModel.estimatedText(),
             QStringLiteral("未设置预计用时"));
}

void FocusViewModelTest::projectsStatesAndExecutesStableIdCommands()
{
    Fixture fixture;
    const QTimeZone zone{"Asia/Shanghai"};
    viewmodel::FocusViewModel viewModel{
        fixture.focusService, [&zone] { return zone; }, 1000};
    QAbstractItemModelTester tester{
        viewModel.history(),
        QAbstractItemModelTester::FailureReportingMode::QtTest};
    QSignalSpy notifications{&viewModel,
        &viewmodel::FocusContract::notificationRaised};

    QCOMPARE(viewModel.pageState(), viewmodel::FocusContract::ReadyToStart);
    QCOMPARE(viewModel.taskId(),
             fixture.taskId.toString(QUuid::WithoutBraces));
    QCOMPARE(viewModel.taskTitle(), QStringLiteral("完成专注页面"));
    QCOMPARE(viewModel.categoryName(), QStringLiteral("开发"));
    QCOMPARE(viewModel.categoryColor(), viewmodel::FocusContract::Violet);
    QCOMPARE(viewModel.estimatedText(), QStringLiteral("预计 45 分钟"));
    QVERIFY(viewModel.canStart());
    QVERIFY(!viewModel.startFocus(QStringLiteral("invalid")));
    QCOMPARE(notifications.count(), 1);

    QVERIFY(viewModel.startFocus(viewModel.taskId()));
    QCOMPARE(viewModel.pageState(), viewmodel::FocusContract::Running);
    QVERIFY(viewModel.canPause());
    QVERIFY(viewModel.canAbandon());
    const QString sessionId = viewModel.sessionId();
    QVERIFY(!sessionId.isEmpty());

    fixture.now = kBase.addMSecs(400);
    viewModel.reload();
    QCOMPARE(viewModel.elapsedMilliseconds(), 400);
    QVERIFY(!viewModel.canComplete());
    QVERIFY(viewModel.pauseFocus(sessionId));
    QCOMPARE(viewModel.pageState(), viewmodel::FocusContract::Paused);
    QVERIFY(viewModel.canResume());
    QVERIFY(!viewModel.completeFocus(sessionId));

    fixture.now = kBase.addSecs(2);
    QVERIFY(viewModel.resumeFocus(sessionId));
    fixture.now = kBase.addMSecs(2'700);
    viewModel.reload();
    QCOMPARE(viewModel.elapsedMilliseconds(), 1'100);
    QVERIFY(viewModel.canComplete());
    QVERIFY(viewModel.completeFocus(sessionId));
    QCOMPARE(viewModel.pageState(), viewmodel::FocusContract::ReadyToStart);
    QCOMPARE(viewModel.historyCount(), 1);

    fixture.now = kBase.addSecs(3);
    QVERIFY(viewModel.startFocus(viewModel.taskId()));
    const QString secondSession = viewModel.sessionId();
    fixture.now = kBase.addSecs(4);
    QVERIFY(viewModel.abandonFocus(secondSession));
    QCOMPARE(viewModel.historyCount(), 1);
}

void FocusViewModelTest::updatesRunningClockEachSecondWithoutWrappingAtOneDay()
{
    Fixture fixture;
    viewmodel::FocusViewModel viewModel{
        fixture.focusService,
        [] { return QTimeZone{QTimeZone::UTC}; },
        5};
    QVERIFY(viewModel.startFocus(viewModel.taskId()));
    QTimer *displayTimer = viewModel.findChild<QTimer *>(
        QStringLiteral("focusDisplayTimer"));
    QVERIFY(displayTimer != nullptr);
    QVERIFY(displayTimer->isSingleShot());
    QVERIFY(displayTimer->isActive());

    fixture.now = kBase.addSecs(25 * 3'600 + 61);
    QTRY_COMPARE_WITH_TIMEOUT(viewModel.elapsedText(),
                              QStringLiteral("25:01:01"), 300);
    QVERIFY(viewModel.canComplete());
    QVERIFY(viewModel.pauseFocus(viewModel.sessionId()));
    QVERIFY(!displayTimer->isActive());
    const QString pausedText = viewModel.elapsedText();
    fixture.now = fixture.now.addSecs(3 * 3'600);
    QTest::qWait(20);
    QCOMPARE(viewModel.elapsedText(), pausedText);
}

void FocusViewModelTest::projectsStableHistoryRolesInLocalTime()
{
    Fixture fixture;
    fixture.seedCompleted(25LL * 60 * 60 * 1000);
    const QTimeZone zone{"Asia/Shanghai"};
    viewmodel::FocusViewModel viewModel{
        fixture.focusService, [&zone] { return zone; }, 1000};
    QCOMPARE(viewModel.historyCount(), 1);
    QVERIFY(viewModel.hasHistory());
    QCOMPARE(viewModel.historyEmptyStateText(), QString{});

    QAbstractItemModel *history = viewModel.history();
    const QModelIndex row = history->index(0, 0);
    QCOMPARE(history->data(row, viewmodel::FocusHistoryContract::TaskTitleRole)
                 .toString(),
             QStringLiteral("历史专注任务"));
    QCOMPARE(history->data(row, viewmodel::FocusHistoryContract::DurationTextRole)
                 .toString(),
             QStringLiteral("25:00:00"));
    QCOMPARE(history->data(row, viewmodel::FocusHistoryContract::CategoryNameRole)
                 .toString(),
             QStringLiteral("开发"));
    QCOMPARE(history->data(row, viewmodel::FocusHistoryContract::CategoryColorRole)
                 .toInt(),
             static_cast<int>(viewmodel::FocusContract::Violet));
    QVERIFY(history->data(row, viewmodel::FocusHistoryContract::StartedAtTextRole)
                .toString().contains(QLatin1Char(':')));
    QVERIFY(history->data(row, viewmodel::FocusHistoryContract::TooltipRole)
                .toString().contains(QStringLiteral("25:00:00")));
    QVERIFY(!history->data(row,
        viewmodel::FocusHistoryContract::AccessibleTextRole).toString().isEmpty());
}

void FocusViewModelTest::limitsHistoryAndAvoidsRedundantNotifications()
{
    Fixture fixture;
    for (int index = 0; index < 55; ++index) {
        fixture.seedCompleted(1'000 + index);
    }
    viewmodel::FocusViewModel viewModel{
        fixture.focusService,
        [] { return QTimeZone{QTimeZone::UTC}; },
        1000};
    QCOMPARE(viewModel.historyCount(), 50);

    QSignalSpy focusChanges{&viewModel,
        &viewmodel::FocusContract::focusChanged};
    QSignalSpy historyChanges{&viewModel,
        &viewmodel::FocusContract::historyChanged};
    QSignalSpy resets{viewModel.history(), &QAbstractItemModel::modelReset};
    viewModel.reload();
    QCOMPARE(focusChanges.count(), 0);
    QCOMPARE(historyChanges.count(), 0);
    QCOMPARE(resets.count(), 0);
}

void FocusViewModelTest::coalescesReadAndCheckpointFailureNotifications()
{
    Fixture fixture;
    viewmodel::FocusViewModel viewModel{
        fixture.focusService,
        [] { return QTimeZone{QTimeZone::UTC}; },
        5};
    QSignalSpy notifications{&viewModel,
        &viewmodel::FocusContract::notificationRaised};
    QSignalSpy warningChanges{&viewModel,
        &viewmodel::FocusContract::storageWarningChanged};

    fixture.focuses.setReadFailure(true);
    viewModel.reload();
    viewModel.reload();
    QVERIFY(viewModel.hasStorageWarning());
    QCOMPARE(notifications.count(), 1);
    fixture.focuses.setReadFailure(false);
    viewModel.reload();
    QVERIFY(!viewModel.hasStorageWarning());
    QCOMPARE(notifications.count(), 2);

    QVERIFY(viewModel.startFocus(viewModel.taskId()));
    fixture.focuses.setCheckpointFailures(100);
    fixture.now = kBase.addSecs(1);
    QTRY_VERIFY_WITH_TIMEOUT(viewModel.hasStorageWarning(), 300);
    QCOMPARE(notifications.count(), 3);
    fixture.focuses.setCheckpointFailures(0);
    QTRY_VERIFY_WITH_TIMEOUT(!viewModel.hasStorageWarning(), 300);
    QCOMPARE(notifications.count(), 4);
    QVERIFY(warningChanges.count() >= 4);
}

void FocusViewModelTest::appViewModelOwnsFocusAndRefreshesTaskAvailability()
{
    Fixture fixture;
    viewmodel::AppViewModel compatible{fixture.taskService};
    QCOMPARE(compatible.focus(), nullptr);

    viewmodel::AppViewModel app{fixture.taskService, fixture.focusService};
    QVERIFY(app.focus() != nullptr);
    QCOMPARE(app.focus(), app.focus());
    QModelIndex taskRow = app.taskList()->index(0, 0);
    QVERIFY(app.taskList()->data(
        taskRow, viewmodel::TaskListContract::CanCompleteRole).toBool());

    QVERIFY(app.focus()->startFocus(app.focus()->taskId()));
    taskRow = app.taskList()->index(0, 0);
    QVERIFY(!app.taskList()->data(
        taskRow, viewmodel::TaskListContract::CanCompleteRole).toBool());
    QVERIFY(!app.taskList()->data(
        taskRow, viewmodel::TaskListContract::CanCancelRole).toBool());
}

QTEST_GUILESS_MAIN(FocusViewModelTest)
#include "tst_FocusViewModel.moc"
