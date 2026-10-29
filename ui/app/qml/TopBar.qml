import QtQuick
import QtQuick.Layouts

// Header: animated page title, KPI chips (sliding counters, pulsing
// unacknowledged-alarm pill, link state) and the theme toggle. While the
// link is down an indeterminate line runs along the bottom edge.
Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property string hint: ""            // overrides subtitle, shown in warning colour
    property int deviceCount: 0
    property int unacked: 0
    property string linkState: "connecting"
    property string linkTooltip: ""
    signal themeToggleRequested()

    color: Theme.bgTopBar

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }

    // Title transition: slide the old title out and the new one in.
    property string _shownTitle: title
    onTitleChanged: titleSwap.restart()
    SequentialAnimation {
        id: titleSwap
        ParallelAnimation {
            NumberAnimation { target: titleCol; property: "opacity"; to: 0; duration: Theme.motionFast }
            NumberAnimation { target: titleCol; property: "y"; to: -6; duration: Theme.motionFast }
        }
        ScriptAction { script: { root._shownTitle = root.title; titleCol.y = 8 } }
        ParallelAnimation {
            NumberAnimation { target: titleCol; property: "opacity"; to: 1; duration: Theme.motionMed }
            NumberAnimation { target: titleCol; property: "y"; to: 0; duration: Theme.motionMed; easing.type: Easing.OutCubic }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceXl
        anchors.rightMargin: Theme.spaceXl
        spacing: Theme.spaceMd

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Column {
                id: titleCol
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                Text {
                    text: root._shownTitle
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontXl
                    font.bold: true
                    color: Theme.textPrimary
                }
                Text {
                    text: root.hint.length > 0 ? root.hint : root.subtitle
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSm
                    color: root.hint.length > 0 ? Theme.warning : Theme.textMuted
                }
            }
        }

        // Sensors KPI
        Rectangle {
            implicitHeight: 32
            implicitWidth: devRow.implicitWidth + 24
            radius: 16
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.border
            Row {
                id: devRow
                anchors.centerIn: parent
                spacing: 7
                Icon { name: "grid"; width: 12; height: 12; color: Theme.textSecondary; anchors.verticalCenter: parent.verticalCenter }
                SlidingNumber {
                    value: root.deviceCount
                    font.pixelSize: Theme.fontSm
                    font.bold: true
                    color: Theme.textPrimary
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "sensors"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontXs
                    color: Theme.textSecondary
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // Unacknowledged alarms KPI — pulses while anything is unacked.
        StatusPill {
            implicitHeight: 32
            status: root.unacked > 0 ? "critical" : "ok"
            pulse: root.unacked > 0
            text: root.unacked > 0 ? root.unacked + " unacked" : "all clear"
        }

        StatusPill {
            id: linkPill
            implicitHeight: 32
            status: root.linkState === "live" ? "ok"
                  : root.linkState === "stale" ? "stale"
                  : root.linkState === "offline" ? "critical" : "info"
            pulse: root.linkState === "live"
            text: root.linkState === "connecting" ? "connecting" : root.linkState

            MouseArea {
                id: linkMouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
            }
            Rectangle {
                visible: linkMouse.containsMouse && root.linkTooltip.length > 0
                anchors.top: parent.bottom
                anchors.topMargin: 6
                anchors.right: parent.right
                width: tip.implicitWidth + 16
                height: tip.implicitHeight + 10
                radius: Theme.radiusSm
                color: Theme.bgTooltip
                z: 10
                Text {
                    id: tip
                    anchors.centerIn: parent
                    text: root.linkTooltip
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontXs
                    color: "#f4f6fa"
                }
            }
        }

        // Theme toggle
        Rectangle {
            implicitWidth: 32
            implicitHeight: 32
            radius: 16
            color: themeMouse.containsMouse ? Theme.bgCardHover : "transparent"
            border.width: 1
            border.color: Theme.border
            Icon {
                anchors.centerIn: parent
                width: 15; height: 15
                name: Theme.dark ? "sun" : "moon"
                color: Theme.textSecondary
                rotation: Theme.dark ? 0 : 360
                Behavior on rotation { NumberAnimation { duration: Theme.motionSlow; easing.type: Easing.OutBack } }
            }
            MouseArea {
                id: themeMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.themeToggleRequested()
            }
        }
    }

    // Indeterminate activity line while (re)connecting.
    Item {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 2
        clip: true
        visible: root.linkState === "connecting" || root.linkState === "offline"
        Rectangle {
            id: runner
            width: parent.width * 0.3
            height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: root.linkState === "offline" ? Theme.critical : Theme.accent }
                GradientStop { position: 1.0; color: "transparent" }
            }
            NumberAnimation on x {
                from: -runner.width; to: root.width
                duration: 1400
                loops: Animation.Infinite
                running: runner.parent.visible && !Theme.reducedMotion
            }
        }
    }
}
