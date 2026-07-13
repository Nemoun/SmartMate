#include "view/widgets/task/TaskPage.h"
#include "view/widgets/task/DeadlinePickerDialog.h"
#include "view/widgets/task/DurationPickerDialog.h"
#include "viewmodel/contracts/TaskDetailsContract.h"
#include "viewmodel/contracts/TaskEditorContract.h"
#include "viewmodel/contracts/TaskFocusContract.h"
#include "viewmodel/contracts/TaskListContract.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSignalBlocker>
#include <QTest>

using namespace smartmate;

namespace {

class FakeTaskList final : public viewmodel::TaskListContract {
public:
    FakeTaskList() : TaskListContract(nullptr) {}
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : 1; }
    int count() const noexcept override { return 1; }
    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || index.row() != 0) return {};
        switch (role) {
        case TaskIdRole: return QStringLiteral("11111111-1111-1111-1111-111111111111");
        case TitleRole: return QStringLiteral("契约任务");
        case DescriptionRole: return QStringLiteral("仅由 Fake Contract 提供");
        case StatusTextRole: return QStringLiteral("待办");
        case PriorityTextRole: return QStringLiteral("高");
        case OrderReasonTextRole: return QStringLiteral("高优先");
        case CanEditTaskRole: case CanStartRole: case CanCancelRole:
        case BulkSelectableRole: return true;
        default: return false;
        }
    }
    bool showArchived() const noexcept override { return archived; }
    QString searchText() const override { return search; }
    int priorityFilterIndex() const noexcept override { return priority; }
    QStringList priorityFilterOptions() const override { return {QStringLiteral("全部优先级"), QStringLiteral("低"), QStringLiteral("普通"), QStringLiteral("高"), QStringLiteral("紧急")}; }
    QVariantList categoryFilterOptions() const override { return {}; }
    int categoryFilterMode() const noexcept override { return 0; }
    QString categoryFilterCategoryId() const override { return {}; }
    bool hasActiveFilters() const override { return !search.isEmpty() || priority != 0; }
    bool bulkSelectionMode() const noexcept override { return bulk; }
    int bulkSelectedCount() const noexcept override { return selected ? 1 : 0; }
    int bulkSelectableVisibleCount() const override { return 1; }
    bool allVisibleSelected() const override { return selected; }
    bool canBulkArchive() const noexcept override { return bulk && selected && !archived; }
    bool canBulkRestore() const override { return bulk && selected && archived; }
    bool canBulkDelete() const noexcept override { return bulk && selected && archived; }
    void setShowArchived(bool value) override { ++showArchivedCalls; archived = value; emit showArchivedChanged(); }
    void setSearchText(const QString &value) override { ++searchCalls; search = value; emit searchTextChanged(); }
    void setPriorityFilterIndex(int value) override { ++priorityCalls; priority = value; emit priorityFilterIndexChanged(); }
    bool setCategoryFilter(int, const QString &) override { return true; }
    void reload() override {}
    void clearFilters() override { search.clear(); priority = 0; emit searchTextChanged(); emit priorityFilterIndexChanged(); }
    bool startTask(const QString &id) override { ++startCalls; lastId = id; return true; }
    bool cancelTask(const QString &id) override { ++cancelCalls; lastId = id; return true; }
    bool completeTask(const QString &id) override { ++completeCalls; lastId = id; return true; }
    bool redoTask(const QString &id) override { ++redoCalls; lastId = id; return true; }
    bool archiveTask(const QString &id) override { ++archiveCalls; lastId = id; return true; }
    bool restoreTask(const QString &id) override { ++restoreCalls; lastId = id; return true; }
    bool deleteArchivedTask(const QString &id) override { ++deleteCalls; lastId = id; return true; }
    void beginBulkSelection() override { bulk = true; emit bulkSelectionChanged(); }
    bool toggleBulkSelection(const QString &id) override { lastId = id; selected = !selected; emit bulkSelectionChanged(); return true; }
    void toggleSelectAllVisible() override { selected = !selected; emit bulkSelectionChanged(); }
    void clearBulkSelection() override { selected = false; emit bulkSelectionChanged(); }
    void cancelBulkSelection() override { selected = false; bulk = false; emit bulkSelectionChanged(); }
    bool archiveSelectedTasks() override { ++bulkArchiveCalls; return true; }
    bool restoreSelectedTasks() override { ++bulkRestoreCalls; return true; }
    bool deleteSelectedArchivedTasks() override { ++bulkDeleteCalls; return true; }

    void pushSearch(QString value) { search = std::move(value); emit searchTextChanged(); }
    bool archived{false}, bulk{false}, selected{false};
    QString search, lastId; int priority{0};
    int searchCalls{0}, priorityCalls{0}, showArchivedCalls{0}, startCalls{0};
    int cancelCalls{0}, completeCalls{0}, redoCalls{0}, archiveCalls{0}, restoreCalls{0}, deleteCalls{0};
    int bulkArchiveCalls{0}, bulkRestoreCalls{0}, bulkDeleteCalls{0};
};

