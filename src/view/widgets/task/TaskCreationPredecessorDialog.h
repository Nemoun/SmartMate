#pragma once

#include <QDialog>

class QLabel;
class QListView;
class QPushButton;

namespace smartmate::viewmodel { class TaskEditorContract; }

namespace smartmate::view::widgets {

/// 新建任务的前置集合选择器；确认前只修改 TaskEditorContract 的局部候选草稿。
class TaskCreationPredecessorDialog final : public QDialog {
    Q_OBJECT
public:
    explicit TaskCreationPredecessorDialog(
        viewmodel::TaskEditorContract &editor, QWidget *parent = nullptr);

    void openSelection();
    void reject() override;

private:
    void synchronize();

    viewmodel::TaskEditorContract &m_editor;
    QListView *m_list;
    QLabel *m_count;
    QLabel *m_empty;
    QPushButton *m_clear;
    bool m_selectionActive{false};
};

} // namespace smartmate::view::widgets
