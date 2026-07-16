#pragma once

#include "common/presentation/UiNotification.h"
#include "viewmodel/contracts/TaskPresentationTypes.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>

namespace smartmate::viewmodel {

/// 依赖图节点、布局、过滤和选择详情的抽象展示契约。
///
/// 主模型提供节点 Role，edges()/关系子模型提供边和邻接详情；Widget 只负责绘制、
/// 平移缩放和稳定 ID 事件转发，不得计算拓扑层级、闭包、坐标或箭头几何。
class TaskGraphContract : public QAbstractListModel {
    Q_OBJECT
    // 子模型对象地址在 Contract 生命周期内稳定，内容变化通过各自 QAbstractItemModel 信号通知。
    Q_PROPERTY(QAbstractItemModel *edges READ edges CONSTANT)
    Q_PROPERTY(QAbstractItemModel *selectedPredecessors READ selectedPredecessors CONSTANT)
    Q_PROPERTY(QAbstractItemModel *selectedSuccessors READ selectedSuccessors CONSTANT)
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY contentWidthChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(int statusFilterIndex READ statusFilterIndex WRITE setStatusFilterIndex
                   NOTIFY statusFilterIndexChanged)
    Q_PROPERTY(QStringList statusFilterOptions READ statusFilterOptions CONSTANT)
    Q_PROPERTY(QVariantList categoryFilterOptions READ categoryFilterOptions
                   NOTIFY categoryOptionsChanged)
    Q_PROPERTY(int categoryFilterMode READ categoryFilterMode NOTIFY categoryFilterChanged)
    Q_PROPERTY(QString categoryFilterCategoryId READ categoryFilterCategoryId
                   NOTIFY categoryFilterChanged)
    Q_PROPERTY(int taskCount READ taskCount NOTIFY graphChanged)
    Q_PROPERTY(int blockedCount READ blockedCount NOTIFY graphChanged)
    Q_PROPERTY(QString currentTaskId READ currentTaskId NOTIFY graphChanged)
    Q_PROPERTY(QString selectedTaskId READ selectedTaskId NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedTaskTitle READ selectedTaskTitle NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDescription READ selectedDescription NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedStatusText READ selectedStatusText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedPriorityText READ selectedPriorityText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDeadlineText READ selectedDeadlineText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedEstimatedDurationText READ selectedEstimatedDurationText
                   NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedBlockingReason READ selectedBlockingReason NOTIFY selectionChanged)
    Q_PROPERTY(int selectedUnlockCount READ selectedUnlockCount NOTIFY selectionChanged)
    Q_PROPERTY(int selectedPredecessorCount READ selectedPredecessorCount NOTIFY selectionChanged)
    Q_PROPERTY(int selectedSuccessorCount READ selectedSuccessorCount NOTIFY selectionChanged)
    Q_PROPERTY(qreal selectedNodeCenterX READ selectedNodeCenterX NOTIFY selectionChanged)
    Q_PROPERTY(qreal selectedNodeCenterY READ selectedNodeCenterY NOTIFY selectionChanged)
    Q_PROPERTY(bool canEditSelectedDependencies READ canEditSelectedDependencies NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCategoryName READ selectedCategoryName NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCategoryAccent READ selectedCategoryAccent NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedHasCategory READ selectedHasCategory NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedCoreNode READ selectedCoreNode NOTIFY selectionChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY graphChanged)

public:
    /// 节点模型的稳定 Role；包含 Model 语义投影和 ViewModel 布局结果。
    enum Role {
        TaskIdRole = Qt::UserRole + 1,
        ShortIdRole,
        TitleRole,
        StatusTextRole,
        StatusIndexRole,
        PriorityTextRole,
        DeadlineTextRole,
        UnlockCountRole,
        BlockedRole,
        BlockingReasonTextRole,
        ArchivedRole,
        CanEditDependenciesRole,
        // 节点位置和尺寸使用场景坐标像素，由 ViewModel 布局计算，Widget 只读取绘制。
        NodeXRole,
        NodeYRole,
        NodeWidthRole,
        NodeHeightRole,
        // 以下 Role 描述选择强调、搜索匹配与类别裁剪上下文。
        SelectedRole,
        EmphasisLevelRole,
        FilterMatchedRole,
        CategoryNameRole,
        CategoryAccentRole,
        HasCategoryRole,
        CoreNodeRole,
    };
    Q_ENUM(Role)

