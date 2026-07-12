#pragma once

#include "TaskGraphProjectionModels.h"

#include <QHash>

namespace smartmate::viewmodel {

inline constexpr qreal taskGraphNodeWidth = 220.0;
inline constexpr qreal taskGraphNodeHeight = 92.0;

struct TaskGraphNodeProjection final {
    model::TaskGraphNode node;
    QString blockingReasonText;
    qreal x{0.0};
    qreal y{0.0};
};

struct TaskGraphLayoutResult final {
    QList<TaskGraphNodeProjection> nodes;
    QList<EdgeProjection> edges;
    QHash<model::TaskId, int> predecessorCounts;
    QHash<model::TaskId, int> successorCounts;
    qreal contentWidth{0.0};
    qreal contentHeight{0.0};
};

/// 将 Model 拓扑层级投影为像素坐标、正交路由和箭头几何。
[[nodiscard]] TaskGraphLayoutResult layoutTaskGraph(
    const model::TaskGraphSnapshot &snapshot);

} // namespace smartmate::viewmodel
