import QtQuick
import QtQuick.Layouts

// Live-link status for the sidebar footer: a pulsing state dot, the data
// age, queued offline acks and the WebSocket endpoint. A thin meter drains
// as the last message ages toward the stale threshold.
GlowCard {
    id: root

    property string linkState: "connecting"   // connecting | live | stale | offline
    property string lastUpdatedText: ""
    property int pendingSync: 0
    property string endpoint: ""
    property real freshness: 1.0              // 1 = just updated, 0 = stale

    readonly property string tone: linkState === "live" ? "ok"
                                 : linkState === "stale" ? "stale"
                                 : linkState === "offline" ? "critical" : "info"
    readonly property color toneColor: Theme.statusColor(tone)

    implicitHeight: col.implicitHeight + 2 * Theme.spaceMd
    elevation: 0
    interactive: false
    color: Theme.bgCard
    colorTop: Theme.bgCard
    borderColor: Theme.alpha(toneColor, linkState === "live" ? 0.22 : 0.4)

    ColumnLayout {
        id: col
        anchors.fill: parent
        anchors.margins: Theme.spaceMd
        spacing: 4

        RowLayout {
            spacing: Theme.spaceSm

            Item {
                width: 10; height: 10
                Rectangle {
                    id: ripple
                    anchors.centerIn: parent
                    width: 10; height: 10; radius: 5
                    color: root.toneColor
                    opacity: 0
                    visible: (root.linkState === "live" || root.linkState === "connecting")
                             && !Theme.reducedMotion
                    ParallelAnimation {
                        running: ripple.visible
                        loops: Animation.Infinite
                        NumberAnimation { target: ripple; property: "scale"; from: 1; to: 2.6; duration: 1500; easing.type: Easing.OutCubic }
                        NumberAnimation { target: ripple; property: "opacity"; from: 0.55; to: 0; duration: 1500; easing.type: Easing.OutCubic }
                    }
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: 8; height: 8; radius: 4
                    color: root.toneColor
                }
            }

            ShimmerText {
                Layout.fillWidth: true
                text: root.linkState === "live" ? "Live"
                    : root.linkState === "stale" ? "Stale data"
                    : root.linkState === "offline" ? "Offline" : "Connecting…"
                running: root.linkState === "connecting"
                color: root.linkState === "connecting" ? Theme.textSecondary : Theme.textPrimary
                font.pixelSize: Theme.fontSm
                font.bold: true
            }

            StatusPill {
                visible: root.pendingSync > 0
                status: "warning"
                text: root.pendingSync + " queued"
                implicitHeight: 18
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.linkState === "offline" ? root.lastUpdatedText + " · cached values"
                                              : root.lastUpdatedText
            elide: Text.ElideRight
            font.family: Theme.fontFamily
            font.pixelSize: 10
            color: root.linkState === "stale" ? Theme.stale : Theme.textMuted
        }

        // Freshness meter.
        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 4
            height: 3
            radius: 1.5
            color: Theme.bgElevated
            Rectangle {
                height: parent.height
                radius: parent.radius
                width: parent.width * Math.max(0, Math.min(1, root.freshness))
                color: root.toneColor
                Behavior on width { NumberAnimation { duration: 950 } }
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 2
            text: root.endpoint
            elide: Text.ElideMiddle
            font.family: Theme.fontFamilyMono
            font.pixelSize: 10
            color: Theme.textMuted
        }
    }
}
