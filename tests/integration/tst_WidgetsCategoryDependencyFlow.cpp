#include "AppViewModel.h"
#include "persistence/SqliteTaskRepository.h"
#include "services/TaskCategoryService.h"
#include "services/TaskService.h"
#include "view/widgets/task/TaskCategoryDialog.h"
#include "view/widgets/task/TaskCreationPredecessorDialog.h"
#include "view/widgets/task/TaskDependencyDialog.h"
#include "view/widgets/task/TaskPage.h"

#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QTest>
#include <QTimer>

#include <algorithm>

using namespace smartmate;

namespace {

void acceptNextConfirmation()
{
    QTimer::singleShot(0, [] {
        auto *messageBox = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        Q_ASSERT(messageBox);
        QAbstractButton *button = messageBox->button(QMessageBox::Ok);
        Q_ASSERT(button);
        button->click();
    });
}

int comboIndexForId(QComboBox &combo, const QString &id, const int role)
{
    for (int index = 0; index < combo.count(); ++index) {
        if (combo.itemData(index, role).toString() == id) return index;
    }
    return -1;
}

model::Task taskNamed(const QList<model::Task> &tasks, const QString &title)
{
    const auto iterator = std::find_if(tasks.cbegin(), tasks.cend(),
        [&title](const model::Task &task) { return task.title() == title; });
    Q_ASSERT(iterator != tasks.cend());
    return *iterator;
}

int taskRowForId(viewmodel::TaskListContract &tasks, const QString &id)
{
    for (int row = 0; row < tasks.rowCount(); ++row) {
        if (tasks.data(tasks.index(row), viewmodel::TaskListContract::TaskIdRole)
                .toString() == id) return row;
    }
    return -1;
}

} // namespace

class WidgetsCategoryDependencyFlowTest final : public QObject {
    Q_OBJECT
private slots:
    void categoriesAndDependenciesTraverseWidgetsContractsAndAtomicRepositories();
};