class FakeFocus final : public viewmodel::TaskFocusContract {
public:
    FakeFocus() : TaskFocusContract(nullptr) {}
    FocusState focusState() const noexcept override { return state; }
    QString focusTaskId() const override { return id; }
    QString focusTitle() const override { return QStringLiteral("契约任务"); }
    QString focusDescription() const override { return QStringLiteral("描述"); }
    QString focusStatusText() const override { return QStringLiteral("待办"); }
    QString focusPriorityText() const override { return QStringLiteral("高"); }
    QString focusDeadlineText() const override { return {}; }
    int focusEstimatedMinutes() const noexcept override { return 30; }
    QString focusReasonText() const override { return QStringLiteral("高优先"); }
    bool focusOverdue() const noexcept override { return false; }
    bool focusCanStart() const noexcept override { return true; }
    bool focusCanComplete() const noexcept override { return false; }
    QString focusCategoryName() const override { return {}; }
    QString focusCategoryAccent() const override { return QStringLiteral("#94a3b8"); }
    bool focusHasCategory() const noexcept override { return false; }
    FocusState state{FocusState::Suggested};
    QString id{QStringLiteral("11111111-1111-1111-1111-111111111111")};
};

class FakeDetails final : public viewmodel::TaskDetailsContract {
public:
    FakeDetails() : TaskDetailsContract(nullptr) {}
    QString selectedTaskId() const override { return id; }
    QString selectedTitle() const override { return id.isEmpty() ? QString{} : QStringLiteral("契约任务"); }
    QString selectedDescription() const override { return QStringLiteral("描述"); }
    QString selectedStatusText() const override { return QStringLiteral("待办"); }
    QString selectedPriorityText() const override { return QStringLiteral("高"); }
    QString selectedDeadlineText() const override { return {}; }
    int selectedEstimatedMinutes() const noexcept override { return 30; }
    QString selectedCreatedAtText() const override { return QStringLiteral("2026-01-01 10:00"); }
    QString selectedUpdatedAtText() const override { return QStringLiteral("2026-01-01 10:00"); }
    QString selectedReasonText() const override { return QStringLiteral("高优先"); }
    QString selectedBlockingReasonText() const override { return {}; }
    int selectedPredecessorCount() const noexcept override { return 0; }
    int selectedUnlockCount() const noexcept override { return 0; }
    bool selectedCanEditTask() const noexcept override { return true; }
    bool selectedCanEditDependencies() const noexcept override { return false; }
    QString selectedCategoryName() const override { return {}; }
    QString selectedCategoryAccent() const override { return QStringLiteral("#94a3b8"); }
    bool selectedHasCategory() const noexcept override { return false; }
    bool selectTask(const QString &value) override { ++selectCalls; id = value; emit selectionChanged(); return !id.isEmpty(); }
    void clearSelection() override { id.clear(); emit selectionChanged(); }
    QString id; int selectCalls{0};
};

