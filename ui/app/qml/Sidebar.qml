import QtQuick
import QtQuick.Layouts

// Left navigation rail: brand mark, nav items with a sliding active pill
// (Motion Primitives' Animated Background), live count badges, and the
// connection card at the bottom.
Rectangle {
    id: root

    property int currentIndex: 0
    property var items: []          // static [{ label, icon }]
    property var badges: []         // live counts, by index (0 hides the badge)
    property var tones: []          // status tone per badge, "" for neutral
    property string linkState: "connecting"
    property string lastUpdatedText: ""
    property int pendingSync: 0
    property string endpoint: ""
    property real freshness: 1.0
    signal navigate(int index)

    gradient: Gradient {
        GradientStop { position: 0.0; color: Theme.bgSidebar }
        GradientStop { position: 1.0; color: Theme.bgSidebarBottom }
    }

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.border
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // —— Brand ——
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.topBarHeight
            Layout.leftMargin: Theme.spaceLg
            Layout.rightMargin: Theme.spaceLg
            spacing: Theme.spaceMd

            Item {
                width: 34; height: 34
                GlowEffect { anchors.fill: parent; radius: Theme.radiusMd; color: Theme.accent2; intensity: 0.7 }
                Rectangle {
                    anchors.fill: parent
                    radius: Theme.radiusMd
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Theme.accent }
                        GradientStop { position: 1.0; color: Theme.accent2 }
                    }
                    Icon { anchors.centerIn: parent; width: 18; height: 18; name: "layers"; color: "#ffffff"; strokeWidth: 1.7 }
                }
            }

            ColumnLayout {
                spacing: 0
                Layout.fillWidth: true
                Text {
                    text: "ForgeSight"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontLg
                    font.bold: true
                    color: Theme.textPrimary
                }
                Text {
                    text: "Plant monitor"
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontXs
                    color: Theme.textMuted
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.divider }

        Text {
            Layout.leftMargin: Theme.spaceLg + 4
            Layout.topMargin: Theme.spaceXl
            Layout.bottomMargin: Theme.spaceSm
            text: "MONITORING"
            font.family: Theme.fontFamily
            font.pixelSize: 10
            font.bold: true
            font.letterSpacing: Theme.trackingCaps
            color: Theme.textMuted
        }

        // —— Nav ——
        Item {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spaceSm
            Layout.rightMargin: Theme.spaceSm
            implicitHeight: nav.implicitHeight

            // Sliding active pill.
            Rectangle {
                id: pill
                readonly property Item target: navRep.count > root.currentIndex
                                               ? navRep.itemAt(root.currentIndex) : null
                visible: target !== null
                width: parent.width
                height: 40
                y: target ? target.y : 0
                radius: Theme.radiusMd
                color: Theme.accentSoft
                border.width: 1
                border.color: Theme.alpha(Theme.accent, 0.28)
                Behavior on y { enabled: !Theme.reducedMotion; SpringAnimation { spring: 4.5; damping: 0.38; epsilon: 0.3 } }

                Rectangle {
                    x: -1
                    anchors.verticalCenter: parent.verticalCenter
                    width: 3; height: 18; radius: 1.5
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Theme.accent }
                        GradientStop { position: 1.0; color: Theme.accent2 }
                    }
                }
            }

            Column {
                id: nav
                width: parent.width
                spacing: 2

                Repeater {
                    id: navRep
                    model: root.items
                    delegate: Item {
                        readonly property bool active: index === root.currentIndex
                        readonly property int badge: root.badges[index] || 0
                        readonly property string tone: root.tones[index] || ""
                        width: nav.width
                        height: 40

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.radiusMd
                            color: Theme.bgCardHover
                            opacity: navMouse.containsMouse && !parent.active ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spaceLg
                            anchors.rightMargin: Theme.spaceMd
                            spacing: Theme.spaceMd

                            Icon {
                                name: modelData.icon
                                width: 16; height: 16
                                color: active ? Theme.accent : Theme.textMuted
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.label
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontMd
                                font.bold: active
                                color: active ? Theme.textPrimary : Theme.textSecondary
                            }
                            Rectangle {
                                visible: badge > 0
                                implicitWidth: Math.max(22, badgeNum.implicitWidth + 12)
                                implicitHeight: 20
                                radius: 10
                                color: tone ? Theme.statusBg(tone) : Theme.bgElevated
                                border.width: tone ? 1 : 0
                                border.color: tone ? Theme.alpha(Theme.statusColor(tone), 0.5) : "transparent"
                                SlidingNumber {
                                    id: badgeNum
                                    anchors.centerIn: parent
                                    value: badge
                                    font.pixelSize: 10
                                    font.bold: true
                                    font.family: Theme.fontFamily
                                    color: tone ? Theme.statusColor(tone) : Theme.textSecondary
                                }
                            }
                        }

                        MouseArea {
                            id: navMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.navigate(index)
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        ConnectionCard {
            Layout.fillWidth: true
            Layout.margins: Theme.spaceMd
            linkState: root.linkState
            lastUpdatedText: root.lastUpdatedText
            pendingSync: root.pendingSync
            endpoint: root.endpoint
            freshness: root.freshness
        }
    }
}
