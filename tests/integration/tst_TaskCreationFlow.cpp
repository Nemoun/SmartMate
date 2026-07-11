#include "AppViewModel.h"
#include "TaskEditorViewModel.h"
#include "TaskListViewModel.h"
#include "persistence/SqliteTaskRepository.h"
#include "services/TaskService.h"

#include <QSignalSpy>
#include <QTest>
#include <QUuid>

using smartmate::model::TaskService;
using smartmate::model::persistence::SqliteTaskRepository;
using smartmate::viewmodel::AppViewModel;
using smartmate::viewmodel::TaskListViewModel;

/// 覆盖真实 SQLite 纵向链路，防止各层独立测试遗漏默认表单值的存储差异。
class TaskCreationFlowTest final : public QObject {
    Q_OBJECT

private slots:
    void createsAndReopensTaskWhenOptionalDescriptionIsUntouched();
    void derivedSearchAndOrderingDoNotModifyStoredTasks();
    void finishToStartDependencyBlocksAndUnlocksThroughFullStack();
};

void TaskCreationFlowTest::createsAndReopensTaskWhenOptionalDescriptionIsUntouched()
{
    SqliteTaskRepository repository{QStringLiteral(":memory:")};
    TaskService service{repository, repository};
    AppViewModel appViewModel{service};
    auto *editor = appViewModel.taskEditor();
    auto *taskList = appViewModel.taskList();
    QSignalSpy listErrorSpy{taskList, &TaskListViewModel::errorOccurred};
    QSignalSpy changedSpy{&service, &TaskService::tasksChanged};

    // 精确模拟用户只填写标题、从未操作可选描述框的创建路径。
    editor->beginCreate();
    editor->setTitle(QStringLiteral("只填写标题也能保存"));

    QVERIFY2(editor->save(), qPrintable(editor->errorMessage()));
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(listErrorSpy.count(), 0);
    QCOMPARE(taskList->count(), 1);

    const QModelIndex firstTask = taskList->index(0);
    const QString taskId =
        taskList->data(firstTask, TaskListViewModel::TaskIdRole).toString();
    QVERIFY(!QUuid::fromString(taskId).isNull());
    QVERIFY(editor->beginEdit(taskId));
    QCOMPARE(editor->title(), QStringLiteral("只填写标题也能保存"));
    QVERIFY(editor->description().isEmpty());

    const auto stored = repository.findById(QUuid::fromString(taskId));
    QVERIFY(stored.has_value());
    QVERIFY(stored->description().isEmpty());
    QVERIFY(!stored->description().isNull());
}

void TaskCreationFlowTest::derivedSearchAndOrderingDoNotModifyStoredTasks()
{
    SqliteTaskRepository repository{QStringLiteral(":memory:")};
    TaskService service{repository, repository};

    smartmate::model::TaskDraft urgentDraft;
    urgentDraft.title = QStringLiteral("准备课程答辩");
    urgentDraft.description = QStringLiteral("整理架构说明");
    urgentDraft.priority = smartmate::model::TaskPriority::Urgent;
    QVERIFY(service.createTask(urgentDraft).ok());

    smartmate::model::TaskDraft lowDraft;
    lowDraft.title = QStringLiteral("清理草稿");
    lowDraft.priority = smartmate::model::TaskPriority::Low;
    QVERIFY(service.createTask(lowDraft).ok());

    AppViewModel appViewModel{service};
    auto *taskList = appViewModel.taskList();
    const auto before = repository.findAll();
    QSignalSpy changedSpy{&service, &TaskService::tasksChanged};

    taskList->setSearchText(QStringLiteral("架构"));
    taskList->setPriorityFilterIndex(4);
    QCOMPARE(taskList->count(), 1);
    taskList->clearFilters();
    taskList->reload();

    const auto after = repository.findAll();
    QCOMPARE(changedSpy.count(), 0);
    QVERIFY(after == before);
}

void TaskCreationFlowTest::finishToStartDependencyBlocksAndUnlocksThroughFullStack()
{
    SqliteTaskRepository repository{QStringLiteral(":memory:")};
    TaskService service{repository, repository};

    smartmate::model::TaskDraft predecessorDraft;
    predecessorDraft.title = QStringLiteral("完成需求分析");
    const auto predecessorResult = service.createTask(predecessorDraft);
    QVERIFY(predecessorResult.ok());

    smartmate::model::TaskDraft successorDraft;
    successorDraft.title = QStringLiteral("实现任务模块");
    const auto successorResult = service.createTask(successorDraft);
    QVERIFY(successorResult.ok());

    const auto dependencyResult = service.replaceTaskPredecessors(
        successorResult.value->id(), {predecessorResult.value->id()});
    QVERIFY(dependencyResult.ok());

    AppViewModel appViewModel{service};
    auto *taskList = appViewModel.taskList();
    const auto rowForTask = [taskList](const smartmate::model::TaskId &taskId) {
        const QString expected = taskId.toString(QUuid::WithoutBraces);
        for (int row = 0; row < taskList->rowCount(); ++row) {
            if (taskList->data(taskList->index(row), TaskListViewModel::TaskIdRole)
                    .toString()
                == expected) {
                return row;
            }
        }
        return -1;
    };

    int predecessorRow = rowForTask(predecessorResult.value->id());
    int successorRow = rowForTask(successorResult.value->id());
    QVERIFY(predecessorRow >= 0);
    QVERIFY(successorRow >= 0);
    QVERIFY(taskList->data(taskList->index(successorRow),
                           TaskListViewModel::BlockedRole).toBool());
    QCOMPARE(taskList->data(taskList->index(successorRow),
                            TaskListViewModel::PredecessorCountRole).toInt(), 1);
    QCOMPARE(taskList->data(taskList->index(predecessorRow),
                            TaskListViewModel::UnlockCountRole).toInt(), 1);
    QVERIFY(taskList->data(taskList->index(successorRow),
                           TaskListViewModel::BlockingReasonTextRole)
                .toString()
                .contains(predecessorDraft.title));

    successorDraft.status = smartmate::model::TaskStatus::InProgress;
    const auto blockedStart = service.updateTask(successorResult.value->id(),
                                                 successorDraft);
    QCOMPARE(blockedStart.error, smartmate::model::TaskError::TaskBlocked);

    predecessorDraft.status = smartmate::model::TaskStatus::Done;
    QVERIFY(service.updateTask(predecessorResult.value->id(), predecessorDraft).ok());
    successorRow = rowForTask(successorResult.value->id());
    QVERIFY(successorRow >= 0);
    QVERIFY(!taskList->data(taskList->index(successorRow),
                            TaskListViewModel::BlockedRole).toBool());

    QVERIFY(service.updateTask(successorResult.value->id(), successorDraft).ok());
    predecessorDraft.status = smartmate::model::TaskStatus::Todo;
    const auto invalidRegression = service.updateTask(
        predecessorResult.value->id(), predecessorDraft);
    QCOMPARE(invalidRegression.error,
             smartmate::model::TaskError::DependencyStateConflict);
    QVERIFY(invalidRegression.context.conflictingTaskIds.contains(
        successorResult.value->id()));
}

// Qt SQL 连接依赖 QCoreApplication 生命周期，测试无需 GUI 事件环境。
QTEST_GUILESS_MAIN(TaskCreationFlowTest)

#include "tst_TaskCreationFlow.moc"
