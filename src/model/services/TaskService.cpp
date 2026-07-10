#include "services/TaskService.h"

namespace smartmate::model {

TaskService::TaskService(ITaskRepository &repository)
    : m_repository(repository)
{
}

QList<Task> TaskService::listTasks() const
{
    return m_repository.findAll();
}

} // namespace smartmate::model
