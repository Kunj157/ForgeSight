import QtQuick
import QtQuick3D
import ForgeSight.Plant3D

// Real-time 3D plant floor. Every device is a GPU-instanced pillar whose
// height follows its live value and whose colour follows alarm state;
// critical devices emit expanding red ground rings. Orbit with drag, zoom
// with the wheel, click a pillar to select it. Labels are crisp 2D text
// projected from the 3D scene each frame.
Item {
    id: root

    property var scene                     // PlantSceneModel
    property string selectedKey: ""
    signal picked(string key)

    readonly property bool animate: !Theme.reducedMotion

    // Orbit state
    property real yaw: -32
    property real pitch: 34
    property real distance: Math.max(40, Math.max(scene ? scene.sceneWidth : 40, scene ? scene.sceneDepth : 40) * 1.75)
    property vector3d target: Qt.vector3d(0, 0, 0)
    property bool interacting: false
    property real pulse: 0
    property real rise: animate ? 0 : 1

    property var zoneList: []
    property var deviceList: []
    function refreshLists() {
        if (!scene) return
        zoneList = scene.zones()
        deviceList = scene.devices()
    }
    Connections {
        target: root.scene
        function onSceneChanged() { root.refreshLists() }
    }

    Component.onCompleted: {
        refreshLists()
        if (animate)
            intro.start()
    }

    // Fly-in: camera swoops down while the pillars grow out of the floor.
    ParallelAnimation {
        id: intro
        NumberAnimation { target: root; property: "pitch"; from: 68; to: 34; duration: 1600; easing.type: Easing.OutCubic }
        NumberAnimation { target: root; property: "yaw"; from: -75; to: -32; duration: 1800; easing.type: Easing.OutCubic }
        NumberAnimation { target: root; property: "rise"; from: 0; to: 1; duration: 1400; easing.type: Easing.OutBack }
    }

    NumberAnimation on pulse {
        from: 0; to: 1
        duration: 1600
        loops: Animation.Infinite
        running: root.animate && root.visible
    }

    // Slow idle drift so the scene feels alive; pauses while the user orbits.
    Timer {
        interval: 33
        repeat: true
        running: root.animate && root.visible && !root.interacting && !intro.running
        onTriggered: root.yaw += 0.04
    }

    Behavior on target { enabled: root.animate; Vector3dAnimation { duration: 700; easing.type: Easing.InOutCubic } }
    Behavior on distance { enabled: root.animate && !root.interacting; NumberAnimation { duration: 600; easing.type: Easing.OutCubic } }

    function focusOn(key) {
        if (!scene) return
        var i = scene.indexOfKey(key)
        if (i < 0) return
        var d = deviceList[i]
        target = Qt.vector3d(d.x, 0, d.z)
        distance = Math.max(28, distance * 0.7)
    }

    View3D {
        id: view
        anchors.fill: parent
        camera: camera
        renderMode: View3D.Offscreen

        environment: SceneEnvironment {
            clearColor: Theme.dark ? "#070a10" : "#eef1f6"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
            aoStrength: 45
            aoDistance: 4
            aoSoftness: 40
        }

        Node {
            id: orbit
            position: root.target
            eulerRotation.y: root.yaw
            Node {
                eulerRotation.x: -root.pitch
                PerspectiveCamera {
                    id: camera
                    z: root.distance
                    fieldOfView: 38
                    clipNear: 1
                    clipFar: 2000
                }
            }
        }

        DirectionalLight {
            eulerRotation.x: -58
            eulerRotation.y: -35
            brightness: 1.15
            castsShadow: true
            shadowMapQuality: Light.ShadowMapQualityHigh
            shadowFactor: 45
            ambientColor: Theme.dark ? Qt.rgba(0.22, 0.25, 0.32, 1) : Qt.rgba(0.55, 0.58, 0.64, 1)
        }
        DirectionalLight {
            eulerRotation.x: -20
            eulerRotation.y: 150
            brightness: 0.35
            color: Theme.dark ? "#8fa8ff" : "#ffffff"
        }

        // Ground with a fine grid baked from a 2D Canvas.
        Model {
            source: "#Rectangle"
            eulerRotation.x: -90
            y: -0.01
            scale: Qt.vector3d(((root.scene ? root.scene.sceneWidth : 40) + 40) / 100,
                               ((root.scene ? root.scene.sceneDepth : 40) + 40) / 100, 1)
            receivesShadows: true
            materials: PrincipledMaterial {
                baseColor: Theme.dark ? "#0d121b" : "#dfe4ec"
                roughness: 0.92
                baseColorMap: Texture {
                    sourceItem: gridCanvas
                    scaleU: ((root.scene ? root.scene.sceneWidth : 40) + 40) / 8
                    scaleV: ((root.scene ? root.scene.sceneDepth : 40) + 40) / 8
                    tilingModeHorizontal: Texture.Repeat
                    tilingModeVertical: Texture.Repeat
                    generateMipmaps: true
                    mipFilter: Texture.Linear
                }
            }
        }

        // Production zones: low plates, tinted when any device in the zone
        // is critical.
        Repeater3D {
            model: root.zoneList
            delegate: Model {
                source: "#Cube"
                position: Qt.vector3d(modelData.x, 0.06, modelData.z)
                scale: Qt.vector3d((modelData.width + 1.2) / 100, 0.12 / 100, (modelData.depth + 1.2) / 100)
                receivesShadows: true
                materials: PrincipledMaterial {
                    baseColor: modelData.critical > 0
                               ? (Theme.dark ? "#2a1418" : "#f6d9d9")
                               : (Theme.dark ? "#141b28" : "#cfd7e3")
                    roughness: 0.7
                }
            }
        }

        // Footprint pads under every device.
        Model {
            source: "#Cylinder"
            instancing: PlantInstancing { scene: root.scene; mode: "pads"; highColor: Theme.accent; criticalColor: Theme.critical }
            materials: PrincipledMaterial {
                baseColor: "white"
                lighting: PrincipledMaterial.NoLighting
                alphaMode: PrincipledMaterial.Blend
            }
        }

        // Live pillars: one draw call for every device.
        Model {
            source: "#Cube"
            castsShadows: true
            receivesShadows: true
            instancing: PlantInstancing {
                scene: root.scene
                mode: "pillars"
                rise: root.rise
                selectedKey: root.selectedKey
                lowColor: Theme.dark ? "#2a3a5a" : "#9fb3d9"
                highColor: Theme.accent
                criticalColor: Theme.critical
                selectedColor: "#ffffff"
            }
            materials: PrincipledMaterial {
                baseColor: "white"
                metalness: 0.25
                roughness: 0.38
                emissiveFactor: Qt.vector3d(0.05, 0.06, 0.09)
            }
        }

        // Expanding alarm rings under critical devices (and a steady ring
        // under the selection).
        Model {
            source: "#Cylinder"
            instancing: PlantInstancing {
                scene: root.scene
                mode: "beacons"
                pulse: root.pulse
                selectedKey: root.selectedKey
                criticalColor: Theme.critical
                selectedColor: Theme.accent
            }
            materials: PrincipledMaterial {
                baseColor: "white"
                lighting: PrincipledMaterial.NoLighting
                alphaMode: PrincipledMaterial.Blend
            }
        }
    }

    Canvas {
        id: gridCanvas
        width: 256; height: 256
        visible: false
        property bool dark: Theme.dark
        onDarkChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d")
            ctx.fillStyle = Theme.dark ? "#ffffff" : "#ffffff"
            ctx.fillRect(0, 0, width, height)
            ctx.fillStyle = Theme.dark ? "#c8d2e6" : "#b7c1d1"
            ctx.fillRect(0, 0, width, 3)
            ctx.fillRect(0, 0, 3, height)
            ctx.fillStyle = Theme.dark ? "#e2e7f1" : "#d5dbe5"
            ctx.fillRect(0, height / 2, width, 1)
            ctx.fillRect(width / 2, 0, 1, height)
        }
    }

    // —— Projected labels ——
    // Re-evaluated whenever the camera moves.
    readonly property var _cam: [yaw, pitch, distance, target, width, height]

    Repeater {
        model: root.zoneList
        delegate: Item {
            readonly property vector3d p: { root._cam; return view.mapFrom3DScene(Qt.vector3d(modelData.x, 0.2, modelData.z + modelData.depth / 2 + 1.2)) }
            x: p.x - width / 2
            y: p.y
            width: zoneLabel.implicitWidth + 16
            height: 22
            visible: p.z > 0
            Rectangle {
                anchors.fill: parent
                radius: 11
                color: Theme.alpha(Theme.bgElevated, 0.85)
                border.width: 1
                border.color: modelData.critical > 0 ? Theme.alpha(Theme.critical, 0.6) : Theme.border
            }
            Text {
                id: zoneLabel
                anchors.centerIn: parent
                text: modelData.plant + " · " + modelData.floor + (modelData.critical > 0 ? "  ⚠ " + modelData.critical : "")
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.bold: true
                color: modelData.critical > 0 ? Theme.critical : Theme.textSecondary
            }
        }
    }

    // Selected device marker label.
    Item {
        id: selLabel
        readonly property int index: root.scene && root.selectedKey.length > 0 ? root.scene.indexOfKey(root.selectedKey) : -1
        readonly property var dev: index >= 0 && index < root.deviceList.length ? root.deviceList[index] : null
        readonly property vector3d p: { root._cam; root.rise; return dev ? view.mapFrom3DScene(Qt.vector3d(dev.x, dev.height * 7 * root.rise + 0.8, dev.z)) : Qt.vector3d(0, 0, -1) }
        visible: dev !== null && p.z > 0
        x: p.x - width / 2
        y: p.y - height - 6
        width: selText.implicitWidth + 18
        height: 24
        Rectangle { anchors.fill: parent; radius: 12; color: Theme.accent }
        Text {
            id: selText
            anchors.centerIn: parent
            text: selLabel.dev ? selLabel.dev.deviceId + " · " + selLabel.dev.value.toFixed(1) : ""
            font.family: Theme.fontFamilyMono
            font.pixelSize: 11
            font.bold: true
            color: "#ffffff"
        }
    }

    // —— Interaction ——
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        cursorShape: pressed && moved ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        property point last
        property bool moved: false
        onPressed: function(m) { last = Qt.point(m.x, m.y); moved = false; root.interacting = true }
        onReleased: function(m) {
            root.interacting = false
            if (!moved)
                root.pickAt(m.x, m.y)
        }
        onPositionChanged: function(m) {
            if (!pressed) return
            var dx = m.x - last.x, dy = m.y - last.y
            if (Math.abs(dx) + Math.abs(dy) > 2) moved = true
            root.yaw -= dx * 0.3
            root.pitch = Math.max(12, Math.min(84, root.pitch + dy * 0.25))
            last = Qt.point(m.x, m.y)
        }
        onWheel: function(w) {
            root.interacting = true
            root.distance = Math.max(14, Math.min(400, root.distance * (w.angleDelta.y > 0 ? 0.88 : 1.14)))
            idle.restart()
        }
        Timer { id: idle; interval: 400; onTriggered: root.interacting = false }
    }

    // Ray-cast from the cursor against every pillar's bounding box (600
    // slab tests is trivial per click), falling back to the ground plane.
    function pickAt(sx, sy) {
        var a = view.mapTo3DScene(Qt.vector3d(sx, sy, 0))
        var b = view.mapTo3DScene(Qt.vector3d(sx, sy, 1000))
        var dir = b.minus(a).normalized()
        var best = -1, bestT = 1e9
        var hw = 0.7
        for (var i = 0; i < deviceList.length; i++) {
            var d = deviceList[i]
            var h = d.height * 7 * rise
            var t = rayBox(a, dir, d.x - hw, 0, d.z - hw, d.x + hw, h, d.z + hw)
            if (t >= 0 && t < bestT) { bestT = t; best = i }
        }
        if (best < 0 && Math.abs(dir.y) > 1e-4) {
            var tg = -a.y / dir.y
            if (tg > 0)
                best = scene.pick(a.x + dir.x * tg, a.z + dir.z * tg, 1.4)
        }
        if (best >= 0)
            root.picked(deviceList[best].key)
    }

    function rayBox(o, d, x0, y0, z0, x1, y1, z1) {
        var tmin = -1e9, tmax = 1e9
        var os = [o.x, o.y, o.z], ds = [d.x, d.y, d.z], lo = [x0, y0, z0], hi = [x1, y1, z1]
        for (var k = 0; k < 3; k++) {
            if (Math.abs(ds[k]) < 1e-9) {
                if (os[k] < lo[k] || os[k] > hi[k]) return -1
            } else {
                var t1 = (lo[k] - os[k]) / ds[k], t2 = (hi[k] - os[k]) / ds[k]
                if (t1 > t2) { var tt = t1; t1 = t2; t2 = tt }
                tmin = Math.max(tmin, t1)
                tmax = Math.min(tmax, t2)
                if (tmin > tmax) return -1
            }
        }
        return tmax < 0 ? -1 : Math.max(tmin, 0)
    }
}
