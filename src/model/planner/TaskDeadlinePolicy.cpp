#include "planner/TaskDeadlinePolicy.h"

namespace smartmate::model {

bool TaskDeadlinePolicy::isOverdue(const Task &task,
                                   const QDateTime &nowUtc) noexcept
{
    if (task.status() != TaskStatus::Todo
        && task.status() != TaskStatus::InProgress) {
        return false;
    }
    if (!nowUtc.isValid() || !task.deadline().has_value()
        || !task.deadline()->isValid()) {
        return false;
    }

    // 严格小于保证截止时间恰好等于当前时刻时仍可按时完成。
    return *task.deadline() < nowUtc;
}

} // namespace smartmate::model
