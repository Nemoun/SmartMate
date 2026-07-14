#pragma once

#include <QDialog>

class QComboBox;
class QFrame;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QShowEvent;
class QWidget;

namespace smartmate::viewmodel { class TaskEditorContract; }

namespace smartmate::view::widgets {

class TaskCreationPredecessorDialog;

/// 使用类型化 Qt Widgets 控件编辑 TaskEditorContract 草稿。
class TaskEditorDialog final : public QDialog {
    Q_OBJECT
public:
    explicit TaskEditorDialog(viewmodel::TaskEditorContract &editor,
                              QWidget *parent = nullptr);

signals:
    void manageCategoriesRequested();

protected:
    void reject() override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void synchronize();
    void synchronizeSession();
    void updateResponsiveLayout();
    void chooseDeadline();
    void chooseDuration();

    viewmodel::TaskEditorContract &m_editor;
    QLabel *m_headerTitle;
    QLabel *m_headerSubtitle;
    QScrollArea *m_scroll;
    QGridLayout *m_planningGrid;
    QWidget *m_statusField;
    QWidget *m_priorityField;
    QWidget *m_categoryField;
    QLineEdit *m_title;
    QPlainTextEdit *m_description;
    QLabel *m_status;
    QComboBox *m_priority;
    QComboBox *m_category;
    QWidget *m_predecessorField;
    QLabel *m_predecessors;
    QPushButton *m_choosePredecessors;
    QPushButton *m_clearPredecessors;
    QLabel *m_deadline;
    QPushButton *m_deadlineClear;
    QLabel *m_duration;
    QPushButton *m_durationClear;
    QLabel *m_validation;
    QPushButton *m_save;
    TaskCreationPredecessorDialog *m_predecessorDialog;
};

} // namespace smartmate::view::widgets
