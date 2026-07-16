#include "TaskListViewModel.h"

#include "TaskErrorMapper.h"
#include "services/TaskService.h"

namespace smartmate::viewmodel {

bool TaskListViewModel::startTask(const QString &taskId)
{
    return performTransition(taskId, model::TaskTransition::Start);
}

bool TaskListViewModel::cancelTask(const QString &taskId)
{
    return performTransition(taskId, model::TaskTransition::Cancel);
}

bool TaskListViewModel::completeTask(const QString &taskId)
{
    return performTransition(taskId, model::TaskTransition::Complete);
}

bool TaskListViewModel::redoTask(const QString &taskId)
{
    return performTransition(taskId, model::TaskTransition::Redo);
}

bool TaskListViewModel::archiveTask(const QString &taskId)
{
    return performTransition(taskId, model::TaskTransition::Archive);
}

bool TaskListViewModel::restoreTask(const QString &taskId)
{
    return performTransition(taskId, model::TaskTransition::Restore);
}

bool TaskListViewModel::deleteArchivedTask(const QString &taskId)
{
    const model::TaskId id = parseTaskId(taskId);
    if (id.isNull()) {
        setError(taskErrorMessage(model::TaskError::NotFound));
        return false;
    }

    const model::TaskResult result = m_taskService.deleteArchivedTask(id);
    if (!result.ok()) {
        setError(taskErrorMessage(result.error));
        return false;
    }
    setError({});
    return true;
}

void TaskListViewModel::clearError()
{
    setError({});
}

bool TaskListViewModel::performTransition(const QString &taskId,
                                          const model::TaskTransition transition)
{
    // Contract 暴露语义命令，ViewModel 只负责路由；状态机、依赖和单进行中约束
    // 必须由 Service 在执行时最终复核，不能仅信任列表中投影的命令资格。
    const model::TaskId id = parseTaskId(taskId);
    if (id.isNull()) {
        setError(taskErrorMessage(model::TaskError::NotFound));
        return false;
    }

    model::TaskResult result;
    switch (transition) {
    case model::TaskTransition::Start:
        result = m_taskService.startTask(id);
        break;
    case model::TaskTransition::Cancel:
        result = m_taskService.cancelTask(id);
        break;
    case model::TaskTransition::Complete:
        result = m_taskService.completeTask(id);
        break;
    case model::TaskTransition::Redo:
        result = m_taskService.redoTask(id);
        break;
    case model::TaskTransition::Archive:
        result = m_taskService.archiveTask(id);
        break;
    case model::TaskTransition::Restore:
        result = m_taskService.restoreTask(id);
        break;
    }

    if (!result.ok()) {
        setError(taskErrorMessage(result.error));
        return false;
    }
    setError({});
    return true;
}

} // namespace smartmate::viewmodel

