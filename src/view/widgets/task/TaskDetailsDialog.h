#pragma once

#include <QDialog>

class QEvent;
class QFrame;
class QGridLayout;
class QLabel;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QWidget;

namespace smartmate::viewmodel { class TaskDetailsContract; }

namespace smartmate::view::widgets {

/// 复用 TaskDetailsContract 的纯展示对话框；动作可按调用场景整体隐藏。
class TaskDetailsDialog final : public QDialog {
    Q_OBJECT
public:
    explicit TaskDetailsDialog(viewmodel::TaskDetailsContract &details,
                               QWidget *parent = nullptr);

    /// 先让 Contract 按稳定 TaskId 建立详情投影，成功后才显示对话框。
    bool openTask(const QString &taskId);
    void setActionsVisible(bool visible);

signals:
    /// 只向页面转发稳定 TaskId；对话框不直接创建编辑器或依赖编辑器。
    void editRequested(const QString &taskId);
    void editDependenciesRequested(const QString &taskId);

protected:
    void done(int result) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void synchronize();
    void updateResponsiveLayout();
    void applyPresentationStyle();

    /// 非拥有详情 Contract；所有标签内容均在 selectionChanged 后重读。
    viewmodel::TaskDetailsContract &m_details;
    QLabel *m_title;
    QLabel *m_statusBadge;
    QLabel *m_priorityBadge;
    QLabel *m_categoryBadge;
    QLabel *m_overdueBadge;
    QScrollArea *m_scroll;
    QLabel *m_description;
    QGridLayout *m_scheduleGrid;
    QWidget *m_deadlineField;
    QWidget *m_estimateField;
    QWidget *m_createdField;
    QWidget *m_updatedField;
    QLabel *m_deadline;
    QLabel *m_estimate;
    QLabel *m_created;
    QLabel *m_updated;
    QLabel *m_predecessorCount;
    QLabel *m_unlockCount;
    QFrame *m_recommendationBlock;
    QLabel *m_recommendation;
    QFrame *m_blockingBlock;
    QLabel *m_blocking;
    QPushButton *m_edit;
    QPushButton *m_editDependencies;
    bool m_actionsVisible{true};
    bool m_applyingStyle{false};
};

} // namespace smartmate::view::widgets
