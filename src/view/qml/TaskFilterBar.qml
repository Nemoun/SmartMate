pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SmartMate.ViewModel 1.0

Rectangle {
    id: bar
    required property TaskListViewModel taskList
    required property var theme
    signal newTaskRequested()

    implicitHeight: toolbarLayout.implicitHeight + bar.theme.px(20)
    radius: 11
    color: bar.theme.surface
    border.color: bar.theme.borderSoft

    RowLayout {
        id: toolbarLayout
        anchors.fill: parent
        anchors.margins: bar.theme.px(10)
        spacing: bar.theme.px(8)
        Button {
            text: qsTr("活动任务")
            checkable: true
            checked: !bar.taskList.showArchived
            onClicked: bar.taskList.showArchived = false
        }
        Button {
            text: qsTr("归档")
            checkable: true
            checked: bar.taskList.showArchived
            onClicked: bar.taskList.showArchived = true
        }
        TextField {
            objectName: "taskSearchField"
            Layout.fillWidth: true
            Layout.minimumWidth: 180
            Layout.maximumWidth: 420
            text: bar.taskList.searchText
            placeholderText: qsTr("搜索任务标题或描述")
            selectByMouse: true
            onTextEdited: bar.taskList.searchText = text
        }
        ComboBox {
            objectName: "priorityFilterComboBox"
            Layout.preferredWidth: bar.theme.px(145)
            model: bar.taskList.priorityFilterOptions
            currentIndex: bar.taskList.priorityFilterIndex
            onActivated: index => bar.taskList.priorityFilterIndex = index
        }
        Button {
            objectName: "clearFiltersButton"
            text: qsTr("清除")
            visible: bar.taskList.hasActiveFilters
            onClicked: bar.taskList.clearFilters()
        }
        Item { Layout.fillWidth: true }
        Label {
            text: qsTr("%1 项").arg(bar.taskList.count)
            color: bar.theme.textMuted
        }
        Button {
            objectName: "newTaskButton"
            text: qsTr("＋ 新建任务")
            onClicked: bar.newTaskRequested()
        }
    }
}
