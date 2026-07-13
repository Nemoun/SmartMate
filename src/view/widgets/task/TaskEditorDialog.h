#pragma once

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

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

private:
    void synchronize();
    void synchronizeSession();
    void chooseDeadline();
    void chooseDuration();

    viewmodel::TaskEditorContract &m_editor;
    QLineEdit *m_title;
    QPlainTextEdit *m_description;
    QLabel *m_status;
    QComboBox *m_priority;
    QComboBox *m_category;
    QLabel *m_predecessors;
    QPushButton *m_choosePredecessors;
    QPushButton *m_clearPredecessors;
    QLabel *m_deadline;
    QLabel *m_duration;
    QLabel *m_validation;
    QPushButton *m_save;
    TaskCreationPredecessorDialog *m_predecessorDialog;
};

} // namespace smartmate::view::widgets
