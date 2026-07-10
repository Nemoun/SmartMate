#include "fakes/FakeTaskRepository.h"

#include "domain/Task.h"
#include "services/TaskService.h"

#include <QTest>

using smartmate::model::Task;
using smartmate::model::TaskService;
using smartmate::model::TaskStatus;
using smartmate::tests::FakeTaskRepository;

class TaskServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void returnsEveryTaskFromTheRepository();
};

void TaskServiceTest::returnsEveryTaskFromTheRepository()
{
    const QList<Task> expected{
        Task{QUuid{QStringLiteral("{11111111-1111-1111-1111-111111111111}")},
             QStringLiteral("Understand MVVM"), TaskStatus::InProgress},
        Task{QUuid{QStringLiteral("{22222222-2222-2222-2222-222222222222}")},
             QStringLiteral("Build SmartMate"), TaskStatus::Todo},
    };
    FakeTaskRepository repository{expected};
    const TaskService service{repository};

    const QList<Task> actual = service.listTasks();

    QCOMPARE(actual.size(), expected.size());
    QCOMPARE(actual.at(0).id(), expected.at(0).id());
    QCOMPARE(actual.at(0).title(), expected.at(0).title());
    QCOMPARE(actual.at(0).status(), expected.at(0).status());
    QCOMPARE(actual.at(1).id(), expected.at(1).id());
}

QTEST_APPLESS_MAIN(TaskServiceTest)

#include "tst_TaskService.moc"
