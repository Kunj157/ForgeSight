import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Window {
    width: 1280
    height: 720
    visible: true
    title: "ForgeSight"

    property int alarmCount: alarmModel.unacknowledgedCount

    Connections {
        target: wsClient
        function onReadingReceived(json) {
            var obj = JSON.parse(json)
            deviceModel.updateDevice(
                obj.device_id, obj.sensor, obj.value,
                obj.unit, obj.timestamp, obj.anomaly
            )
        }
        function onAlarmReceived(json) {
            var obj = JSON.parse(json)
            alarmModel.addAlarm(
                obj.id, obj.device_id, obj.sensor,
                obj.value, obj.severity, obj.message, obj.timestamp
            )
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                Label {
                    text: "ForgeSight"
                    font.bold: true
                    font.pixelSize: 18
                    Layout.fillWidth: true
                }
                Label {
                    text: wsClient.connected ? "Connected" : "Disconnected"
                    color: wsClient.connected ? "green" : "red"
                }
                Label {
                    text: alarmCount > 0 ? alarmCount + " alarms" : ""
                    color: "red"
                    visible: alarmCount > 0
                }
            }
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            TabButton { text: "Devices" }
            TabButton { text: "Alarms" }
        }

        StackLayout {
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            DeviceTreePanel {}
            AlarmPanel {}
        }
    }
}
