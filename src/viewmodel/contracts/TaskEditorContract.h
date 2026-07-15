#pragma once

#include "common/presentation/UiNotification.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>

namespace smartmate::viewmodel {

/// 任务创建和编辑草稿的抽象展示契约。
///
/// 可写属性只修改 ViewModel 草稿，save() 才形成一次 Model 命令。截止时间和预计用时
/// 必须通过类型化槽提交；创建前置选择属于临时子会话，接受后才并入创建草稿。
class TaskEditorContract : public QAbstractListModel {
    Q_OBJECT
    // 属性按变化范围拆分 NOTIFY，Widget 可只刷新相关控件；CONSTANT 选项在生命周期内不变。
    Q_PROPERTY(QString taskId READ taskId NOTIFY modeChanged)
    Q_PROPERTY(bool editMode READ editMode NOTIFY modeChanged)
    Q_PROPERTY(bool sessionActive READ sessionActive NOTIFY sessionActiveChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QString currentStatusText READ currentStatusText NOTIFY currentStatusTextChanged)
    Q_PROPERTY(int priorityIndex READ priorityIndex WRITE setPriorityIndex NOTIFY priorityIndexChanged)
    Q_PROPERTY(bool hasDeadline READ hasDeadline NOTIFY deadlineChanged)
    Q_PROPERTY(QString deadlineDisplayText READ deadlineDisplayText NOTIFY deadlineChanged)
    Q_PROPERTY(int deadlineYear READ deadlineYear NOTIFY deadlineChanged)
    Q_PROPERTY(int deadlineMonth READ deadlineMonth NOTIFY deadlineChanged)
    Q_PROPERTY(int deadlineDay READ deadlineDay NOTIFY deadlineChanged)
    Q_PROPERTY(int deadlineHour READ deadlineHour NOTIFY deadlineChanged)
    Q_PROPERTY(int deadlineMinute READ deadlineMinute NOTIFY deadlineChanged)
    Q_PROPERTY(bool hasEstimatedDuration READ hasEstimatedDuration NOTIFY estimatedDurationChanged)
    Q_PROPERTY(QString estimatedDurationDisplayText READ estimatedDurationDisplayText
                   NOTIFY estimatedDurationChanged)
    Q_PROPERTY(int estimatedDays READ estimatedDays NOTIFY estimatedDurationChanged)
    Q_PROPERTY(int estimatedHours READ estimatedHours NOTIFY estimatedDurationChanged)
    Q_PROPERTY(int estimatedMinutePart READ estimatedMinutePart NOTIFY estimatedDurationChanged)
    Q_PROPERTY(int minimumEstimatedMinutes READ minimumEstimatedMinutes CONSTANT)
    Q_PROPERTY(int maximumEstimatedMinutes READ maximumEstimatedMinutes CONSTANT)
    Q_PROPERTY(QStringList priorityOptions READ priorityOptions CONSTANT)
    Q_PROPERTY(QVariantList categoryOptions READ categoryOptions NOTIFY categoryOptionsChanged)
    Q_PROPERTY(QString selectedCategoryId READ selectedCategoryId WRITE setSelectedCategoryId
                   NOTIFY categoryChanged)
    Q_PROPERTY(QString selectedCategoryName READ selectedCategoryName NOTIFY categoryChanged)
    Q_PROPERTY(QString selectedCategoryAccent READ selectedCategoryAccent NOTIFY categoryChanged)
    Q_PROPERTY(bool hasCategory READ hasCategory NOTIFY categoryChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY formStateChanged)
    Q_PROPERTY(bool canSave READ canSave NOTIFY formStateChanged)
    Q_PROPERTY(QString validationMessage READ validationMessage NOTIFY formStateChanged)
    Q_PROPERTY(int predecessorCandidateCount READ predecessorCandidateCount
                   NOTIFY predecessorCandidatesChanged)
    Q_PROPERTY(int selectedPredecessorCount READ selectedPredecessorCount
                   NOTIFY predecessorSelectionChanged)
    Q_PROPERTY(QString predecessorSummaryText READ predecessorSummaryText
                   NOTIFY predecessorSelectionChanged)
    Q_PROPERTY(bool canConfigurePredecessors READ canConfigurePredecessors NOTIFY modeChanged)

public:
    /// 创建前置候选列表的稳定 Role；候选身份始终使用 TaskId，不使用行号或标题。
    enum Role {
        CandidateTaskIdRole = Qt::UserRole + 1,
        CandidateShortIdRole,
        CandidateTitleRole,
        CandidateStatusTextRole,
        CandidatePriorityTextRole,
        // 类别字段是候选任务的展示投影，不参与依赖资格计算。
        CandidateCategoryNameRole,
        CandidateCategoryAccentRole,
        CandidateHasCategoryRole,
        // 选中状态属于创建草稿会话，不代表依赖边已经持久化。
        CandidateSelectedRole,
    };
    Q_ENUM(Role)

    ~TaskEditorContract() override = default;