void WidgetsCategoryDependencyFlowTest::categoriesAndDependenciesTraverseWidgetsContractsAndAtomicRepositories()
{
    model::persistence::SqliteTaskRepository repository{QStringLiteral(":memory:")};
    model::TaskService taskService{repository, repository, repository,
                                   repository, repository, repository};
    model::TaskCategoryService categoryService{repository};
    viewmodel::AppViewModel app{taskService, categoryService};
    view::widgets::TaskPage page{{*app.taskList(), *app.taskFocus(),
                                  *app.taskDetails(), *app.taskEditor(),
                                  *app.taskCategories(), *app.taskDependencies()}};
    page.resize(1100, 760);
    page.show();

    auto *manageCategories = page.findChild<QPushButton *>(QStringLiteral("manageCategoriesButton"));
    QTest::mouseClick(manageCategories, Qt::LeftButton);
    auto *categoryDialog = page.findChild<view::widgets::TaskCategoryDialog *>(
        QStringLiteral("taskCategoryDialog"));
    auto *categoryName = categoryDialog->findChild<QLineEdit *>(QStringLiteral("categoryNameField"));
    auto *saveCategory = categoryDialog->findChild<QPushButton *>(QStringLiteral("saveCategoryButton"));
    QTRY_VERIFY(categoryDialog->isVisible());
    QTest::keyClicks(categoryName, QStringLiteral("Study"));
    QTest::mouseClick(saveCategory, Qt::LeftButton);
    QTRY_COMPARE(app.taskCategories()->count(), 1);
    const QString categoryId = app.taskCategories()->editingCategoryId();
    QVERIFY(!categoryId.isEmpty());
    categoryDialog->close();

    auto *newTask = page.findChild<QPushButton *>(QStringLiteral("newTaskButton"));
    auto *title = page.findChild<QLineEdit *>(QStringLiteral("taskTitleField"));
    auto *taskCategory = page.findChild<QComboBox *>(QStringLiteral("taskCategoryComboBox"));
    auto *saveTask = page.findChild<QPushButton *>(QStringLiteral("saveTaskButton"));
    QTest::mouseClick(newTask, Qt::LeftButton);
    QTRY_VERIFY(title->isVisible());
    QTest::keyClicks(title, QStringLiteral("First"));
    int categoryIndex = taskCategory->findData(categoryId);
    QVERIFY(categoryIndex >= 0);
    taskCategory->activated(categoryIndex);
    QTest::mouseClick(saveTask, Qt::LeftButton);
    QTRY_COMPARE(repository.findAll().size(), 1);
    const model::Task first = taskNamed(repository.findAll(), QStringLiteral("First"));
    QCOMPARE(first.categoryId()->toString(QUuid::WithoutBraces), categoryId);

    QTest::mouseClick(newTask, Qt::LeftButton);
    QTRY_VERIFY(title->isVisible());
    QTest::keyClicks(title, QStringLiteral("Second"));
    categoryIndex = taskCategory->findData(categoryId);
    taskCategory->activated(categoryIndex);
    auto *openPredecessors = page.findChild<QPushButton *>(QStringLiteral("openCreationPredecessorButton"));
    QTest::mouseClick(openPredecessors, Qt::LeftButton);
    auto *creationDialog = page.findChild<view::widgets::TaskCreationPredecessorDialog *>();
    QTRY_VERIFY(creationDialog->isVisible());
    auto *creationList = creationDialog->findChild<QListView *>(QStringLiteral("creationPredecessorCandidateList"));
    QCOMPARE(app.taskEditor()->predecessorCandidateCount(), 1);
    QTest::mouseClick(creationList->viewport(), Qt::LeftButton, Qt::NoModifier,
                      creationList->visualRect(app.taskEditor()->index(0)).center());
    auto *acceptPredecessors = creationDialog->findChild<QPushButton *>(QStringLiteral("acceptCreationPredecessorsButton"));
    QTest::mouseClick(acceptPredecessors, Qt::LeftButton);
    QCOMPARE(app.taskEditor()->selectedPredecessorCount(), 1);
    QTest::mouseClick(saveTask, Qt::LeftButton);

    QTRY_COMPARE(repository.findAll().size(), 2);
    const model::Task second = taskNamed(repository.findAll(), QStringLiteral("Second"));
    QCOMPARE(repository.findAllDependencies(),
             QList<model::TaskDependency>({{first.id(), second.id()}}));
    const QString secondId = second.id().toString(QUuid::WithoutBraces);
    QTRY_VERIFY(taskRowForId(*app.taskList(), secondId) >= 0);
    QVERIFY(app.taskList()->data(app.taskList()->index(
        taskRowForId(*app.taskList(), secondId)),
        viewmodel::TaskListContract::BlockedRole).toBool());

    auto *categoryFilter = page.findChild<QComboBox *>(QStringLiteral("categoryFilterComboBox"));
    const int filterIndex = comboIndexForId(*categoryFilter, categoryId, Qt::UserRole + 1);
    QVERIFY(filterIndex >= 0);
    categoryFilter->activated(filterIndex);
    QTRY_COMPARE(app.taskList()->count(), 2);
    QCOMPARE(app.taskList()->categoryFilterCategoryId(), categoryId);

    auto *dependencyDialog = page.findChild<view::widgets::TaskDependencyDialog *>();
    QVERIFY(dependencyDialog->openTask(first.id().toString(QUuid::WithoutBraces)));
    QTRY_VERIFY(dependencyDialog->isVisible());
    auto *dependencyList = dependencyDialog->findChild<QListView *>(QStringLiteral("dependencyCandidateList"));
    QTest::mouseClick(dependencyList->viewport(), Qt::LeftButton, Qt::NoModifier,
                      dependencyList->visualRect(app.taskDependencies()->index(0)).center());
    auto *saveDependencies = dependencyDialog->findChild<QPushButton *>(QStringLiteral("saveDependenciesButton"));
    QTest::mouseClick(saveDependencies, Qt::LeftButton);
    QTRY_VERIFY(dependencyDialog->isVisible());
    QCOMPARE(repository.findAllDependencies(),
             QList<model::TaskDependency>({{first.id(), second.id()}}));
    auto *dependencyNotification = dependencyDialog->findChild<QLabel *>(QStringLiteral("dependencyNotificationLabel"));
    QVERIFY(dependencyNotification->text().contains(QStringLiteral("循环依赖")));
    dependencyDialog->reject();

    QVERIFY(dependencyDialog->openTask(second.id().toString(QUuid::WithoutBraces)));
    QTRY_VERIFY(dependencyDialog->isVisible());
    QTest::mouseClick(dependencyList->viewport(), Qt::LeftButton, Qt::NoModifier,
                      dependencyList->visualRect(app.taskDependencies()->index(0)).center());
    QTest::mouseClick(saveDependencies, Qt::LeftButton);
    QTRY_VERIFY(!dependencyDialog->isVisible());
    QVERIFY(repository.findAllDependencies().isEmpty());

    QTest::mouseClick(manageCategories, Qt::LeftButton);
    QTRY_VERIFY(categoryDialog->isVisible());
    auto *categoryList = categoryDialog->findChild<QListView *>(QStringLiteral("categoryListView"));
    categoryList->setCurrentIndex(app.taskCategories()->index(0));
    auto *deleteCategory = categoryDialog->findChild<QPushButton *>(QStringLiteral("deleteSelectedCategoryButton"));
    acceptNextConfirmation();
    QTest::mouseClick(deleteCategory, Qt::LeftButton);
    QTRY_COMPARE(app.taskCategories()->count(), 0);
    QTRY_COMPARE(app.taskList()->categoryFilterMode(), 1);
    for (const model::Task &task : repository.findAll()) QVERIFY(!task.categoryId().has_value());
    QVERIFY(repository.findAllDependencies().isEmpty());
}

QTEST_MAIN(WidgetsCategoryDependencyFlowTest)
#include "tst_WidgetsCategoryDependencyFlow.moc"
