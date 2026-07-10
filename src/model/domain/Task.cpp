#include "domain/Task.h"

#include <utility>

namespace smartmate::model {

Task::Task()
    : m_id(QUuid::createUuid())
{
}

Task::Task(TaskId id, QString title, TaskStatus status)
    : m_id(std::move(id))
    , m_title(std::move(title))
    , m_status(status)
{
}

const TaskId &Task::id() const noexcept
{
    return m_id;
}

const QString &Task::title() const noexcept
{
    return m_title;
}

TaskStatus Task::status() const noexcept
{
    return m_status;
}

} // namespace smartmate::model
