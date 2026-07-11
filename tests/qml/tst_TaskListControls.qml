import QtQuick
import QtQuick.Controls
import QtTest
import SmartMate.View

TestCase {
    id: testCase

    name: "TaskListControls"
    when: windowShown
    property var subject: null
    property string alphaId
    property string betaId
    property string gammaId

    Component {
        id: taskPageComponent

        Main {
            width: 1000
            height: 760
            visible: true
            appViewModel: testAppViewModel
        }
    }

    function createTask(title, description, priorityIndex) {
        const editor = testAppViewModel.taskEditor
        editor.beginCreate()
        editor.title = title
        editor.description = description
        editor.priorityIndex = priorityIndex
        verify(editor.save(), editor.errorMessage)
        return editor.taskId
    }

    function clearDependencies(taskId) {
        const editor = testAppViewModel.taskDependencies
        if (!editor.beginEdit(taskId))
            return
        const taskIds = [alphaId, betaId, gammaId]
        for (let index = 0; index < taskIds.length; ++index) {
            if (taskIds[index] !== taskId)
                verify(editor.setPredecessorSelected(taskIds[index], false))
        }
        if (editor.dirty)
            verify(editor.save(), editor.errorMessage)
    }

    function saveSinglePredecessor(taskId, predecessorId) {
        const editor = testAppViewModel.taskDependencies
        verify(editor.beginEdit(taskId), editor.errorMessage)
        verify(editor.setPredecessorSelected(predecessorId, true))
        verify(editor.save(), editor.errorMessage)
    }

    function taskDelegate(taskId) {
        const listView = findChild(subject, "taskListView")
        if (listView === null)
            return null
        for (let row = 0; row < listView.count; ++row) {
            listView.positionViewAtIndex(row, ListView.Contain)
            const delegate = listView.itemAtIndex(row)
            if (delegate !== null && delegate.taskId === taskId)
                return delegate
        }
        return null
    }

    function dependencyCandidateDelegate(dialog, taskId) {
        const listView = findChild(dialog, "dependencyCandidateList")
        if (listView === null)
            return null
        for (let row = 0; row < listView.count; ++row) {
            listView.positionViewAtIndex(row, ListView.Contain)
            const delegate = listView.itemAtIndex(row)
            if (delegate !== null && delegate.taskId === taskId)
                return delegate
        }
        return null
    }

    function initTestCase() {
        verify(testAppViewModel !== null)
        alphaId = createTask("Alpha 架构设计", "Model planner 说明", 3)
        betaId = createTask("Beta 文档", "整理使用手册", 0)
        gammaId = createTask("Gamma 测试", "覆盖筛选条件", 2)

        subject = taskPageComponent.createObject(testCase)
        verify(subject !== null)
        tryCompare(subject, "visible", true)
    }

    function cleanupTestCase() {
        if (subject !== null) {
            subject.close()
            subject.destroy()
            subject = null
        }
    }

    function init() {
        testAppViewModel.taskList.showArchived = false
        testAppViewModel.taskList.clearFilters()
        compare(testAppViewModel.taskList.count, 3)
    }

    function cleanup() {
        // 每个依赖用例都恢复三项任务的空关系，避免共享内存数据库污染其他测试。
        clearDependencies(alphaId)
        clearDependencies(betaId)
        clearDependencies(gammaId)
    }

    function test_searchFieldForwardsEditedText() {
        const searchField = findChild(subject, "taskSearchField")
        verify(searchField !== null)
        tryCompare(searchField, "text", "")

        // 直接触发控件的用户编辑信号，验证 QML 只把当前文本转发给 ViewModel。
        searchField.text = "PLANNER"
        searchField.textEdited()

        tryCompare(testAppViewModel.taskList, "searchText", "PLANNER")
        compare(testAppViewModel.taskList.count, 1)
    }

    function test_priorityComboForwardsSelection() {
        const priorityCombo = findChild(subject, "priorityFilterComboBox")
        verify(priorityCombo !== null)

        priorityCombo.activated(4)

        tryCompare(testAppViewModel.taskList, "priorityFilterIndex", 4)
        compare(testAppViewModel.taskList.count, 1)
    }

    function test_clearButtonClearsBothConditions() {
        const clearButton = findChild(subject, "clearFiltersButton")
        verify(clearButton !== null)
        testAppViewModel.taskList.searchText = "架构"
        testAppViewModel.taskList.priorityFilterIndex = 4
        tryCompare(clearButton, "visible", true)

        clearButton.clicked()

        compare(testAppViewModel.taskList.searchText, "")
        compare(testAppViewModel.taskList.priorityFilterIndex, 0)
        compare(testAppViewModel.taskList.count, 3)
        tryCompare(clearButton, "visible", false)
    }

    function test_emptyStateDistinguishesFilteredResult() {
        const emptyState = findChild(subject, "taskEmptyStateLabel")
        verify(emptyState !== null)

        testAppViewModel.taskList.searchText = "不存在的任务"

        compare(testAppViewModel.taskList.count, 0)
        tryCompare(emptyState, "visible", true)
        verify(emptyState.text.indexOf("没有符合") >= 0)
    }

    function test_archivedTaskHidesEditEntryUntilRestored() {
        tryVerify(function() { return taskDelegate(alphaId) !== null })
        let editButton = findChild(taskDelegate(alphaId),
                                   "editTaskButton_" + alphaId)
        verify(editButton !== null)
        tryCompare(editButton, "visible", true)

        // Todo 不能直接归档；先严格经过 Todo → InProgress → Done。
        verify(testAppViewModel.taskList.startTask(alphaId),
               testAppViewModel.taskList.errorMessage)
        verify(testAppViewModel.taskList.completeTask(alphaId),
               testAppViewModel.taskList.errorMessage)
        verify(testAppViewModel.taskList.archiveTask(alphaId),
               testAppViewModel.taskList.errorMessage)
        testAppViewModel.taskList.showArchived = true
        tryCompare(testAppViewModel.taskList, "count", 1)
        tryVerify(function() { return taskDelegate(alphaId) !== null })
        editButton = findChild(taskDelegate(alphaId),
                               "editTaskButton_" + alphaId)
        const restoreButton = findChild(taskDelegate(alphaId),
                                        "restoreTaskButton_" + alphaId)
        verify(editButton !== null)
        verify(restoreButton !== null)
        tryCompare(editButton, "visible", false)
        tryCompare(restoreButton, "visible", true)

        verify(testAppViewModel.taskList.restoreTask(alphaId),
               testAppViewModel.taskList.errorMessage)
        testAppViewModel.taskList.showArchived = false
        tryCompare(testAppViewModel.taskList, "count", 3)
        tryVerify(function() { return taskDelegate(alphaId) !== null })
        editButton = findChild(taskDelegate(alphaId),
                               "editTaskButton_" + alphaId)
        verify(editButton !== null)
        tryCompare(editButton, "visible", true)

        // 恢复后的正常状态是 Done；重做回 Todo，避免污染后续依赖编辑用例。
        verify(testAppViewModel.taskList.redoTask(alphaId),
               testAppViewModel.taskList.errorMessage)
    }

    function test_editorShowsReadOnlyInitialStatusInsteadOfStatusSelector() {
        const newTaskButton = findChild(subject, "newTaskButton")
        verify(newTaskButton !== null)
        mouseClick(newTaskButton)

        const editorDialog = findChild(subject, "taskEditorDialog")
        verify(editorDialog !== null)
        tryCompare(editorDialog, "opened", true)
        const statusLabel = findChild(editorDialog, "taskCurrentStatusLabel")
        verify(statusLabel !== null)
        verify(statusLabel.text.indexOf("初始状态：待办") >= 0)
        compare(findChild(editorDialog, "taskStatusComboBox"), null)

        testAppViewModel.taskEditor.cancel()
        editorDialog.close()
    }

    function test_stateButtonsFollowProjectionAndCancellationRequiresConfirmation() {
        tryVerify(function() { return taskDelegate(gammaId) !== null })
        let delegate = taskDelegate(gammaId)
        let startButton = findChild(delegate, "startTaskButton_" + gammaId)
        let cancelButton = findChild(delegate, "cancelTaskButton_" + gammaId)
        let completeButton = findChild(delegate, "completeTaskButton_" + gammaId)
        let redoButton = findChild(delegate, "redoTaskButton_" + gammaId)
        let archiveButton = findChild(delegate, "archiveTaskButton_" + gammaId)
        verify(startButton !== null)
        verify(cancelButton !== null)
        verify(completeButton !== null)
        verify(redoButton !== null)
        verify(archiveButton !== null)
        tryCompare(startButton, "visible", true)
        tryCompare(cancelButton, "visible", true)
        tryCompare(completeButton, "visible", false)
        tryCompare(redoButton, "visible", false)
        tryCompare(archiveButton, "visible", false)

        mouseClick(startButton)
        tryVerify(function() {
            return taskDelegate(gammaId) !== null
                   && taskDelegate(gammaId).statusText === "进行中"
        })
        delegate = taskDelegate(gammaId)
        completeButton = findChild(delegate, "completeTaskButton_" + gammaId)
        cancelButton = findChild(delegate, "cancelTaskButton_" + gammaId)
        tryCompare(completeButton, "visible", true)
        tryCompare(cancelButton, "visible", true)

        mouseClick(completeButton)
        tryVerify(function() {
            return taskDelegate(gammaId) !== null
                   && taskDelegate(gammaId).statusText === "已完成"
        })
        delegate = taskDelegate(gammaId)
        redoButton = findChild(delegate, "redoTaskButton_" + gammaId)
        archiveButton = findChild(delegate, "archiveTaskButton_" + gammaId)
        tryCompare(redoButton, "visible", true)
        tryCompare(archiveButton, "visible", true)
        mouseClick(redoButton)
        tryVerify(function() {
            return taskDelegate(gammaId) !== null
                   && taskDelegate(gammaId).statusText === "待办"
        })

        delegate = taskDelegate(gammaId)
        cancelButton = findChild(delegate, "cancelTaskButton_" + gammaId)
        mouseClick(cancelButton)
        const cancelDialog = findChild(subject, "cancelTaskDialog")
        verify(cancelDialog !== null)
        tryCompare(cancelDialog, "opened", true)
        cancelDialog.reject()
        tryCompare(cancelDialog, "opened", false)
        compare(taskDelegate(gammaId).statusText, "待办")

        cancelButton = findChild(taskDelegate(gammaId),
                                 "cancelTaskButton_" + gammaId)
        mouseClick(cancelButton)
        tryCompare(cancelDialog, "opened", true)
        cancelDialog.accept()
        tryVerify(function() {
            return taskDelegate(gammaId) !== null
                   && taskDelegate(gammaId).statusText === "已取消"
        })
        delegate = taskDelegate(gammaId)
        redoButton = findChild(delegate, "redoTaskButton_" + gammaId)
        archiveButton = findChild(delegate, "archiveTaskButton_" + gammaId)
        tryCompare(redoButton, "visible", true)
        tryCompare(archiveButton, "visible", true)

        // 返回 Todo，保证共享内存数据库中的后续用例仍可编辑依赖。
        mouseClick(redoButton)
        tryVerify(function() {
            return taskDelegate(gammaId) !== null
                   && taskDelegate(gammaId).statusText === "待办"
        })
    }

    function test_dependencyDialogShowsBlockerAndUnlockProjection() {
        const listView = findChild(subject, "taskListView")
        verify(listView !== null)
        compare(listView.count, 3)
        verify(listView.height > 0, "taskListView height=" + listView.height)
        listView.positionViewAtBeginning()
        wait(0)
        tryVerify(function() { return taskDelegate(alphaId) !== null })
        const alphaDelegate = taskDelegate(alphaId)
        const editButton = findChild(alphaDelegate,
                                     "editDependenciesButton_" + alphaId)
        verify(editButton !== null)
        mouseClick(editButton)
        wait(0)

        const dialog = findChild(subject, "taskDependencyDialog")
        verify(dialog !== null)
        tryVerify(function() {
            return dependencyCandidateDelegate(dialog, betaId) !== null
        })
        const betaCheckBox = findChild(
                    dependencyCandidateDelegate(dialog, betaId),
                    "dependencyCandidateCheckBox_" + betaId)
        const saveButton = findChild(dialog, "saveDependenciesButton")
        verify(betaCheckBox !== null)
        verify(saveButton !== null)
        mouseClick(betaCheckBox)
        tryCompare(testAppViewModel.taskDependencies, "selectedCount", 1)
        mouseClick(saveButton)

        tryVerify(function() {
            return taskDelegate(alphaId) !== null && taskDelegate(betaId) !== null
        })
        const blockingLabel = findChild(taskDelegate(alphaId),
                                        "blockingReasonLabel_" + alphaId)
        const unlockLabel = findChild(taskDelegate(betaId),
                                      "unlockCountLabel_" + betaId)
        verify(blockingLabel !== null)
        verify(unlockLabel !== null)
        tryCompare(blockingLabel, "visible", true)
        verify(blockingLabel.text.indexOf("Beta 文档") >= 0)
        tryCompare(unlockLabel, "visible", true)
        verify(unlockLabel.text.indexOf("1") >= 0)
    }

    function test_dependencyDialogShowsCompleteCyclePathAndCancelsDraft() {
        // Beta → Alpha、Gamma → Beta 已存在时，再添加 Alpha → Gamma 会形成完整三节点环。
        saveSinglePredecessor(alphaId, betaId)
        saveSinglePredecessor(betaId, gammaId)

        tryVerify(function() { return taskDelegate(gammaId) !== null })
        const editButton = findChild(taskDelegate(gammaId),
                                     "editDependenciesButton_" + gammaId)
        verify(editButton !== null)
        mouseClick(editButton)
        wait(0)

        const dialog = findChild(subject, "taskDependencyDialog")
        verify(dialog !== null)
        tryVerify(function() {
            return dependencyCandidateDelegate(dialog, alphaId) !== null
        })
        const alphaCheckBox = findChild(
                    dependencyCandidateDelegate(dialog, alphaId),
                    "dependencyCandidateCheckBox_" + alphaId)
        const saveButton = findChild(dialog, "saveDependenciesButton")
        const cancelButton = findChild(dialog, "cancelDependenciesButton")
        const errorLabel = findChild(dialog, "dependencyErrorLabel")
        verify(alphaCheckBox !== null)
        verify(saveButton !== null)
        verify(cancelButton !== null)
        verify(errorLabel !== null)

        mouseClick(alphaCheckBox)
        mouseClick(saveButton)
        tryVerify(function() { return errorLabel.text.length > 0 })
        verify(errorLabel.text.indexOf("Alpha 架构设计") >= 0)
        verify(errorLabel.text.indexOf("Beta 文档") >= 0)
        verify(errorLabel.text.indexOf("Gamma 测试") >= 0)
        verify(errorLabel.text.indexOf("→") >= 0)

        mouseClick(cancelButton)
        verify(!testAppViewModel.taskDependencies.dirty)
    }
}