    enum EmphasisLevel {
        /// 未受当前选择影响的默认节点。
        NormalEmphasis = 0,
        /// 与选择节点无依赖关系的弱化节点。
        UnrelatedEmphasis,
        /// 位于间接前驱或后继闭包中的节点。
        TransitiveEmphasis,
        /// 与选择节点直接相连的节点。
        DirectEmphasis,
        /// 当前选中节点。
        SelectedEmphasis,
    };
    Q_ENUM(EmphasisLevel)

    /// edges() 子模型的稳定 Role；Widget 不得通过 roleName 字符串反射读取边。
    enum EdgeRole {
        EdgePredecessorIdRole = Qt::UserRole + 1,
        EdgeSuccessorIdRole,
        // 正交路径点和箭头顶点均使用场景坐标像素，Widget 不再计算几何。
        EdgeRoutePointsRole,
        EdgeArrowTipXRole,
        EdgeArrowTipYRole,
        EdgeArrowLeftXRole,
        EdgeArrowLeftYRole,
        EdgeArrowRightXRole,
        EdgeArrowRightYRole,
        EdgeSatisfiedRole,
        EdgeCancelledRole,
        EdgeHighlightedRole,
        EdgeDimmedRole,
        EdgeHoveredRole,
    };
    Q_ENUM(EdgeRole)

    /// selectedPredecessors()/selectedSuccessors() 子模型的稳定 Role。
    enum RelationRole {
        RelationTaskIdRole = Qt::UserRole + 1,
        RelationTitleRole,
        RelationStatusTextRole,
        RelationTextRole,
    };
    Q_ENUM(RelationRole)

    ~TaskGraphContract() override = default;

