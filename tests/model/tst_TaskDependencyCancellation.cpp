#include "dependencies/TaskDependencyGraph.h"

#include <QDateTime>
#include <QTest>
#include <QTimeZone>

#include <optional>

using smartmate::model::Task;
using smartmate::model::TaskDependency;
using smartmate::model::TaskDependencyGraph;
using smartmate::model::TaskDependencyResolution;
using smartmate::model::TaskId;
using smartmate::model::TaskPriority;
using smartmate::model::TaskStatus;

namespace {

[[nodiscard]] Task makeTask(
    const TaskId &id,
    const TaskStatus status,
    const std::optional<TaskStatus> statusBeforeArchive = std::nullopt)
{
    const QDateTime now = QDateTime::fromMSecsSinceEpoch(
        1'700'000'000'000, QTimeZone::UTC);
    return {id,
            QStringLiteral("依赖测试"),
            QStringLiteral("取消关系必须保留但不阻塞"),
            TaskPriority::Normal,
            status,
            statusBeforeArchive,
            std::nullopt,
            std::nullopt,
            now,
            now};
}

} // namespace

/// 验证依赖边的三态解析，确保“取消”不会被误当作完成或未满足。
class TaskDependencyCancellationTest final : public QObject {
    Q_OBJECT

private slots:
    void cancelledPredecessorKeepsEdgeButDoesNotBlock();
    void archivedCancelledPredecessorRemainsNonBlocking();
    void andSemanticsOnlyReportsPendingPredecessors();
    void completingOrCancellingTheLastPendingPredecessorUnlocksSuccessor();
};

void TaskDependencyCancellationTest::cancelledPredecessorKeepsEdgeButDoesNotBlock()
{
    const TaskId predecessorId = QUuid::createUuid();
    const TaskId successorId = QUuid::createUuid();
    const Task predecessor = makeTask(predecessorId, TaskStatus::Cancelled);
    const Task successor = makeTask(successorId, TaskStatus::Todo);
    const TaskDependency dependency{predecessorId, successorId};
    const TaskDependencyGraph graph{{predecessor, successor}, {dependency}};

    QVERIFY(graph.validation().ok());
    QCOMPARE(graph.dependencies(), QList<TaskDependency>{dependency});
    QCOMPARE(TaskDependencyGraph::dependencyResolution(predecessor),
             TaskDependencyResolution::Cancelled);
    const auto state = graph.dependencyState(successorId);
    QCOMPARE(state.predecessorIds, QList<TaskId>{predecessorId});
    QVERIFY(state.unsatisfiedPredecessorIds.isEmpty());
    QCOMPARE(state.cancelledPredecessorIds, QList<TaskId>{predecessorId});
    QVERIFY(!state.blocked);
}

void TaskDependencyCancellationTest::archivedCancelledPredecessorRemainsNonBlocking()
{
    const TaskId predecessorId = QUuid::createUuid();
    const TaskId successorId = QUuid::createUuid();
    const Task predecessor = makeTask(predecessorId,
                                      TaskStatus::Archived,
                                      TaskStatus::Cancelled);
    const Task successor = makeTask(successorId, TaskStatus::Todo);
    const TaskDependencyGraph graph{
        {predecessor, successor}, {{predecessorId, successorId}}};

    QCOMPARE(TaskDependencyGraph::dependencyResolution(predecessor),
             TaskDependencyResolution::Cancelled);
    const auto state = graph.dependencyState(successorId);
    QVERIFY(state.unsatisfiedPredecessorIds.isEmpty());
    QCOMPARE(state.cancelledPredecessorIds, QList<TaskId>{predecessorId});
    QVERIFY(!state.blocked);
}

void TaskDependencyCancellationTest::andSemanticsOnlyReportsPendingPredecessors()
{
    const TaskId doneId = QUuid::createUuid();
    const TaskId cancelledId = QUuid::createUuid();
    const TaskId pendingId = QUuid::createUuid();
    const TaskId successorId = QUuid::createUuid();
    const TaskDependencyGraph graph{
        {makeTask(doneId, TaskStatus::Done),
         makeTask(cancelledId, TaskStatus::Cancelled),
         makeTask(pendingId, TaskStatus::Todo),
         makeTask(successorId, TaskStatus::Todo)},
        {{doneId, successorId},
         {cancelledId, successorId},
         {pendingId, successorId}}};

    const auto state = graph.dependencyState(successorId);
    QCOMPARE(state.predecessorIds.size(), 3);
    QCOMPARE(state.unsatisfiedPredecessorIds, QList<TaskId>{pendingId});
    QCOMPARE(state.cancelledPredecessorIds, QList<TaskId>{cancelledId});
    QVERIFY(state.blocked);
}

void TaskDependencyCancellationTest::completingOrCancellingTheLastPendingPredecessorUnlocksSuccessor()
{
    const TaskId predecessorId = QUuid::createUuid();
    const TaskId successorId = QUuid::createUuid();
    const TaskDependencyGraph pendingGraph{
        {makeTask(predecessorId, TaskStatus::Todo),
         makeTask(successorId, TaskStatus::Todo)},
        {{predecessorId, successorId}}};
    QCOMPARE(pendingGraph.dependencyState(predecessorId).unlockCount, 1);

    const TaskDependencyGraph cancelledGraph{
        {makeTask(predecessorId, TaskStatus::Cancelled),
         makeTask(successorId, TaskStatus::Todo)},
        {{predecessorId, successorId}}};
    QVERIFY(!cancelledGraph.dependencyState(successorId).blocked);
    QCOMPARE(cancelledGraph.dependencyState(predecessorId).unlockCount, 0);
}

QTEST_APPLESS_MAIN(TaskDependencyCancellationTest)

#include "tst_TaskDependencyCancellation.moc"
