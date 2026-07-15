#include "TaskGraphProjectionModels.h"

#include <utility>

namespace smartmate::viewmodel {
namespace {

[[nodiscard]] QString stableId(const model::TaskId &id)
{
    return id.toString(QUuid::WithoutBraces);
}

} // namespace

TaskGraphEdgeListModel::TaskGraphEdgeListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TaskGraphEdgeListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant TaskGraphEdgeListModel::data(const QModelIndex &index, const int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const EdgeProjection &row = m_rows.at(index.row());
    const model::TaskId source = row.edge.dependency.predecessorId;
    const model::TaskId target = row.edge.dependency.successorId;
    const bool selectedPath = !m_selectedTaskId.isNull()
        && m_relatedTaskIds.contains(source) && m_relatedTaskIds.contains(target);
    const bool hovered = !m_hoveredTaskId.isNull()
        && (source == m_hoveredTaskId || target == m_hoveredTaskId);
    switch (role) {
    case PredecessorIdRole: return stableId(source);
    case SuccessorIdRole: return stableId(target);
    case RoutePointsRole: return row.routePoints;
    case ArrowTipXRole: return row.arrowTip.x();
    case ArrowTipYRole: return row.arrowTip.y();
    case ArrowLeftXRole: return row.arrowLeft.x();
    case ArrowLeftYRole: return row.arrowLeft.y();
    case ArrowRightXRole: return row.arrowRight.x();
    case ArrowRightYRole: return row.arrowRight.y();
    case SatisfiedRole:
        return row.edge.resolution == model::TaskDependencyResolution::Satisfied;
    case CancelledRole:
        return row.edge.resolution == model::TaskDependencyResolution::Cancelled;
    case HighlightedRole: return selectedPath;
    case DimmedRole: return !m_selectedTaskId.isNull() && !selectedPath && !hovered;
    case HoveredRole: return hovered;
    default: return {};
    }
}

QHash<int, QByteArray> TaskGraphEdgeListModel::roleNames() const
{
    return {{PredecessorIdRole, "predecessorId"},
            {SuccessorIdRole, "successorId"},
            {RoutePointsRole, "routePoints"},
            {ArrowTipXRole, "arrowTipX"}, {ArrowTipYRole, "arrowTipY"},
            {ArrowLeftXRole, "arrowLeftX"}, {ArrowLeftYRole, "arrowLeftY"},
            {ArrowRightXRole, "arrowRightX"}, {ArrowRightYRole, "arrowRightY"},
            {SatisfiedRole, "satisfied"}, {CancelledRole, "cancelled"},
            {HighlightedRole, "highlighted"}, {DimmedRole, "dimmed"},
            {HoveredRole, "hovered"}};
}

void TaskGraphEdgeListModel::replaceRows(QList<EdgeProjection> rows)
{
    // 边几何属于一个整体布局快照，reset 可避免 Widget 绘制新旧坐标混合状态。
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
}

void TaskGraphEdgeListModel::setInteraction(
    const model::TaskId &selectedTaskId,
    const model::TaskId &hoveredTaskId,
    QSet<model::TaskId> relatedTaskIds)
{
    m_selectedTaskId = selectedTaskId;
    m_hoveredTaskId = hoveredTaskId;
    m_relatedTaskIds = std::move(relatedTaskIds);
    if (!m_rows.isEmpty()) {
        // 几何和依赖满足状态未变，只通知三个会话交互 Role。
        emit dataChanged(index(0), index(m_rows.size() - 1),
                         {HighlightedRole, DimmedRole, HoveredRole});
    }
}

TaskGraphRelationListModel::TaskGraphRelationListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TaskGraphRelationListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant TaskGraphRelationListModel::data(const QModelIndex &index,
                                          const int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const RelationProjection &row = m_rows.at(index.row());
    switch (role) {
    case TaskIdRole: return stableId(row.taskId);
    case TitleRole: return row.title;
    case StatusTextRole: return row.statusText;
    case RelationTextRole: return row.relationText;
    default: return {};
    }
}

QHash<int, QByteArray> TaskGraphRelationListModel::roleNames() const
{
    return {{TaskIdRole, "taskId"}, {TitleRole, "title"},
            {StatusTextRole, "statusText"}, {RelationTextRole, "relationText"}};
}

void TaskGraphRelationListModel::replaceRows(QList<RelationProjection> rows)
{
    // 选择切换会改变整个直接关系集合，使用 reset 发布原子详情快照。
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
}

} // namespace smartmate::viewmodel
