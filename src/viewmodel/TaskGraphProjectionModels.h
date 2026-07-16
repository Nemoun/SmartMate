#pragma once

#include "domain/TaskGraph.h"
#include "viewmodel/contracts/TaskGraphContract.h"

#include <QAbstractListModel>
#include <QPointF>
#include <QSet>
#include <QVariantList>

namespace smartmate::viewmodel {

/// 可直接绑定绘制的边快照；路径点与箭头顶点使用场景坐标像素。
struct EdgeProjection final {
    /// Model 计算的依赖边及满足状态。
    model::TaskGraphEdge edge;
    /// ViewModel 计算的正交折线路径。
    QVariantList routePoints;
    QPointF arrowTip;
    QPointF arrowLeft;
    QPointF arrowRight;
};

/// 选中节点的单条直接关系展示行。
struct RelationProjection final {
    /// 关联任务稳定身份。
    model::TaskId taskId;
    QString title;
    QString statusText;
    QString relationText;
};

/// 依赖边的只读 Qt 模型投影；高亮与悬停仅属于会话级展示状态。
class TaskGraphEdgeListModel final : public QAbstractListModel {
public:
    enum Role {
        PredecessorIdRole = TaskGraphContract::EdgePredecessorIdRole,
        SuccessorIdRole = TaskGraphContract::EdgeSuccessorIdRole,
        RoutePointsRole = TaskGraphContract::EdgeRoutePointsRole,
        ArrowTipXRole = TaskGraphContract::EdgeArrowTipXRole,
        ArrowTipYRole = TaskGraphContract::EdgeArrowTipYRole,
        ArrowLeftXRole = TaskGraphContract::EdgeArrowLeftXRole,
        ArrowLeftYRole = TaskGraphContract::EdgeArrowLeftYRole,
        ArrowRightXRole = TaskGraphContract::EdgeArrowRightXRole,
        ArrowRightYRole = TaskGraphContract::EdgeArrowRightYRole,
        SatisfiedRole = TaskGraphContract::EdgeSatisfiedRole,
        CancelledRole = TaskGraphContract::EdgeCancelledRole,
        HighlightedRole = TaskGraphContract::EdgeHighlightedRole,
        DimmedRole = TaskGraphContract::EdgeDimmedRole,
        HoveredRole = TaskGraphContract::EdgeHoveredRole,
    };

    explicit TaskGraphEdgeListModel(QObject *parent);
    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    /// 使用模型重置协议原子替换全部边，避免 Widget 观察半成品布局。
    void replaceRows(QList<EdgeProjection> rows);
    /// 更新选择/悬停会话并只通知受影响的交互 Role。
    void setInteraction(const model::TaskId &selectedTaskId,
                        const model::TaskId &hoveredTaskId,
                        QSet<model::TaskId> relatedTaskIds);

private:
    /// 当前边布局快照，是 data() 的唯一行数据源。
    QList<EdgeProjection> m_rows;
    /// 当前选择、悬停和关联闭包，仅影响展示 Role。
    model::TaskId m_selectedTaskId;
    model::TaskId m_hoveredTaskId;
    QSet<model::TaskId> m_relatedTaskIds;
};

/// 选中任务直接前置/后继的只读详情投影。
class TaskGraphRelationListModel final : public QAbstractListModel {
public:
    enum Role {
        TaskIdRole = TaskGraphContract::RelationTaskIdRole,
        TitleRole = TaskGraphContract::RelationTitleRole,
        StatusTextRole = TaskGraphContract::RelationStatusTextRole,
        RelationTextRole = TaskGraphContract::RelationTextRole,
    };

    explicit TaskGraphRelationListModel(QObject *parent);
    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    /// 使用模型重置协议替换当前直接关系详情。
    void replaceRows(QList<RelationProjection> rows);

private:
    /// 当前选中任务的直接前置或后继展示行。
    QList<RelationProjection> m_rows;
};

} // namespace smartmate::viewmodel
