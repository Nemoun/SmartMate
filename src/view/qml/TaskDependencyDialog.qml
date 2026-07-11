pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SmartMate.ViewModel 1.0

// 对话框只编辑稳定 TaskId 的选择草稿；环检测、状态约束和持久化全部由 Model 完成。
Dialog {
    id: root

    required property TaskDependencyViewModel editor

    width: 600
    height: Math.min(560, parent ? parent.height - 40 : 560)
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    title: qsTr("管理前置任务")

    contentItem: ColumnLayout {
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: qsTr("为“%1”选择必须先完成的任务。所有前置任务完成后，当前任务才可开始。")
                  .arg(root.editor.taskTitle)
            color: "#475467"
            wrapMode: Text.Wrap
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("已选择 %1 项").arg(root.editor.selectedCount)
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: "#f9fafb"
            border.color: "#d0d5dd"

            ListView {
                id: dependencyCandidateList
                objectName: "dependencyCandidateList"

                anchors.fill: parent
                anchors.margins: 8
                clip: true
                spacing: 6
                model: root.editor

                delegate: Rectangle {
                    id: candidateDelegate

                    required property string taskId
                    required property string shortId
                    required property string title
                    required property string statusText
                    required property string priorityText
                    required property bool selected
                    required property bool archived
                    required property bool selectable

                    width: ListView.view.width
                    height: 48
                    radius: 6
                    color: candidateDelegate.selected ? "#eff8ff" : "#ffffff"
                    border.color: candidateDelegate.selected ? "#84caff" : "#eaecf0"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10

                        CheckBox {
                            objectName: "dependencyCandidateCheckBox_"
                                        + candidateDelegate.taskId
                            checked: candidateDelegate.selected
                            enabled: candidateDelegate.selectable
                            Accessible.name: candidateDelegate.title
                            onToggled: root.editor.setPredecessorSelected(
                                           candidateDelegate.taskId, checked)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true

                            Label {
                                Layout.fillWidth: true
                                text: candidateDelegate.title
                                color: candidateDelegate.selectable ? "#172033" : "#98a2b3"
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("ID %1").arg(candidateDelegate.shortId)
                                color: "#98a2b3"
                                font.pixelSize: 11
                            }
                        }

                        Label {
                            text: qsTr("%1优先级 · %2")
                                  .arg(candidateDelegate.priorityText)
                                  .arg(candidateDelegate.statusText)
                            color: candidateDelegate.archived ? "#98a2b3" : "#475467"
                            font.pixelSize: 13
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar { }
            }

            Label {
                anchors.centerIn: parent
                visible: root.editor.count === 0
                text: qsTr("没有可选择的前置任务")
                color: "#667085"
            }
        }

        Label {
            objectName: "dependencyErrorLabel"
            Layout.fillWidth: true
            visible: text.length > 0
            text: root.editor.errorMessage
            color: "#b42318"
            wrapMode: Text.Wrap
        }
    }

    footer: Rectangle {
        implicitHeight: 62
        color: "#ffffff"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            Item { Layout.fillWidth: true }

            Button {
                objectName: "cancelDependenciesButton"
                text: qsTr("取消")
                onClicked: {
                    root.editor.cancel()
                    root.close()
                }
            }

            Button {
                objectName: "saveDependenciesButton"
                text: qsTr("保存")
                enabled: root.editor.canSave
                onClicked: {
                    if (root.editor.save())
                        root.close()
                }
            }
        }
    }
}
