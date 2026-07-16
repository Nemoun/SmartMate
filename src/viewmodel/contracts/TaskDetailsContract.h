#pragma once

#include "common/presentation/UiNotification.h"
#include "viewmodel/contracts/TaskPresentationTypes.h"

#include <QObject>
#include <QString>

namespace smartmate::viewmodel {

/// 稳定 TaskId 驱动的只读详情会话契约。
///
/// 详情属性是具体 ViewModel 对 Model 计划的只读投影；Widget 通过选择命令提交稳定 ID，
/// 收到 selectionChanged() 后统一重读 getter，不得持有或修改领域 Task。
class TaskDetailsContract : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString selectedTaskId READ selectedTaskId NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDescription READ selectedDescription NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedStatusText READ selectedStatusText NOTIFY selectionChanged)
    Q_PROPERTY(TaskStatusVisual selectedStatusVisual READ selectedStatusVisual NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedPriorityText READ selectedPriorityText NOTIFY selectionChanged)
    Q_PROPERTY(TaskPriorityVisual selectedPriorityVisual READ selectedPriorityVisual NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDeadlineText READ selectedDeadlineText NOTIFY selectionChanged)
    Q_PROPERTY(int selectedEstimatedMinutes READ selectedEstimatedMinutes NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCreatedAtText READ selectedCreatedAtText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedUpdatedAtText READ selectedUpdatedAtText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedReasonText READ selectedReasonText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedBlockingReasonText READ selectedBlockingReasonText NOTIFY selectionChanged)
    Q_PROPERTY(int selectedPredecessorCount READ selectedPredecessorCount NOTIFY selectionChanged)
    Q_PROPERTY(int selectedUnlockCount READ selectedUnlockCount NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedCanEditTask READ selectedCanEditTask NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedCanEditDependencies READ selectedCanEditDependencies NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCategoryName READ selectedCategoryName NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCategoryAccent READ selectedCategoryAccent NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedHasCategory READ selectedHasCategory NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedOverdue READ selectedOverdue NOTIFY selectionChanged)

public:
    ~TaskDetailsContract() override = default;

    // getter 共同描述当前选择；空选择时具体实现返回安全空值和 false/0。
    [[nodiscard]] virtual QString selectedTaskId() const = 0;
    [[nodiscard]] virtual QString selectedTitle() const = 0;
    [[nodiscard]] virtual QString selectedDescription() const = 0;
    [[nodiscard]] virtual QString selectedStatusText() const = 0;
    [[nodiscard]] virtual TaskStatusVisual selectedStatusVisual() const noexcept = 0;
    [[nodiscard]] virtual QString selectedPriorityText() const = 0;
    [[nodiscard]] virtual TaskPriorityVisual selectedPriorityVisual() const noexcept = 0;
    [[nodiscard]] virtual QString selectedDeadlineText() const = 0;
    [[nodiscard]] virtual int selectedEstimatedMinutes() const noexcept = 0;
    [[nodiscard]] virtual QString selectedCreatedAtText() const = 0;
    [[nodiscard]] virtual QString selectedUpdatedAtText() const = 0;
    [[nodiscard]] virtual QString selectedReasonText() const = 0;
    [[nodiscard]] virtual QString selectedBlockingReasonText() const = 0;
    [[nodiscard]] virtual int selectedPredecessorCount() const noexcept = 0;
    [[nodiscard]] virtual int selectedUnlockCount() const noexcept = 0;
    [[nodiscard]] virtual bool selectedCanEditTask() const noexcept = 0;
    [[nodiscard]] virtual bool selectedCanEditDependencies() const noexcept = 0;
    [[nodiscard]] virtual QString selectedCategoryName() const = 0;
    [[nodiscard]] virtual QString selectedCategoryAccent() const = 0;
    [[nodiscard]] virtual bool selectedHasCategory() const noexcept = 0;
    [[nodiscard]] virtual bool selectedOverdue() const noexcept = 0;

public slots:
    /// 选择稳定 TaskId；格式无效或当前投影不存在时返回 false。
    virtual bool selectTask(const QString &taskId) = 0;
    /// 清除详情会话选择，不改变 Model 中的任务。
    virtual void clearSelection() = 0;

signals:
    /// 选择或所选任务的任一详情投影变化后发送；绑定方应重新读取全部 getter。
    void selectionChanged();
    /// 一次性展示通知；只描述 UI 消息，不允许 ViewModel 直接弹窗。
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    /// 仅允许具体详情 ViewModel 派生构造。
    explicit TaskDetailsContract(QObject *parent = nullptr) : QObject(parent) {}
};

} // namespace smartmate::viewmodel
