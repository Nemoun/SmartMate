#include "domain/TaskStateMachine.h"

#include <QDateTime>
#include <QTest>
#include <QTimeZone>

#include <algorithm>
#include <optional>
#include <utility>

using smartmate::model::Task;
using smartmate::model::TaskPriority;
using smartmate::model::TaskStateMachine;
using smartmate::model::TaskStatus;
using smartmate::model::TaskTransition;

namespace {

[[nodiscard]] Task taskWithStatus(
    const TaskStatus status,
    const std::optional<TaskStatus> statusBeforeArchive = std::nullopt)
{
    const QDateTime now = QDateTime::fromMSecsSinceEpoch(
        1'700'000'000'000, QTimeZone::UTC);
    return {QUuid::createUuid(),
            QStringLiteral("状态机测试任务"),
            QStringLiteral("验证显式命令，不通过编辑草稿改变状态"),
            TaskPriority::Normal,
            status,
            statusBeforeArchive,
            std::nullopt,
            std::nullopt,
            now,
            now};
}

struct TransitionCase final {
    TaskStatus source;
    TaskTransition transition;
    std::optional<TaskStatus> target;
};

} // namespace

/// 状态机测试只验证纯领域转换表；依赖和单进行中约束由 Service 测试负责。
class TaskStateMachineTest final : public QObject {
    Q_OBJECT

private slots:
    void acceptsOnlyTheExplicitTransitionMatrix();
    void restoresTerminalArchivesAndNormalizesLegacyArchives();
};

void TaskStateMachineTest::acceptsOnlyTheExplicitTransitionMatrix()
{
    const QList<TransitionCase> allowed{
        {TaskStatus::Todo, TaskTransition::Start, TaskStatus::InProgress},
        {TaskStatus::Todo, TaskTransition::Cancel, TaskStatus::Cancelled},
        {TaskStatus::InProgress, TaskTransition::Cancel, TaskStatus::Cancelled},
        {TaskStatus::InProgress, TaskTransition::Complete, TaskStatus::Done},
        {TaskStatus::Done, TaskTransition::Redo, TaskStatus::Todo},
        {TaskStatus::Done, TaskTransition::Archive, TaskStatus::Archived},
        {TaskStatus::Cancelled, TaskTransition::Redo, TaskStatus::Todo},
        {TaskStatus::Cancelled, TaskTransition::Archive, TaskStatus::Archived},
        {TaskStatus::Archived, TaskTransition::Restore, TaskStatus::Todo},
    };
    const QList<TaskStatus> statuses{TaskStatus::Todo,
                                     TaskStatus::InProgress,
                                     TaskStatus::Done,
                                     TaskStatus::Cancelled,
                                     TaskStatus::Archived};
    const QList<TaskTransition> transitions{TaskTransition::Start,
                                            TaskTransition::Cancel,
                                            TaskTransition::Complete,
                                            TaskTransition::Redo,
                                            TaskTransition::Archive,
                                            TaskTransition::Restore};

    for (const TaskStatus status : statuses) {
        const Task task = taskWithStatus(status);
        for (const TaskTransition transition : transitions) {
            const auto expected = std::find_if(
                allowed.cbegin(), allowed.cend(),
                [status, transition](const TransitionCase &candidate) {
                    return candidate.source == status
                        && candidate.transition == transition;
                });
            const std::optional<TaskStatus> expectedTarget =
                expected == allowed.cend() ? std::nullopt : expected->target;

            QCOMPARE(TaskStateMachine::targetStatus(task, transition),
                     expectedTarget);
            QCOMPARE(TaskStateMachine::canApply(task, transition),
                     expectedTarget.has_value());
        }
    }
}

void TaskStateMachineTest::restoresTerminalArchivesAndNormalizesLegacyArchives()
{
    const QList<std::pair<std::optional<TaskStatus>, TaskStatus>> cases{
        {TaskStatus::Done, TaskStatus::Done},
        {TaskStatus::Cancelled, TaskStatus::Cancelled},
        {TaskStatus::Todo, TaskStatus::Todo},
        {TaskStatus::InProgress, TaskStatus::Todo},
        {TaskStatus::Archived, TaskStatus::Todo},
        {std::nullopt, TaskStatus::Todo},
    };

    for (const auto &[storedStatus, expected] : cases) {
        const Task archived = taskWithStatus(TaskStatus::Archived, storedStatus);
        QCOMPARE(TaskStateMachine::targetStatus(archived,
                                                TaskTransition::Restore),
                 std::optional<TaskStatus>{expected});
        QVERIFY(TaskStateMachine::canApply(archived, TaskTransition::Restore));
    }
}

QTEST_APPLESS_MAIN(TaskStateMachineTest)

#include "tst_TaskStateMachine.moc"
