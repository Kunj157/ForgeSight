import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Window {
    id: root
    width: 1440
    height: 900
    visible: true
    title: "ForgeSight"
    color: Theme.bgApp
    minimumWidth: 1100
    minimumHeight: 680

    property int alarmCount: 0
    property int navIndex: 0
    property string statusHint: "Connecting…"

    // Staleness tracking: the raw WS socket can look "connected" for a few
    // seconds after the backend actually stops publishing, and conversely a
    // brief reconnect blip is not the same as truly having no fresh data.
    // "Offline" here means no message has arrived recently, which is a
    // better signal for the user than the bare socket state.
    property double lastMessageAtMs: 0
    property bool stale: false
    readonly property bool offline: !wsClient.connected || stale
    readonly property int staleThresholdMs: 10000
    property string lastUpdatedText: "No data yet"
    property real freshness: 0
    property bool everConnected: false

    // connecting | live | stale | offline — drives the header pill, the
    // sidebar connection card and the reconnect activity line.
    readonly property string linkState: !wsClient.connected
                                        ? (everConnected ? "offline" : "connecting")
                                        : stale ? "stale"
                                        : lastMessageAtMs === 0 ? "connecting" : "live"

    readonly property var pages: [
        { label: "Devices", icon: "grid", subtitle: "Live sensor readings across the plant" },
        { label: "Alarms", icon: "bell", subtitle: "Active and acknowledged alarm events" },
        { label: "History", icon: "chart", subtitle: "Historical trends and export" },
        { label: "Rules", icon: "gauge", subtitle: "Create and manage alarm thresholds" },
    ]

    function markFresh() { lastMessageAtMs = Date.now() }
    function toast(title, message, kind) { toastHost.show(title, message, kind) }

    Component.onCompleted: {
        Theme.dark = displayOptions.theme !== "light"
        Theme.reducedMotion = displayOptions.reducedMotion
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            root.stale = root.lastMessageAtMs > 0
                       && (Date.now() - root.lastMessageAtMs) > root.staleThresholdMs

            if (root.lastMessageAtMs === 0) {
                root.lastUpdatedText = "No data yet"
                root.freshness = 0
                return
            }
            var age = Date.now() - root.lastMessageAtMs
            root.freshness = Math.max(0, 1 - age / root.staleThresholdMs)
            var secs = Math.round(age / 1000)
            if (secs < 2)
                root.lastUpdatedText = "Updated just now"
            else if (secs < 60)
                root.lastUpdatedText = "Updated " + secs + "s ago"
            else
                root.lastUpdatedText = "Updated " + Math.round(secs / 60) + "m ago"
        }
    }

    Connections {
        target: alarmModel
        function onAlarmAdded() { alarmCount = alarmModel.unacknowledgedCount }
        function onAlarmAcknowledged() {
            alarmCount = alarmModel.unacknowledgedCount
            root.toast("Alarm acknowledged", alarmCount + " still unacknowledged", "ok")
        }
    }

    Connections {
        target: wsClient
        function onReadingReceived(json) {
            var obj = JSON.parse(json)
            deviceModel.updateDevice(
                obj.device_id, obj.sensor, obj.value,
                obj.unit, obj.timestamp, obj.anomaly
            )
            root.markFresh()
        }
        function onAlarmReceived(json) {
            var obj = JSON.parse(json)
            alarmModel.add_alarm(
                obj.id, obj.device_id, obj.sensor,
                obj.value, obj.severity, obj.message, obj.timestamp,
                !!obj.acknowledged
            )
            root.markFresh()
            if (obj.severity === "critical" && !obj.acknowledged)
                root.toast("Critical alarm", obj.device_id + " · " + obj.sensor, "critical")
        }
        function onConnectedChanged() {
            if (wsClient.connected) {
                statusHint = ""
                if (root.everConnected)
                    root.toast("Reconnected", "Live data resumed", "ok")
                root.everConnected = true
            } else {
                statusHint = "Disconnected — run scripts/dev-up.sh"
                if (root.everConnected)
                    root.toast("Connection lost", "Showing cached values — retrying", "warning")
            }
        }
    }

    Connections {
        target: apiClient
        function onBootstrapFinished(ok, error) {
            if (ok)
                root.markFresh()
        }
    }

    // Backdrop: an ambient accent wash in the top-left plus a faint dot grid
    // that fades out toward the bottom — keeps a dark control-room UI from
    // reading flat. Repainted only on resize or theme change.
    Canvas {
        id: backdrop
        anchors.fill: parent
        z: -1
        property bool dark: Theme.dark
        onDarkChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var g = ctx.createRadialGradient(width * 0.18, 0, 0, width * 0.18, 0, width * 0.6)
            g.addColorStop(0, Theme.alpha(Theme.accent, Theme.dark ? 0.10 : 0.07))
            g.addColorStop(1, Theme.alpha(Theme.accent, 0))
            ctx.fillStyle = g
            ctx.fillRect(0, 0, width, height)
            var step = 22
            for (var y = step; y < height; y += step) {
                ctx.fillStyle = Theme.alpha(Theme.textMuted, (Theme.dark ? 0.10 : 0.16) * (1 - y / height))
                for (var x = Theme.sidebarWidth + step; x < width; x += step)
                    ctx.fillRect(x, y, 1, 1)
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            Layout.preferredWidth: Theme.sidebarWidth
            Layout.fillHeight: true
            currentIndex: root.navIndex
            items: root.pages
            badges: [deviceModel.count, root.alarmCount, 0, ruleModel.count]
            tones: ["", root.alarmCount > 0 ? "critical" : "", "", ""]
            linkState: root.linkState
            lastUpdatedText: root.lastUpdatedText
            pendingSync: offlineCache.pendingAckCount
            endpoint: wsClient.url.toString()
            freshness: root.freshness
            onNavigate: function(index) { root.navIndex = index }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            TopBar {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.topBarHeight
                title: root.pages[root.navIndex].label
                subtitle: root.pages[root.navIndex].subtitle
                hint: root.statusHint
                deviceCount: deviceModel.count
                unacked: root.alarmCount
                linkState: root.linkState
                linkTooltip: root.offline ? root.lastUpdatedText + " — showing cached values"
                                          : root.lastUpdatedText
                onThemeToggleRequested: Theme.toggle()
            }

            // Lazy Loaders — ChartView in History is expensive; don't build
            // all tabs at startup or the window appears to hang with no logs.
            // The current page fades and rises in; the previous one fades out.
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                component PanelSlot: Loader {
                    property int page: 0
                    readonly property bool current: root.navIndex === page
                    anchors.fill: parent
                    opacity: current ? 1 : 0
                    visible: opacity > 0.01
                    Behavior on opacity { NumberAnimation { duration: Theme.motionMed; easing.type: Easing.OutCubic } }
                    transform: Translate {
                        y: current ? 0 : 12
                        Behavior on y { NumberAnimation { duration: Theme.motionSlow; easing.type: Easing.OutCubic } }
                    }
                }

                PanelSlot {
                    page: 0
                    active: current || item !== null
                    source: "DeviceTreePanel.qml"
                }
                PanelSlot {
                    page: 1
                    active: current || item !== null
                    source: "AlarmPanel.qml"
                }
                PanelSlot {
                    id: historySlot
                    page: 2
                    active: current
                    asynchronous: true
                    source: "HistoryPanel.qml"

                    Item {
                        anchors.fill: parent
                        z: 1
                        visible: historySlot.status === Loader.Loading
                        Column {
                            anchors.fill: parent
                            anchors.margins: Theme.spaceXl
                            spacing: Theme.spaceLg
                            Skeleton { width: parent.width; height: 56; radius: Theme.radiusLg }
                            Skeleton { width: parent.width; height: parent.height - 160; radius: Theme.radiusLg }
                        }
                        ShimmerText {
                            anchors.centerIn: parent
                            text: "Loading history view…"
                            font.pixelSize: Theme.fontMd
                        }
                    }
                }
                PanelSlot {
                    page: 3
                    active: current || item !== null
                    source: "RuleEditorPanel.qml"
                }
            }
        }
    }

    ToastHost {
        id: toastHost
        anchors.fill: parent
        z: 100
    }
}
