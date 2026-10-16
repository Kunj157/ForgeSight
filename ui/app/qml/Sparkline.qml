import QtQuick
import QtQuick.Shapes

// Rolling mini-chart: a stroked polyline over a vertical gradient fill, with
// a glowing dot on the latest sample. Built on Shapes (GPU geometry) rather
// than Canvas, so hundreds of cards repainting each second don't each
// rasterise and upload a texture.
Item {
    id: root

    property var values: []
    property color color: Theme.accent
    property real lineWidth: 1.6

    property var _line: []
    property var _fill: []
    property point _last: Qt.point(-10, -10)

    onValuesChanged: rebuild()
    onWidthChanged: rebuild()
    onHeightChanged: rebuild()

    function rebuild() {
        var pts = values
        if (!pts || pts.length < 2 || width <= 0 || height <= 0) {
            _line = []
            _fill = []
            return
        }
        var minV = pts[0], maxV = pts[0]
        for (var i = 1; i < pts.length; i++) {
            if (pts[i] < minV) minV = pts[i]
            if (pts[i] > maxV) maxV = pts[i]
        }
        var range = (maxV - minV) || 1
        var stepX = width / (pts.length - 1)
        var line = []
        for (var j = 0; j < pts.length; j++)
            line.push(Qt.point(j * stepX, height - 4 - ((pts[j] - minV) / range) * (height - 10)))
        _last = line[line.length - 1]
        _line = line
        _fill = line.concat([Qt.point(width, height), Qt.point(0, height)])
    }

    Shape {
        anchors.fill: parent
        antialiasing: true
        visible: root._line.length > 1

        ShapePath {
            strokeColor: "transparent"
            fillGradient: LinearGradient {
                x1: 0; y1: 0; x2: 0; y2: root.height
                GradientStop { position: 0.0; color: Theme.alpha(root.color, 0.24) }
                GradientStop { position: 1.0; color: Theme.alpha(root.color, 0.0) }
            }
            PathPolyline { path: root._fill }
        }
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.lineWidth
            joinStyle: ShapePath.RoundJoin
            capStyle: ShapePath.RoundCap
            PathPolyline { path: root._line }
        }
    }

    Rectangle {
        visible: root._line.length > 1
        x: root._last.x - width / 2
        y: root._last.y - height / 2
        width: 9; height: 9; radius: 4.5
        color: Theme.alpha(root.color, 0.25)
        Rectangle { anchors.centerIn: parent; width: 5; height: 5; radius: 2.5; color: root.color }
    }
}
