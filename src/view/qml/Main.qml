import QtQuick
import QtQuick.Controls
import SmartMate.ViewModel 1.0

ApplicationWindow {
    id: root

    required property AppViewModel appViewModel

    width: 960
    height: 640
    visible: true
    title: appViewModel.applicationName
    color: "#f5f7fb"

    Column {
        anchors.centerIn: parent
        spacing: 12

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.appViewModel.applicationName
            color: "#172033"
            font.pixelSize: 32
            font.bold: true
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Strict MVVM scaffold is ready")
            color: "#667085"
            font.pixelSize: 16
        }
    }
}
