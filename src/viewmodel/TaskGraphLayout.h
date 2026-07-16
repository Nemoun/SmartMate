#pragma once

#include "TaskGraphProjectionModels.h"

#include <QHash>

namespace smartmate::viewmodel {

/// 节点固定宽度，单位为 QGraphicsScene 场景像素。
inline constexpr qreal taskGraphNodeWidth = 220.0;
/// 节点固定高度，单位为场景像素。
inline constexpr qreal taskGraphNodeHeight = 92.0;

/// 单个领域节点的展示投影；坐标由 ViewModel 布局计算，不持久化。
struct TaskGraphNodeProjection final {
    /// Model 提供的节点语义、闭包和命令资格。
    model::TaskGraphNode node;
    /// 根据阻塞 ID 和标题生成的中文说明。
    QString blockingReasonText;
    /// 节点左上角场景坐标，单位为像素。
    qreal x{0.0};
    qreal y{0.0};
};

/// 一次完整图布局结果，供 TaskGraphViewModel 原子替换所有绑定模型。
struct TaskGraphLayoutResult final {
    /// 带坐标的节点投影。
    QList<TaskGraphNodeProjection> nodes;
    /// 带正交路径和箭头顶点的边投影。
    QList<EdgeProjection> edges;
    /// 按稳定 ID 索引的直接前置/后继数量。
    QHash<model::TaskId, int> predecessorCounts;
    QHash<model::TaskId, int> successorCounts;
    /// 图内容包围尺寸，单位为场景像素。
    qreal contentWidth{0.0};
    qreal contentHeight{0.0};
};

/// 将 Model 拓扑层级投影为像素坐标、正交路由和箭头几何。
[[nodiscard]] TaskGraphLayoutResult layoutTaskGraph(
    const model::TaskGraphSnapshot &snapshot);

} // namespace smartmate::viewmodel
