#pragma once

#include <QDialog>

class QLabel;
class QListView;
class QPushButton;

namespace smartmate::viewmodel { class TaskDependencyContract; }

namespace smartmate::view::widgets {

/// 已有任务的完整前置集合编辑器；保存时只调用一次原子 Contract 命令。
class TaskDependencyDialog final : public QDialog {
    Q_OBJECT
public:
    explicit TaskDependencyDialog(viewmodel::TaskDependencyContract &dependencies,
                                  QWidget *parent = nullptr);

    /// 仅在 Contract 成功建立稳定 TaskId 草稿后显示窗口。
    bool openTask(const QString &taskId);
    void reject() override;

private:
    void synchronize();

    viewmodel::TaskDependencyContract &m_dependencies;
    QListView *m_list;
    QLabel *m_description;
    QLabel *m_count;
    QLabel *m_empty;
    QLabel *m_notification;
    QPushButton *m_save;
    bool m_draftActive{false};
};

} // namespace smartmate::view::widgets
