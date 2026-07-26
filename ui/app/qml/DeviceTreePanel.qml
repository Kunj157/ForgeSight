import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    color: "#f5f5f5"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        Label {
            text: "Live Devices"
            font.bold: true
            font.pixelSize: 14
        }

        ListView {
            id: deviceList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: deviceModel

            delegate: Rectangle {
                width: deviceList.width
                height: 56
                radius: 6
                color: status === "critical" ? "#ffebee" :
                       status === "warning" ? "#fff8e1" : "#e8f5e9"
                border.color: status === "critical" ? "#f44336" :
                              status === "warning" ? "#ffc107" : "#4caf50"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 12

                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: status === "critical" ? "#f44336" :
                               status === "warning" ? "#ffc107" : "#4caf50"
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: deviceId
                            font.bold: true
                            font.pixelSize: 13
                        }
                        Label {
                            text: sensor + ": " + value + " " + unit
                            font.pixelSize: 11
                            color: "#666"
                        }
                    }

                    Label {
                        text: timestamp
                        font.pixelSize: 10
                        color: "#999"
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                text: "No devices connected"
                color: "#999"
                visible: deviceList.count === 0
            }
        }
    }
}