class FakeEditor final : public viewmodel::TaskEditorContract {
public:
    FakeEditor() : TaskEditorContract(nullptr) {}
    int rowCount(const QModelIndex & = {}) const override { return 0; }
    QVariant data(const QModelIndex &, int) const override { return {}; }
    QString taskId() const override { return {}; }
    bool editMode() const noexcept override { return edit; }
    bool sessionActive() const noexcept override { return active; }
    QString title() const override { return titleValue; }
    void setTitle(const QString &value) override { ++titleWrites; titleValue = value; emit titleChanged(); emit formStateChanged(); }
    QString description() const override { return descriptionValue; }
    void setDescription(const QString &value) override { ++descriptionWrites; descriptionValue = value; emit descriptionChanged(); }
    QString currentStatusText() const override { return QStringLiteral("待办"); }
    int priorityIndex() const noexcept override { return priority; }
    void setPriorityIndex(int value) override { priority = value; emit priorityIndexChanged(); }
    bool hasDeadline() const noexcept override { return false; }
    QString deadlineDisplayText() const override { return QStringLiteral("未设置"); }
    int deadlineYear() const override { return 2026; } int deadlineMonth() const override { return 1; }
    int deadlineDay() const override { return 1; } int deadlineHour() const override { return 10; }
    int deadlineMinute() const override { return 0; }
    bool hasEstimatedDuration() const noexcept override { return false; }
    QString estimatedDurationDisplayText() const override { return QStringLiteral("未设置"); }
    int estimatedDays() const noexcept override { return 0; } int estimatedHours() const noexcept override { return 0; }
    int estimatedMinutePart() const noexcept override { return 0; }
    int minimumEstimatedMinutes() const noexcept override { return 1; }
    int maximumEstimatedMinutes() const noexcept override { return 525600; }
    QStringList priorityOptions() const override { return {QStringLiteral("低"), QStringLiteral("普通"), QStringLiteral("高"), QStringLiteral("紧急")}; }
    QVariantList categoryOptions() const override { return {QVariantMap{{QStringLiteral("name"), QStringLiteral("未分类")}, {QStringLiteral("categoryId"), QString{}}}}; }
    QString selectedCategoryId() const override { return {}; }
    void setSelectedCategoryId(const QString &) override {}
    QString selectedCategoryName() const override { return QStringLiteral("未分类"); }
    QString selectedCategoryAccent() const override { return QStringLiteral("#94a3b8"); }
    bool hasCategory() const noexcept override { return false; }
    bool dirty() const noexcept override { return !titleValue.isEmpty(); }
    bool canSave() const noexcept override { return dirty(); }
    QString validationMessage() const override { return {}; }
    int predecessorCandidateCount() const noexcept override { return 0; }
    int selectedPredecessorCount() const noexcept override { return 0; }
    QString predecessorSummaryText() const override { return {}; }
    bool canConfigurePredecessors() const noexcept override { return !edit; }
    bool beginCreate() override { ++beginCreateCalls; edit = false; active = true; emit modeChanged(); emit sessionActiveChanged(); return true; }
    bool beginEdit(const QString &) override { ++beginEditCalls; edit = true; active = true; emit modeChanged(); emit sessionActiveChanged(); return true; }
    bool setDeadlineSelection(int, int, int, int, int) override { return true; }
    void clearDeadline() override {}
    bool setEstimatedDuration(int, int, int) override { return true; }
    void clearEstimatedDuration() override {}
    void beginPredecessorSelection() override {}
    bool setCreationPredecessorSelected(const QString &, bool) override { return true; }
    void acceptPredecessorSelection() override {} void cancelPredecessorSelection() override {}
    void clearCreationPredecessors() override {}
    bool save() override { if (!canSave()) return false; active = false; emit sessionActiveChanged(); emit saved(QStringLiteral("id")); return true; }
    void cancel() override { active = false; emit sessionActiveChanged(); emit cancelled(); }
    void pushTitle(QString value) { titleValue = std::move(value); emit titleChanged(); }
    bool edit{false}, active{false}; int priority{1};
    QString titleValue, descriptionValue; int beginCreateCalls{0}, beginEditCalls{0}, titleWrites{0}, descriptionWrites{0};
};

} // namespace

class TaskWidgetsTest final : public QObject {
    Q_OBJECT
private slots:
    void initialBindingAndUserCommandsUseContracts();
    void programmaticUpdatesDoNotWriteBack();
    void typedPickersPreserveValuesAndContractBounds();
};

