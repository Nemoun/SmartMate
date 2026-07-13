#include "AppViewModel.h"
#include "persistence/SqliteTaskRepository.h"
#include "services/TaskService.h"
#include "view/widgets/task/TaskItemDelegate.h"
#include "view/widgets/task/TaskPage.h"

#include <QAbstractButton>
#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QToolButton>

using namespace smartmate;

namespace {

void answerNextConfirmation(const QMessageBox::StandardButton answer)
{
    QTimer::singleShot(0, [answer] {
        auto *messageBox = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        Q_ASSERT(messageBox);
        QAbstractButton *button = messageBox->button(answer);
        Q_ASSERT(button);
        button->click();
    });
}

void clickPrimaryAction(QListView &list, const QModelIndex &index)
{
    const QRect rect = list.visualRect(index);
    QTest::mouseClick(list.viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint{rect.right() - 98, rect.center().y()});
}

} // namespace

class WidgetsTaskFlowTest final : public QObject {
    Q_OBJECT
private slots:
    void taskMainFlowPersistsAndRefreshesEveryProjection();
};

void WidgetsTaskFlowTest::taskMainFlowPersistsAndRefreshesEveryProjection()
{
    model::persistence::SqliteTaskRepository repository{QStringLiteral(":memory:")};
    model::TaskService service{repository, repository, repository,
                               repository, repository, repository};
    viewmodel::AppViewModel app{service};
    view::widgets::TaskPage page{{*app.taskList(), *app.taskFocus(),
                                  *app.taskDetails(), *app.taskEditor()}};
    page.resize(980, 700);
    page.show();

    auto *newTask = page.findChild<QPushButton *>(QStringLiteral("newTaskButton"));
    QVERIFY(newTask);
    QTest::mouseClick(newTask, Qt::LeftButton);
    auto *title = page.findChild<QLineEdit *>(QStringLiteral("taskTitleField"));
    auto *save = page.findChild<QPushButton *>(QStringLiteral("saveTaskButton"));
    QTRY_VERIFY(title && title->isVisible());
    QTest::keyClicks(title, QStringLiteral("Widgets created task"));
    QTRY_VERIFY(save->isEnabled());
    QTest::mouseClick(save, Qt::LeftButton);

    QTRY_COMPARE(app.taskList()->count(), 1);
    QCOMPARE(repository.findAll().size(), 1);
    const QString taskId = app.taskList()->data(app.taskList()->index(0),
        viewmodel::TaskListContract::TaskIdRole).toString();
    QCOMPARE(app.taskFocus()->focusTaskId(), taskId);
    QCOMPARE(app.taskFocus()->focusState(),
             viewmodel::TaskFocusContract::FocusState::Suggested);

    auto *list = page.findChild<QListView *>(QStringLiteral("taskListView"));
    QVERIFY(list);
    list->activated(app.taskList()->index(0));
    auto *details = page.findChild<QDialog *>(QStringLiteral("taskDetailsDialog"));
    QTRY_VERIFY(details && details->isVisible());
    QCOMPARE(app.taskDetails()->selectedTaskId(), taskId);
    auto *edit = details->findChild<QPushButton *>(QStringLiteral("editSelectedTaskButton"));
    QTest::mouseClick(edit, Qt::LeftButton);
    QTRY_VERIFY(title->isVisible());
    title->selectAll();
    QTest::keyClicks(title, QStringLiteral("Widgets edited task"));
    QTRY_VERIFY(save->isEnabled());
    QTest::mouseClick(save, Qt::LeftButton);
    QTRY_COMPARE(repository.findAll().constFirst().title(),
                 QStringLiteral("Widgets edited task"));

    auto *search = page.findChild<QLineEdit *>(QStringLiteral("taskSearchField"));
    QTest::keyClicks(search, QStringLiteral("not-found"));
    QTRY_COMPARE(app.taskList()->count(), 0);
    search->selectAll();
    QTest::keyClick(search, Qt::Key_Backspace);
    QTRY_COMPARE(app.taskList()->count(), 1);
    QCOMPARE(app.taskFocus()->focusTaskId(), taskId);

    auto *delegate = qobject_cast<view::widgets::TaskItemDelegate *>(list->itemDelegate());
    QVERIFY(delegate);
    answerNextConfirmation(QMessageBox::Cancel);
    delegate->cancelRequested(taskId, QStringLiteral("Widgets edited task"));
    QCOMPARE(repository.findAll().constFirst().status(), model::TaskStatus::Todo);
    answerNextConfirmation(QMessageBox::Ok);
    delegate->cancelRequested(taskId, QStringLiteral("Widgets edited task"));
    QTRY_COMPARE(repository.findAll().constFirst().status(), model::TaskStatus::Cancelled);

    clickPrimaryAction(*list, app.taskList()->index(0));
    QTRY_COMPARE(repository.findAll().constFirst().status(), model::TaskStatus::Todo);

    auto *focusAction = page.findChild<QPushButton *>(QStringLiteral("focusPrimaryActionButton"));
    QTest::mouseClick(focusAction, Qt::LeftButton);
    QTRY_COMPARE(app.taskFocus()->focusState(),
                 viewmodel::TaskFocusContract::FocusState::InProgress);
    QCOMPARE(repository.findAll().constFirst().status(), model::TaskStatus::InProgress);
    QTest::mouseClick(focusAction, Qt::LeftButton);
    QTRY_COMPARE(repository.findAll().constFirst().status(), model::TaskStatus::Done);
    QVERIFY(app.taskFocus()->focusTaskId().isEmpty());

    answerNextConfirmation(QMessageBox::Ok);
    delegate->archiveRequested(taskId, QStringLiteral("Widgets edited task"));
    QTRY_COMPARE(repository.findAll().constFirst().status(), model::TaskStatus::Archived);

    auto *archived = page.findChild<QToolButton *>(QStringLiteral("archivedTasksButton"));
    auto *active = page.findChild<QToolButton *>(QStringLiteral("activeTasksButton"));
    QVERIFY(archived && active);
    QTest::mouseClick(archived, Qt::LeftButton);
    QTRY_COMPARE(app.taskList()->count(), 1);
    clickPrimaryAction(*list, app.taskList()->index(0));
    QTRY_COMPARE(repository.findAll().constFirst().status(), model::TaskStatus::Done);

    QTest::mouseClick(active, Qt::LeftButton);
    QTRY_COMPARE(app.taskList()->count(), 1);
    answerNextConfirmation(QMessageBox::Ok);
    delegate->archiveRequested(taskId, QStringLiteral("Widgets edited task"));
    QTRY_COMPARE(repository.findAll().constFirst().status(), model::TaskStatus::Archived);
    QTest::mouseClick(archived, Qt::LeftButton);
    QTRY_COMPARE(app.taskList()->count(), 1);
    answerNextConfirmation(QMessageBox::Ok);
    delegate->deleteRequested(taskId, QStringLiteral("Widgets edited task"));
    QTRY_VERIFY(repository.findAll().isEmpty());
    QTRY_COMPARE(app.taskList()->count(), 0);
    QVERIFY(app.taskDetails()->selectedTaskId().isEmpty());
}

QTEST_MAIN(WidgetsTaskFlowTest)
#include "tst_WidgetsTaskFlow.moc"