    /// 返回稳定边子模型；所有权仍归具体 ViewModel，Widget 不得删除。
    [[nodiscard]] virtual QAbstractItemModel *edges() noexcept = 0;
    /// 返回当前选中节点的直接前置子模型；指针在 Contract 生命周期内稳定。
    [[nodiscard]] virtual QAbstractItemModel *selectedPredecessors() noexcept = 0;
    /// 返回当前选中节点的直接后继子模型；指针在 Contract 生命周期内稳定。
    [[nodiscard]] virtual QAbstractItemModel *selectedSuccessors() noexcept = 0;
    /// 布局内容宽度，单位为场景坐标像素。
    [[nodiscard]] virtual qreal contentWidth() const noexcept = 0;
    /// 布局内容高度，单位为场景坐标像素。
    [[nodiscard]] virtual qreal contentHeight() const noexcept = 0;
    // 其余 getter 提供过滤、统计与选择详情快照；建立绑定时先同步读取。
    [[nodiscard]] virtual QString searchText() const = 0;
    /// 更新图搜索会话状态，不持久化，也不改变 Model 节点集合。
    virtual void setSearchText(const QString &searchText) = 0;
    [[nodiscard]] virtual int statusFilterIndex() const noexcept = 0;
    [[nodiscard]] virtual QStringList statusFilterOptions() const = 0;
    /// 更新状态筛选索引；具体 ViewModel 负责范围检查和匹配投影。
    virtual void setStatusFilterIndex(int index) = 0;
    [[nodiscard]] virtual QVariantList categoryFilterOptions() const = 0;
    [[nodiscard]] virtual int categoryFilterMode() const noexcept = 0;
    [[nodiscard]] virtual QString categoryFilterCategoryId() const = 0;
    [[nodiscard]] virtual int taskCount() const noexcept = 0;
    [[nodiscard]] virtual int blockedCount() const noexcept = 0;
    [[nodiscard]] virtual QString currentTaskId() const = 0;
    [[nodiscard]] virtual QString selectedTaskId() const = 0;
    [[nodiscard]] virtual QString selectedTaskTitle() const = 0;
    [[nodiscard]] virtual QString selectedDescription() const = 0;
    [[nodiscard]] virtual QString selectedStatusText() const = 0;
    [[nodiscard]] virtual QString selectedPriorityText() const = 0;
    [[nodiscard]] virtual QString selectedDeadlineText() const = 0;
    [[nodiscard]] virtual QString selectedEstimatedDurationText() const = 0;
    [[nodiscard]] virtual QString selectedBlockingReason() const = 0;
    [[nodiscard]] virtual int selectedUnlockCount() const noexcept = 0;
    [[nodiscard]] virtual int selectedPredecessorCount() const noexcept = 0;
    [[nodiscard]] virtual int selectedSuccessorCount() const noexcept = 0;
    /// 选中节点中心的场景 X 坐标，供视图定位而非重新推导布局。
    [[nodiscard]] virtual qreal selectedNodeCenterX() const noexcept = 0;
    /// 选中节点中心的场景 Y 坐标。
    [[nodiscard]] virtual qreal selectedNodeCenterY() const noexcept = 0;
    [[nodiscard]] virtual bool canEditSelectedDependencies() const noexcept = 0;
    [[nodiscard]] virtual QString selectedCategoryName() const = 0;
    [[nodiscard]] virtual QString selectedCategoryAccent() const = 0;
    [[nodiscard]] virtual bool selectedHasCategory() const noexcept = 0;
    [[nodiscard]] virtual bool selectedCoreNode() const noexcept = 0;
    [[nodiscard]] virtual bool empty() const noexcept = 0;

public slots:
    /// 从 Service 重新生成领域图快照和布局投影；通常由 Model 失效通知触发。
    virtual void reload() = 0;
    /// 按稳定 TaskId 选择节点；不存在或不可见时返回 false。
    virtual bool selectTask(const QString &taskId) = 0;
    /// 清除选中和关系详情，不改变 Model 数据。
    virtual void clearSelection() = 0;
    /// 定位并选择当前筛选下第一个匹配节点；没有匹配项返回 false。
    virtual bool locateFirstMatch() = 0;
    /// 选择 Model 计划中的当前任务；当前图不可见时返回 false。
    virtual bool selectCurrentTask() = 0;
    /// 更新类别裁剪模式与稳定类别 ID；非法组合返回 false。
    virtual bool setCategoryFilter(int mode, const QString &categoryId = {}) = 0;
    /// 设置悬停节点，仅影响强调与边高亮的会话投影。
    virtual void setHoveredTask(const QString &taskId) = 0;
    /// 清除悬停投影。
    virtual void clearHoveredTask() = 0;

signals:
    /// 布局重算导致画布宽度变化。
    void contentWidthChanged();
    /// 布局重算导致画布高度变化。
    void contentHeightChanged();
    /// 搜索文本实际变化。
    void searchTextChanged();
    /// 状态筛选实际变化。
    void statusFilterIndexChanged();
    /// 可选类别集合变化，绑定方应重新读取 categoryFilterOptions。
    void categoryOptionsChanged();
    /// 类别筛选模式或稳定类别 ID 变化。
    void categoryFilterChanged();
    /// 选中节点、关系子模型或选中详情变化后发送。
    void selectionChanged();
    /// 节点集合、统计、当前任务或节点投影整体变化；行数据仍配合标准模型信号。
    void graphChanged();
    /// 请求 View 展示图查询或命令错误，不承担节点数据绑定。
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    /// 仅供具体图 ViewModel 构造；Widget 只能持有抽象 Contract 引用。
    explicit TaskGraphContract(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
};

} // namespace smartmate::viewmodel