    // getter 暴露当前会话快照和派生资格；绑定建立时应先读取，再监听对应通知。
    [[nodiscard]] virtual QString taskId() const = 0;
    [[nodiscard]] virtual bool editMode() const noexcept = 0;
    [[nodiscard]] virtual bool sessionActive() const noexcept = 0;
    [[nodiscard]] virtual QString title() const = 0;
    /// 双向标题草稿入口；只更新会话状态，不直接写 Model。
    virtual void setTitle(const QString &title) = 0;
    [[nodiscard]] virtual QString description() const = 0;
    /// 双向描述草稿入口；程序性控件回填应阻断控件信号，避免回写循环。
    virtual void setDescription(const QString &description) = 0;
    [[nodiscard]] virtual QString currentStatusText() const = 0;
    [[nodiscard]] virtual int priorityIndex() const noexcept = 0;
    /// 以稳定选项索引更新优先级草稿，具体实现负责范围检查。
    virtual void setPriorityIndex(int priorityIndex) = 0;
    [[nodiscard]] virtual bool hasDeadline() const noexcept = 0;
    [[nodiscard]] virtual QString deadlineDisplayText() const = 0;
    [[nodiscard]] virtual int deadlineYear() const = 0;
    [[nodiscard]] virtual int deadlineMonth() const = 0;
    [[nodiscard]] virtual int deadlineDay() const = 0;
    [[nodiscard]] virtual int deadlineHour() const = 0;
    [[nodiscard]] virtual int deadlineMinute() const = 0;
    [[nodiscard]] virtual bool hasEstimatedDuration() const noexcept = 0;
    [[nodiscard]] virtual QString estimatedDurationDisplayText() const = 0;
    [[nodiscard]] virtual int estimatedDays() const noexcept = 0;
    [[nodiscard]] virtual int estimatedHours() const noexcept = 0;
    [[nodiscard]] virtual int estimatedMinutePart() const noexcept = 0;
    [[nodiscard]] virtual int minimumEstimatedMinutes() const noexcept = 0;
    [[nodiscard]] virtual int maximumEstimatedMinutes() const noexcept = 0;
    [[nodiscard]] virtual QStringList priorityOptions() const = 0;
    [[nodiscard]] virtual QVariantList categoryOptions() const = 0;
    [[nodiscard]] virtual QString selectedCategoryId() const = 0;
    /// 使用稳定 TaskCategoryId 更新类别草稿；空字符串表示未分类。
    virtual void setSelectedCategoryId(const QString &categoryId) = 0;
    [[nodiscard]] virtual QString selectedCategoryName() const = 0;
    [[nodiscard]] virtual QString selectedCategoryAccent() const = 0;
    [[nodiscard]] virtual bool hasCategory() const noexcept = 0;
    [[nodiscard]] virtual bool dirty() const noexcept = 0;
    [[nodiscard]] virtual bool canSave() const noexcept = 0;
    [[nodiscard]] virtual QString validationMessage() const = 0;
    [[nodiscard]] virtual int predecessorCandidateCount() const noexcept = 0;
    [[nodiscard]] virtual int selectedPredecessorCount() const noexcept = 0;
    [[nodiscard]] virtual QString predecessorSummaryText() const = 0;
    [[nodiscard]] virtual bool canConfigurePredecessors() const noexcept = 0;

public slots:
    /// 开启创建会话并初始化空白草稿和可选前置候选。
    virtual bool beginCreate() = 0;
    /// 按稳定 TaskId 加载可编辑 Todo；不存在或不可编辑时返回 false。
    virtual bool beginEdit(const QString &taskId) = 0;
    /// 用类型化日期时间分量更新截止时间；非法组合返回 false，不解析自由文本。
    virtual bool setDeadlineSelection(int year, int month, int day, int hour, int minute) = 0;
    /// 清除可选截止时间。
    virtual void clearDeadline() = 0;
    /// 用日/时/分更新预计用时；具体实现换算总分钟并调用 Model 校验。
    virtual bool setEstimatedDuration(int days, int hours, int minutes) = 0;
    /// 清除可选预计用时。
    virtual void clearEstimatedDuration() = 0;
    /// 开启创建前置临时选择；尚未修改正式创建草稿。
    virtual void beginPredecessorSelection() = 0;
    /// 切换临时前置选择；候选不存在或不可选时返回 false。
    virtual bool setCreationPredecessorSelected(const QString &taskId, bool selected) = 0;
    /// 接受临时选择并一次合并到创建草稿。
    virtual void acceptPredecessorSelection() = 0;
    /// 放弃临时选择，恢复进入子会话前的正式草稿。
    virtual void cancelPredecessorSelection() = 0;
    /// 清空创建草稿中的全部前置关系。
    virtual void clearCreationPredecessors() = 0;
    /// 一次提交完整创建/编辑草稿；成功发 saved()，失败保留会话并发布通知。
    virtual bool save() = 0;
    /// 放弃整个编辑会话，不调用 Model 写命令。
    virtual void cancel() = 0;

signals:
    /// 会话打开或关闭后发送，控制编辑器可见性与生命周期绑定。
    void sessionActiveChanged();
    /// 创建/编辑模式、稳定 TaskId 或前置配置资格变化后发送。
    void modeChanged();
    /// 以下细粒度通知分别对应草稿字段，避免无关控件全部刷新。
    void titleChanged();
    void descriptionChanged();
    void currentStatusTextChanged();
    void priorityIndexChanged();
    void deadlineChanged();
    void estimatedDurationChanged();
    /// dirty、canSave 或 validationMessage 任一变化后发送。
    void formStateChanged();
    /// 前置候选模型整体变化后发送；行内容仍配合 QAbstractItemModel 标准信号。
    void predecessorCandidatesChanged();
    /// 前置选中 Role、数量或摘要变化后发送。
    void predecessorSelectionChanged();
    /// 可选类别集合变化后发送。
    void categoryOptionsChanged();
    /// 当前类别 ID、名称、颜色或有无类别状态变化后发送。
    void categoryChanged();
    /// 保存成功的流程结果，携带稳定 TaskId 供 View 关闭并定位任务。
    void saved(const QString &taskId);
    /// 整个编辑会话被取消的流程通知。
    void cancelled();
    /// 请求 View 展示校验/持久化错误，不直接操纵 QMessageBox。
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    /// 仅供具体编辑器 ViewModel 构造，Widget 只能接收 Contract 引用。
    explicit TaskEditorContract(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
};

} // namespace smartmate::viewmodel
