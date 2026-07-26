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
            text: "Alarms"
            font.bold: true
            font.pixelSize: 14
        }

        ListView {
            id: alarmList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: alarmModel

            delegate: Rectangle {
                width: alarmList.width
                height: 64
                radius: 6
                color: acknowledged ? "#f5f5f5" :
                       severity === "critical" ? "#ffebee" :
                       severity === "warning" ? "#fff8e1" : "#e3f2fd"
                border.color: acknowledged ? "#ccc" :
                              severity === "critical" ? "#f44336" :
                              severity === "warning" ? "#ffc107" : "#2196f3"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 12

                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: acknowledged ? "#ccc" :
                               severity === "critical" ? "#f44336" :
                               severity === "warning" ? "#ffc107" : "#2196f3"
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: deviceId + " / " + sensor
                            font.bold: true
                            font.pixelSize: 13
                        }
                        Label {
                            text: message
                            font.pixelSize: 11
                            color: "#666"
                        }
                    }

                    Label {
                        text: value.toFixed(1)
                        font.pixelSize: 16
                        font.bold: true
                        color: severity === "critical" ? "#f44336" :
                               severity === "warning" ? "#ff8f00" : "#1976d2"
                    }

                    Button {
                        text: "ACK"
                        visible: !acknowledged
                        onClicked: alarmModel.acknowledge(id)
                        font.pixelSize: 10
                    }

                    Label {
                        text: "ACK'd"
                        visible: acknowledged
                        color: "#999"
                        font.pixelSize: 10
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                text: "No alarms"
                color: "#999"
                visible: alarmList.count === 0
            }
        }
    }
}
