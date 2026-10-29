import QtQuick

// 2.5D isometric plant floor — the fallback when Qt Quick 3D is not built
// in, the 3D view fails to load, the scene graph runs on the software
// backend, or FORGESIGHT_DISABLE_3D is set. Same scene model, same
// interaction contract (selectedKey / picked) as PlantView3D: zones are
// rhombus plates, devices are shaded prisms painted back-to-front on a
// Canvas, critical ones carry a pulsing ring.
Item {
    id: root

    property var scene
    property string selectedKey: ""
    signal picked(string key)

    property var zoneList: []
    property var deviceList: []
    property real pulse: 0
    property real rise: Theme.reducedMotion ? 1 : 0
    property real rotationDeg: 0   // spin around the vertical axis, by drag

    readonly property real _span: scene ? Math.max(20, scene.sceneWidth + scene.sceneDepth) : 40
    readonly property real k: Math.min(width, height * 1.7) / (_span * 0.95)
    readonly property real heightScale: 7

    function refreshLists() {
        if (!scene) return
        zoneList = scene.zones()
        deviceList = scene.devices()
        canvas.requestPaint()
    }
    Connections {
        target: root.scene
        function onSceneChanged() { root.refreshLists() }
    }
    Component.onCompleted: refreshLists()

    NumberAnimation on rise { from: 0; to: 1; duration: 1200; easing.type: Easing.OutBack; running: !Theme.reducedMotion }
    NumberAnimation on pulse { from: 0; to: 1; duration: 1600; loops: Animation.Infinite; running: !Theme.reducedMotion && root.visible }
    onRiseChanged: canvas.requestPaint()
    onRotationDegChanged: canvas.requestPaint()
    // Pulses are repainted at a modest rate rather than every frame.
    Timer { interval: 120; repeat: true; running: !Theme.reducedMotion && root.visible; onTriggered: canvas.requestPaint() }

    // Ground (x, z) rotated, then projected isometrically; y is up.
    function project(x, y, z) {
        var r = rotationDeg * Math.PI / 180
        var rx = x * Math.cos(r) - z * Math.sin(r)
        var rz = x * Math.sin(r) + z * Math.cos(r)
        return Qt.point(width / 2 + (rx - rz) * 0.866 * k,
                        height * 0.56 + (rx + rz) * 0.5 * k - y * k)
    }
    function depthOf(x, z) {
        var r = rotationDeg * Math.PI / 180
        return x * Math.sin(r) + z * Math.cos(r) + x * Math.cos(r) - z * Math.sin(r)
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        function quad(ctx, a, b, c, d, fill) {
            ctx.beginPath()
            ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.lineTo(c.x, c.y); ctx.lineTo(d.x, d.y)
            ctx.closePath()
            ctx.fillStyle = fill
            ctx.fill()
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var P = root.project

            // Zones
            for (var i = 0; i < root.zoneList.length; i++) {
                var zn = root.zoneList[i]
                var hw = zn.width / 2 + 0.6, hd = zn.depth / 2 + 0.6
                quad(ctx, P(zn.x - hw, 0, zn.z - hd), P(zn.x + hw, 0, zn.z - hd),
                     P(zn.x + hw, 0, zn.z + hd), P(zn.x - hw, 0, zn.z + hd),
                     zn.critical > 0 ? Theme.alpha(Theme.critical, 0.10) : Theme.alpha(Theme.accent, Theme.dark ? 0.07 : 0.09))
            }

            // Devices back-to-front
            var order = []
            for (var j = 0; j < root.deviceList.length; j++) order.push(j)
            order.sort(function(a, b) {
                var da = root.deviceList[a], db = root.deviceList[b]
                return root.depthOf(da.x, da.z) - root.depthOf(db.x, db.z)
            })
            var w = 0.62
            for (var n = 0; n < order.length; n++) {
                var d = root.deviceList[order[n]]
                var h = Math.max(0.05, d.height * root.heightScale * root.rise)
                var selected = d.key === root.selectedKey
                var base = d.critical ? Theme.critical : (selected ? "#ffffff" : Qt.tint(Theme.dark ? "#2a3a5a" : "#9fb3d9", Theme.alpha(Theme.accent, d.height)))

                if (d.critical) {
                    var rr = (1.2 + 2.4 * root.pulse) * root.k
                    var c = P(d.x, 0, d.z)
                    ctx.save()
                    ctx.translate(c.x, c.y)
                    ctx.scale(1, 0.5)
                    ctx.beginPath()
                    ctx.arc(0, 0, rr, 0, Math.PI * 2)
                    ctx.restore()
                    ctx.strokeStyle = Theme.alpha(Theme.critical, 0.8 * (1 - root.pulse))
                    ctx.lineWidth = 2
                    ctx.stroke()
                }

                var b0 = P(d.x - w, 0, d.z + w), b1 = P(d.x + w, 0, d.z + w), b2 = P(d.x + w, 0, d.z - w)
                var t0 = P(d.x - w, h, d.z + w), t1 = P(d.x + w, h, d.z + w), t2 = P(d.x + w, h, d.z - w), t3 = P(d.x - w, h, d.z - w)
                quad(ctx, b0, b1, t1, t0, Qt.darker(base, 1.45))
                quad(ctx, b1, b2, t2, t1, Qt.darker(base, 1.15))
                quad(ctx, t0, t1, t2, t3, Qt.lighter(base, 1.12))
                if (selected) {
                    ctx.strokeStyle = Theme.accent
                    ctx.lineWidth = 2
                    ctx.beginPath()
                    ctx.moveTo(t0.x, t0.y); ctx.lineTo(t1.x, t1.y); ctx.lineTo(t2.x, t2.y); ctx.lineTo(t3.x, t3.y)
                    ctx.closePath()
                    ctx.stroke()
                }
            }

            // Zone captions
            ctx.font = "bold 10px " + Theme.fontFamily
            ctx.textAlign = "center"
            for (var q = 0; q < root.zoneList.length; q++) {
                var z2 = root.zoneList[q]
                var lp = P(z2.x, 0, z2.z + z2.depth / 2 + 1.6)
                ctx.fillStyle = z2.critical > 0 ? Theme.critical : Theme.textSecondary
                ctx.fillText(z2.plant + " · " + z2.floor, lp.x, lp.y)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        property real lastX: 0
        property bool moved: false
        cursorShape: Qt.PointingHandCursor
        onPressed: function(m) { lastX = m.x; moved = false }
        onPositionChanged: function(m) {
            if (Math.abs(m.x - lastX) > 2) moved = true
            root.rotationDeg += (m.x - lastX) * 0.25
            lastX = m.x
        }
        onReleased: function(m) {
            if (moved) return
            // Nearest projected pillar top.
            var best = -1, bestD = 18 * 18
            for (var i = 0; i < root.deviceList.length; i++) {
                var d = root.deviceList[i]
                var p = root.project(d.x, d.height * root.heightScale * root.rise, d.z)
                var dd = (p.x - m.x) * (p.x - m.x) + (p.y - m.y) * (p.y - m.y)
                if (dd < bestD) { bestD = dd; best = i }
            }
            if (best >= 0)
                root.picked(root.deviceList[best].key)
        }
    }
}
