import QtQuick

// Lightweight line-icon set drawn on Canvas so the UI doesn't depend on an
// icon font or any extra Qt module. Stroke-based, rounded caps/joins to read
// as a clean, modern glyph at small sizes (16-24px).
Canvas {
    id: root
    property string name: ""
    property color color: "#ffffff"
    property real strokeWidth: 1.6

    antialiasing: true
    onNameChanged: requestPaint()
    onColorChanged: requestPaint()
    onStrokeWidthChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.clearRect(0, 0, width, height)
        ctx.strokeStyle = color
        ctx.fillStyle = color
        ctx.lineWidth = strokeWidth
        ctx.lineCap = "round"
        ctx.lineJoin = "round"

        var s = Math.min(width, height)
        var p = s * 0.18
        var cx = width / 2
        var cy = height / 2

        function line(x1, y1, x2, y2) {
            ctx.beginPath(); ctx.moveTo(x1, y1); ctx.lineTo(x2, y2); ctx.stroke()
        }
        function circle(x, y, r, fill) {
            ctx.beginPath(); ctx.arc(x, y, r, 0, Math.PI * 2)
            if (fill) ctx.fill(); else ctx.stroke()
        }

        switch (name) {
        case "grid": // devices / tiles
            var cell = (s - 2 * p) * 0.42
            var gap = (s - 2 * p) * 0.16
            for (var gy = 0; gy < 2; gy++) {
                for (var gx = 0; gx < 2; gx++) {
                    ctx.strokeRect(p + gx * (cell + gap), p + gy * (cell + gap), cell, cell)
                }
            }
            break

        case "bell": // alarms
            ctx.beginPath()
            ctx.arc(cx, cy - s * 0.04, s * 0.24, Math.PI * 1.02, Math.PI * 1.98)
            ctx.lineTo(cx + s * 0.26, cy + s * 0.22)
            ctx.lineTo(cx - s * 0.26, cy + s * 0.22)
            ctx.closePath()
            ctx.stroke()
            line(cx - s * 0.08, cy + s * 0.32, cx + s * 0.08, cy + s * 0.32)
            break

        case "chart": // history
            line(p, height - p, p, p)
            line(p, height - p, width - p, height - p)
            ctx.beginPath()
            ctx.moveTo(p + s * 0.08, height - p - s * 0.10)
            ctx.lineTo(p + s * 0.32, height - p - s * 0.36)
            ctx.lineTo(p + s * 0.52, height - p - s * 0.20)
            ctx.lineTo(p + s * 0.78, height - p - s * 0.52)
            ctx.stroke()
            break

        case "search":
            circle(cx - s * 0.06, cy - s * 0.06, s * 0.20, false)
            line(cx + s * 0.10, cy + s * 0.10, cx + s * 0.28, cy + s * 0.28)
            break

        case "close":
            line(p, p, width - p, height - p)
            line(width - p, p, p, height - p)
            break

        case "chevron-down":
            ctx.beginPath()
            ctx.moveTo(p, cy - s * 0.10)
            ctx.lineTo(cx, cy + s * 0.16)
            ctx.lineTo(width - p, cy - s * 0.10)
            ctx.stroke()
            break

        case "chevron-right":
            ctx.beginPath()
            ctx.moveTo(cx - s * 0.10, p)
            ctx.lineTo(cx + s * 0.16, cy)
            ctx.lineTo(cx - s * 0.10, height - p)
            ctx.stroke()
            break

        case "activity": // live pulse
            ctx.beginPath()
            ctx.moveTo(p, cy)
            ctx.lineTo(cx - s * 0.18, cy)
            ctx.lineTo(cx - s * 0.06, cy - s * 0.30)
            ctx.lineTo(cx + s * 0.06, cy + s * 0.30)
            ctx.lineTo(cx + s * 0.18, cy)
            ctx.lineTo(width - p, cy)
            ctx.stroke()
            break

        case "wifi-off":
            ctx.beginPath()
            ctx.arc(cx, cy + s * 0.16, s * 0.30, Math.PI * 1.2, Math.PI * 1.8)
            ctx.stroke()
            circle(cx, cy + s * 0.16, s * 0.035, true)
            line(p, p, width - p, height - p)
            break

        case "check":
            ctx.beginPath()
            ctx.moveTo(p, cy)
            ctx.lineTo(cx - s * 0.06, height - p)
            ctx.lineTo(width - p, p)
            ctx.stroke()
            break

        case "clock": // pending / queued for sync
            circle(cx, cy, s * 0.36, false)
            line(cx, cy, cx, cy - s * 0.20)
            line(cx, cy, cx + s * 0.15, cy)
            break

        case "filter":
            ctx.beginPath()
            ctx.moveTo(p, p)
            ctx.lineTo(width - p, p)
            ctx.lineTo(cx + s * 0.09, cy)
            ctx.lineTo(cx + s * 0.09, height - p)
            ctx.lineTo(cx - s * 0.09, height - p * 1.7)
            ctx.lineTo(cx - s * 0.09, cy)
            ctx.closePath()
            ctx.stroke()
            break

        case "download":
            line(cx, p, cx, height - p * 1.7)
            ctx.beginPath()
            ctx.moveTo(cx - s * 0.16, height - p * 1.95)
            ctx.lineTo(cx, height - p * 1.55)
            ctx.lineTo(cx + s * 0.16, height - p * 1.95)
            ctx.stroke()
            line(p, height - p, width - p, height - p)
            break

        case "trash":
            line(p * 1.1, p * 1.6, width - p * 1.1, p * 1.6)
            ctx.strokeRect(cx - s * 0.20, p * 1.6, s * 0.40, height - p * 2.2)
            line(cx - s * 0.13, p * 1.15, cx + s * 0.13, p * 1.15)
            break

        case "gauge": // generic sensor
            ctx.beginPath()
            ctx.arc(cx, cy + s * 0.08, s * 0.28, Math.PI, 0)
            ctx.stroke()
            line(cx, cy + s * 0.08, cx + s * 0.18, cy - s * 0.14)
            circle(cx, cy + s * 0.08, s * 0.045, true)
            break

        case "layers": // brand mark
            ctx.beginPath()
            ctx.moveTo(cx, p)
            ctx.lineTo(width - p, cy - s * 0.06)
            ctx.lineTo(cx, cy + s * 0.20)
            ctx.lineTo(p, cy - s * 0.06)
            ctx.closePath()
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(p, cy + s * 0.10)
            ctx.lineTo(cx, cy + s * 0.36)
            ctx.lineTo(width - p, cy + s * 0.10)
            ctx.stroke()
            break

        default:
            circle(cx, cy, s * 0.28, false)
        }
    }
}
