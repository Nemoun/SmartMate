#pragma once

#include <QDialog>
#include <QFrame>
#include <QListView>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

namespace smartmate::viewmodel {
class TaskListContract;
class TaskFocusContract;
class TaskDetailsContract;
class TaskEditorContract;
}

namespace smartmate::view::widgets {

struct TaskPageDependencies {
    viewmodel::TaskListContract &taskList;
    viewmodel::TaskFocusContract &taskFocus;
    viewmodel::TaskDetailsContract &taskDetails;
    viewmodel::TaskEditorContract &taskEditor;
};

class TaskListView final : public QListView {
    Q_OBJECT
public:
    explicit TaskListView(QWidget *parent = nullptr);
protected:
    void startDrag(Qt::DropActions supportedActions) override;
};

class TaskFocusPanel final : public QFrame {
    Q_OBJECT
public:
    TaskFocusPanel(viewmodel::TaskFocusContract &focus,
                   viewmodel::TaskListContract &tasks,
                   QWidget *parent = nullptr);
signals:
    void detailsRequested(const QString &taskId);
    void createRequested();
    void dependencyGraphRequested();
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    void synchronize();
    viewmodel::TaskFocusContract &m_focus;
    viewmodel::TaskListContract &m_tasks;
    QLabel *m_title;
    QLabel *m_description;
    QLabel *m_meta;
    QPushButton *m_details;
    QPushButton *m_primary;
};

class TaskDetailsDialog final : public QDialog {
    Q_OBJECT
public:
    explicit TaskDetailsDialog(viewmodel::TaskDetailsContract &details,
                               QWidget *parent = nullptr);
    bool openTask(const QString &taskId);
signals:
    void editRequested(const QString &taskId);
protected:
    void done(int result) override;
private:
    void synchronize();
    viewmodel::TaskDetailsContract &m_details;
    QLabel *m_title;
    QLabel *m_summary;
    QLabel *m_description;
    QLabel *m_schedule;
    QLabel *m_insight;
    QPushButton *m_edit;
};

/// 任务主流程页面，只组合抽象 Contract 并转发稳定 TaskId。
class TaskPage final : public QWidget {
    Q_OBJECT
public:
    explicit TaskPage(TaskPageDependencies dependencies,
                      QWidget *parent = nullptr);
signals:
    void showDependencyGraphRequested();
private:
    bool confirm(const QString &title, const QString &message);
    void openEditor(const QString &taskId);
    void updateControls();

    TaskPageDependencies m_dependencies;
    TaskFocusPanel *m_focus;
    QLineEdit *m_search;
    QComboBox *m_priority;
    QToolButton *m_active;
    QToolButton *m_archived;
    QPushButton *m_bulk;
    QPushButton *m_newTask;
    QWidget *m_bulkBar;
    QLabel *m_bulkCount;
    QPushButton *m_selectAll;
    QPushButton *m_bulkArchive;
    QPushButton *m_bulkRestore;
    QPushButton *m_bulkDelete;
    TaskListView *m_list;
    QLabel *m_empty;
    TaskDetailsDialog *m_details;
};

} // namespace smartmate::view::widgets