void TaskWidgetsTest::initialBindingAndUserCommandsUseContracts()
{
    FakeTaskList tasks; FakeFocus focus; FakeDetails details; FakeEditor editor;
    view::widgets::TaskPage page{{tasks, focus, details, editor}};
    page.resize(900, 650); page.show();

    auto *search = page.findChild<QLineEdit *>(QStringLiteral("taskSearchField"));
    QVERIFY(search);
    QTest::keyClicks(search, QStringLiteral("MVVM"));
    QCOMPARE(tasks.search, QStringLiteral("MVVM"));
    QVERIFY(tasks.searchCalls > 0);

    auto *priority = page.findChild<QComboBox *>(QStringLiteral("priorityFilterComboBox"));
    QVERIFY(priority);
    priority->activated(3);
    QCOMPARE(tasks.priority, 3);

    auto *focusAction = page.findChild<QPushButton *>(QStringLiteral("focusPrimaryActionButton"));
    QTest::mouseClick(focusAction, Qt::LeftButton);
    QCOMPARE(tasks.startCalls, 1);
    QCOMPARE(tasks.lastId, focus.id);

    auto *newTask = page.findChild<QPushButton *>(QStringLiteral("newTaskButton"));
    QTest::mouseClick(newTask, Qt::LeftButton);
    QCOMPARE(editor.beginCreateCalls, 1);
    auto *title = page.findChild<QLineEdit *>(QStringLiteral("taskTitleField"));
    QTRY_VERIFY(title && title->isVisible());
    QTest::keyClicks(title, QStringLiteral("New task"));
    QCOMPARE(editor.titleValue, QStringLiteral("New task"));
    static_cast<QDialog *>(title->window())->reject();

    tasks.setShowArchived(true);
    auto *bulk = page.findChild<QPushButton *>(QStringLiteral("bulkManagementButton"));
    QTest::mouseClick(bulk, Qt::LeftButton);
    auto *list = page.findChild<QListView *>(QStringLiteral("taskListView"));
    list->setCurrentIndex(tasks.index(0));
    QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                      list->visualRect(tasks.index(0)).center());
    QVERIFY(tasks.selected);
    auto *restore = page.findChild<QPushButton *>(QStringLiteral("bulkRestoreButton"));
    QTest::mouseClick(restore, Qt::LeftButton);
    QCOMPARE(tasks.bulkRestoreCalls, 1);
}

void TaskWidgetsTest::programmaticUpdatesDoNotWriteBack()
{
    FakeTaskList tasks; FakeFocus focus; FakeDetails details; FakeEditor editor;
    view::widgets::TaskPage page{{tasks, focus, details, editor}};
    page.resize(900, 650); page.show();
    const int searchCalls = tasks.searchCalls;
    tasks.pushSearch(QStringLiteral("服务刷新"));
    QCOMPARE(page.findChild<QLineEdit *>(QStringLiteral("taskSearchField"))->text(),
             QStringLiteral("服务刷新"));
    QCOMPARE(tasks.searchCalls, searchCalls);

    editor.beginCreate();
    auto *title = page.findChild<QLineEdit *>(QStringLiteral("taskTitleField"));
    QTRY_VERIFY(title && title->isVisible());
    const int titleWrites = editor.titleWrites;
    editor.pushTitle(QStringLiteral("程序回填"));
    QCOMPARE(title->text(), QStringLiteral("程序回填"));
    QCOMPARE(editor.titleWrites, titleWrites);
}

void TaskWidgetsTest::typedPickersPreserveValuesAndContractBounds()
{
    view::widgets::DeadlinePickerDialog deadline;
    deadline.setSelection(2028, 2, 29, 23, 45);
    QCOMPARE(deadline.selectedYear(), 2028);
    QCOMPARE(deadline.selectedMonth(), 2);
    QCOMPARE(deadline.selectedDay(), 29);
    QCOMPARE(deadline.selectedHour(), 23);
    QCOMPARE(deadline.selectedMinute(), 45);

    view::widgets::DurationPickerDialog duration{1, 1500};
    duration.setDuration(0, 0, 0);
    QCOMPARE(duration.totalMinutes(), 1);
    duration.setDuration(2, 0, 0);
    QCOMPARE(duration.totalMinutes(), 1500);
    QCOMPARE(duration.selectedDays(), 1);
    QCOMPARE(duration.selectedHours(), 1);
    QCOMPARE(duration.selectedMinutes(), 0);

    auto *hours = duration.findChild<QSpinBox *>(QStringLiteral("durationHoursSpinBox"));
    auto *minutes = duration.findChild<QSpinBox *>(QStringLiteral("durationMinutesSpinBox"));
    QVERIFY(hours && minutes);
    QCOMPARE(hours->maximum(), 1);
    QCOMPARE(minutes->maximum(), 0);
}

QTEST_MAIN(TaskWidgetsTest)
#include "tst_TaskWidgets.moc"
