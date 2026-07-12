pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Shapes

Shape {
    id: edge
    required property var theme
    required property string predecessorId
    required property string successorId
    required property var routePoints
    required property real arrowTipX
    required property real arrowTipY
    required property real arrowLeftX
    required property real arrowLeftY
    required property real arrowRightX
    required property real arrowRightY
    required property bool satisfied
    required property bool cancelled
    required property bool highlighted
    required property bool dimmed
    required property bool hovered

    readonly property color edgeColor: edge.cancelled ? edge.theme.textDisabled
        : edge.satisfied ? edge.theme.done : edge.theme.warning
    objectName: "graphEdge_" + edge.predecessorId + "_" + edge.successorId
    opacity: edge.dimmed ? 0.24 : 1.0

    ShapePath {
        strokeColor: edge.edgeColor
        strokeWidth: edge.highlighted || edge.hovered ? 4 : 2.2
        strokeStyle: edge.cancelled ? ShapePath.DashLine : ShapePath.SolidLine
        dashPattern: [5, 4]
        fillColor: "transparent"
        capStyle: ShapePath.RoundCap
        joinStyle: ShapePath.RoundJoin
        PathPolyline { path: edge.routePoints }
    }
    ShapePath {
        strokeColor: edge.edgeColor
        strokeWidth: 1
        fillColor: edge.edgeColor
        startX: edge.arrowTipX
        startY: edge.arrowTipY
        PathLine { x: edge.arrowLeftX; y: edge.arrowLeftY }
        PathLine { x: edge.arrowRightX; y: edge.arrowRightY }
        PathLine { x: edge.arrowTipX; y: edge.arrowTipY }
    }
}
