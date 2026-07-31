import QtQuick
import "Theme.js" as Theme

// A card with a real soft drop-shadow, rendered on a Canvas using the
// standard Context2D shadow* properties (no Qt5Compat.GraphicalEffects /
// QtQuick.Effects dependency required — those modules aren't available in
// every Qt install, e.g. this project's Qt 6.4.2 doesn't ship them).
Item {
    id: root
    default property alias content: contentItem.data

    property alias color: bg.color
    property real cardRadius: Theme.radiusLg
    property alias border: bg.border
    property real shadowBlur: 18
    property color shadowColor: "#00000066"
    property real shadowOffsetY: 5
    property bool glow: false
    property color glowColor: Theme.accentGlow

    Canvas {
        id: shadowCanvas
        anchors.fill: parent
        anchors.margins: -Math.ceil(root.shadowBlur * 1.6)
        renderStrategy: Canvas.Cooperative

        function drawRoundedRect(ctx, x, y, w, h, r) {
            ctx.beginPath()
            ctx.moveTo(x + r, y)
            ctx.lineTo(x + w - r, y)
            ctx.quadraticCurveTo(x + w, y, x + w, y + r)
            ctx.lineTo(x + w, y + h - r)
            ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
            ctx.lineTo(x + r, y + h)
            ctx.quadraticCurveTo(x, y + h, x, y + h - r)
            ctx.lineTo(x, y + r)
            ctx.quadraticCurveTo(x, y, x + r, y)
            ctx.closePath()
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)

            var m = Math.ceil(root.shadowBlur * 1.6)
            var x = m, y = m, w = root.width, h = root.height

            if (root.glow) {
                ctx.shadowColor = root.glowColor
                ctx.shadowBlur = root.shadowBlur * 1.3
                ctx.shadowOffsetX = 0
                ctx.shadowOffsetY = 0
                ctx.fillStyle = "#00000000"
                ctx.globalAlpha = 0.55
                drawRoundedRect(ctx, x, y, w, h, root.cardRadius)
                ctx.fill()
                ctx.globalAlpha = 1.0
            }

            ctx.shadowColor = root.shadowColor
            ctx.shadowBlur = root.shadowBlur
            ctx.shadowOffsetX = 0
            ctx.shadowOffsetY = root.shadowOffsetY
            ctx.fillStyle = "#101318"
            drawRoundedRect(ctx, x, y, w, h, root.cardRadius)
            ctx.fill()
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: root.cardRadius
        color: Theme.bgCard
        border.width: 1
        border.color: Theme.border
    }

    Item {
        id: contentItem
        anchors.fill: parent
    }
}
