#include "TaskGraphLayout.h"

#include <QMap>
#include <QStringList>

#include <algorithm>

namespace smartmate::viewmodel {
namespace {

constexpr qreal canvasMargin = 52.0;
constexpr qreal layerGap = 110.0;
constexpr qreal columnGap = 36.0;
constexpr qreal arrowLength = 12.0;
constexpr qreal arrowHalfWidth = 6.0;
constexpr int layoutSweeps = 4;

struct LayoutItem final {
    QString key;
    int stableOrder{0};
};

[[nodiscard]] QString stableId(const model::TaskId &id)
{
    return id.toString(QUuid::WithoutBraces);
}

[[nodiscard]] QString blockingReasonText(
    const QList<model::TaskId> &blockingIds,
    const QHash<model::TaskId, QString> &taskTitles,
    const QHash<QString, int> &titleCounts)
{
    if (blockingIds.isEmpty()) {
        return QStringLiteral("存在尚未完成或取消的前置任务");
    }
    QStringList names;
    for (const model::TaskId &id : blockingIds) {
        const QString title = taskTitles.value(id);
        const QString shortId = stableId(id).left(8);
        names.append(title.isEmpty() ? QStringLiteral("未知任务（%1）").arg(shortId)
                     : titleCounts.value(title) > 1
                         ? QStringLiteral("“%1”（%2）").arg(title, shortId)
                         : QStringLiteral("“%1”").arg(title));
    }
    return QStringLiteral("等待%1完成或取消")
        .arg(names.join(QStringLiteral("、")));
}

[[nodiscard]] qreal medianPosition(const QList<QString> &neighbors,
                                   const QHash<QString, qreal> &positions,
                                   const qreal fallback)
{
    QList<qreal> values;
    for (const QString &neighbor : neighbors) {
        if (positions.contains(neighbor)) values.append(positions.value(neighbor));
    }
    if (values.isEmpty()) return fallback;
    std::sort(values.begin(), values.end());
    const qsizetype middle = values.size() / 2;
    return values.size() % 2 == 0
        ? (values.at(middle - 1) + values.at(middle)) / 2.0
        : values.at(middle);
}

[[nodiscard]] QHash<QString, qreal> layerPositions(
    const QList<LayoutItem> &items)
{
    QHash<QString, qreal> positions;
    for (qsizetype index = 0; index < items.size(); ++index) {
        positions.insert(items.at(index).key, static_cast<qreal>(index));
    }
    return positions;
}

void appendPoint(QVariantList &points, const QPointF &point)
{
    if (points.isEmpty() || points.constLast().toPointF() != point) {
        points.append(QVariant::fromValue(point));
    }
}

} // namespace

TaskGraphLayoutResult layoutTaskGraph(const model::TaskGraphSnapshot &snapshot)
{
    QHash<model::TaskId, QString> taskTitles;
    QHash<QString, int> titleCounts;
    QMap<int, QList<LayoutItem>> layers;
    QHash<QString, int> levelByKey;
    QHash<QString, int> stableOrder;
    int maximumLevel = 0;
    int nextStableOrder = 0;
    for (const model::TaskGraphNode &node : snapshot.nodes) {
        const QString key = stableId(node.task.id());
        const int level = std::max(0, node.dependencyLevel);
        taskTitles.insert(node.task.id(), node.task.title());
        ++titleCounts[node.task.title()];
        stableOrder.insert(key, nextStableOrder);
        layers[level].append({key, nextStableOrder++});
        levelByKey.insert(key, level);
        maximumLevel = std::max(maximumLevel, level);
    }

    QHash<QString, QList<QString>> predecessors;
    QHash<QString, QList<QString>> successors;
    QList<QStringList> edgeChains;
    edgeChains.reserve(snapshot.edges.size());
    for (int edgeIndex = 0; edgeIndex < snapshot.edges.size(); ++edgeIndex) {
        const auto &edge = snapshot.edges.at(edgeIndex);
        const QString source = stableId(edge.dependency.predecessorId);
        const QString target = stableId(edge.dependency.successorId);
        const int sourceLevel = levelByKey.value(source);
        const int targetLevel = levelByKey.value(target);
        QStringList chain{source};
        QString previous = source;
        for (int level = sourceLevel + 1; level < targetLevel; ++level) {
            const QString dummy = QStringLiteral("dummy:%1:%2").arg(edgeIndex).arg(level);
            stableOrder.insert(dummy, nextStableOrder);
            layers[level].append({dummy, nextStableOrder++});
            levelByKey.insert(dummy, level);
            successors[previous].append(dummy);
            predecessors[dummy].append(previous);
            previous = dummy;
            chain.append(dummy);
        }
        successors[previous].append(target);
        predecessors[target].append(previous);
        chain.append(target);
        edgeChains.append(chain);
    }

    for (int sweep = 0; sweep < layoutSweeps; ++sweep) {
        for (int level = 1; level <= maximumLevel; ++level) {
            const auto positions = layerPositions(layers.value(level - 1));
            auto &items = layers[level];
            std::stable_sort(items.begin(), items.end(), [&](const auto &left,
                                                             const auto &right) {
                const qreal leftValue = medianPosition(
                    predecessors.value(left.key), positions,
                    stableOrder.value(left.key));
                const qreal rightValue = medianPosition(
                    predecessors.value(right.key), positions,
                    stableOrder.value(right.key));
                return leftValue == rightValue
                    ? left.stableOrder < right.stableOrder : leftValue < rightValue;
            });
        }
        for (int level = maximumLevel - 1; level >= 0; --level) {
            const auto positions = layerPositions(layers.value(level + 1));
            auto &items = layers[level];
            std::stable_sort(items.begin(), items.end(), [&](const auto &left,
                                                             const auto &right) {
                const qreal leftValue = medianPosition(
                    successors.value(left.key), positions,
                    stableOrder.value(left.key));
                const qreal rightValue = medianPosition(
                    successors.value(right.key), positions,
                    stableOrder.value(right.key));
                return leftValue == rightValue
                    ? left.stableOrder < right.stableOrder : leftValue < rightValue;
            });
        }
    }

    int maximumColumns = 0;
    QHash<QString, QPointF> keyCenters;
    QHash<model::TaskId, QPointF> positions;
    for (auto layer = layers.cbegin(); layer != layers.cend(); ++layer) {
        maximumColumns = std::max(maximumColumns,
                                  static_cast<int>(layer.value().size()));
    }
    const qreal widestLayer = maximumColumns > 0
        ? maximumColumns * taskGraphNodeWidth
              + (maximumColumns - 1) * columnGap
        : 0.0;
    for (auto layer = layers.cbegin(); layer != layers.cend(); ++layer) {
        const qreal layerWidth = layer.value().size() * taskGraphNodeWidth
            + std::max<qsizetype>(0, layer.value().size() - 1) * columnGap;
        const qreal left = canvasMargin + (widestLayer - layerWidth) / 2.0;
        for (qsizetype column = 0; column < layer.value().size(); ++column) {
            const QString key = layer.value().at(column).key;
            const qreal x = left + column * (taskGraphNodeWidth + columnGap);
            const qreal y = canvasMargin
                + layer.key() * (taskGraphNodeHeight + layerGap);
            keyCenters.insert(key, {x + taskGraphNodeWidth / 2.0,
                                    y + taskGraphNodeHeight / 2.0});
        }
    }

    TaskGraphLayoutResult result;
    result.nodes.reserve(snapshot.nodes.size());
    for (const model::TaskGraphNode &node : snapshot.nodes) {
        const QPointF center = keyCenters.value(stableId(node.task.id()));
        const QPointF topLeft{center.x() - taskGraphNodeWidth / 2.0,
                              center.y() - taskGraphNodeHeight / 2.0};
        positions.insert(node.task.id(), topLeft);
        result.nodes.append({node,
                             node.dependencyState.blocked
                                 ? blockingReasonText(
                                       node.dependencyState.unsatisfiedPredecessorIds,
                                       taskTitles, titleCounts)
                                 : QString{},
                             topLeft.x(), topLeft.y()});
    }

    QHash<model::TaskId, QList<int>> outgoingEdges;
    QHash<model::TaskId, QList<int>> incomingEdges;
    QMap<int, QList<int>> bandEdges;
    for (int edgeIndex = 0; edgeIndex < snapshot.edges.size(); ++edgeIndex) {
        const auto &edge = snapshot.edges.at(edgeIndex);
        outgoingEdges[edge.dependency.predecessorId].append(edgeIndex);
        incomingEdges[edge.dependency.successorId].append(edgeIndex);
        const int sourceLevel = levelByKey.value(stableId(edge.dependency.predecessorId));
        const int targetLevel = levelByKey.value(stableId(edge.dependency.successorId));
        for (int level = sourceLevel; level < targetLevel; ++level) {
            bandEdges[level].append(edgeIndex);
        }
    }
    const auto sortByOppositeX = [&](QList<int> &indexes, const bool outgoing) {
        std::sort(indexes.begin(), indexes.end(), [&](const int left, const int right) {
            const auto &leftEdge = snapshot.edges.at(left).dependency;
            const auto &rightEdge = snapshot.edges.at(right).dependency;
            const model::TaskId leftId = outgoing
                ? leftEdge.successorId : leftEdge.predecessorId;
            const model::TaskId rightId = outgoing
                ? rightEdge.successorId : rightEdge.predecessorId;
            const qreal leftX = keyCenters.value(stableId(leftId)).x();
            const qreal rightX = keyCenters.value(stableId(rightId)).x();
            return leftX == rightX ? left < right : leftX < rightX;
        });
    };
    for (auto it = outgoingEdges.begin(); it != outgoingEdges.end(); ++it)
        sortByOppositeX(it.value(), true);
    for (auto it = incomingEdges.begin(); it != incomingEdges.end(); ++it)
        sortByOppositeX(it.value(), false);
    for (auto it = bandEdges.begin(); it != bandEdges.end(); ++it)
        std::sort(it.value().begin(), it.value().end());

    const auto groupedIndexes = [&](const QList<int> &indexes, const int edgeIndex) {
        QList<int> grouped;
        const auto resolution = snapshot.edges.at(edgeIndex).resolution;
        for (const int candidate : indexes) {
            if (snapshot.edges.at(candidate).resolution == resolution)
                grouped.append(candidate);
        }
        return grouped;
    };
    const auto portOffset = [&](const QList<int> &indexes, const int edgeIndex) {
        if (groupedIndexes(indexes, edgeIndex).size() >= 3 || indexes.size() <= 1)
            return 0.0;
        const qreal span = std::min<qreal>(120.0, taskGraphNodeWidth - 48.0);
        return -span / 2.0
            + indexes.indexOf(edgeIndex) * span / (indexes.size() - 1);
    };
    const auto laneY = [&](const int level, const int edgeIndex,
                           const int sourceLevel, const int targetLevel) {
        const QList<int> indexes = bandEdges.value(level);
        const qreal center = canvasMargin
            + level * (taskGraphNodeHeight + layerGap)
            + taskGraphNodeHeight + layerGap / 2.0;
        if (indexes.size() <= 1) return center;
        int laneEdgeIndex = edgeIndex;
        const auto &dependency = snapshot.edges.at(edgeIndex).dependency;
        if (level == sourceLevel) {
            const auto group = groupedIndexes(
                outgoingEdges.value(dependency.predecessorId), edgeIndex);
            if (group.size() >= 3) laneEdgeIndex = group.constFirst();
        }
        if (level == targetLevel - 1) {
            const auto group = groupedIndexes(
                incomingEdges.value(dependency.successorId), edgeIndex);
            if (group.size() >= 3) laneEdgeIndex = group.constFirst();
        }
        const qreal usable = layerGap - 28.0;
        return center - usable / 2.0
            + indexes.indexOf(laneEdgeIndex) * usable / (indexes.size() - 1);
    };

    result.edges.reserve(snapshot.edges.size());
    for (int edgeIndex = 0; edgeIndex < snapshot.edges.size(); ++edgeIndex) {
        const auto &edge = snapshot.edges.at(edgeIndex);
        const QPointF sourceTopLeft = positions.value(edge.dependency.predecessorId);
        const QPointF targetTopLeft = positions.value(edge.dependency.successorId);
        const qreal startX = sourceTopLeft.x() + taskGraphNodeWidth / 2.0
            + portOffset(outgoingEdges.value(edge.dependency.predecessorId), edgeIndex);
        const qreal endX = targetTopLeft.x() + taskGraphNodeWidth / 2.0
            + portOffset(incomingEdges.value(edge.dependency.successorId), edgeIndex);
        const QPointF start{startX, sourceTopLeft.y() + taskGraphNodeHeight};
        const QPointF end{endX, targetTopLeft.y()};
        QVariantList points;
        appendPoint(points, start);
        const QStringList chain = edgeChains.at(edgeIndex);
        qreal currentX = start.x();
        const int sourceLevel = levelByKey.value(chain.constFirst());
        for (int chainIndex = 0; chainIndex < chain.size() - 1; ++chainIndex) {
            const int level = sourceLevel + chainIndex;
            const qreal nextX = chainIndex == chain.size() - 2
                ? end.x() : keyCenters.value(chain.at(chainIndex + 1)).x();
            const qreal y = laneY(level, edgeIndex, sourceLevel,
                                  levelByKey.value(chain.constLast()));
            appendPoint(points, {currentX, y});
            appendPoint(points, {nextX, y});
            currentX = nextX;
        }
        appendPoint(points, end);
        result.edges.append({edge, points, end,
                             {end.x() - arrowHalfWidth, end.y() - arrowLength},
                             {end.x() + arrowHalfWidth, end.y() - arrowLength}});
        ++result.predecessorCounts[edge.dependency.successorId];
        ++result.successorCounts[edge.dependency.predecessorId];
    }

    result.contentWidth = snapshot.nodes.isEmpty()
        ? 0.0 : canvasMargin * 2.0 + widestLayer;
    result.contentHeight = snapshot.nodes.isEmpty()
        ? 0.0
        : canvasMargin * 2.0 + (maximumLevel + 1) * taskGraphNodeHeight
              + maximumLevel * layerGap;
    return result;
}

} // namespace smartmate::viewmodel
