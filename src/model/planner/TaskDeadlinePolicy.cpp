#include "planner/TaskDeadlinePolicy.h"

namespace smartmate::model {

bool TaskDeadlinePolicy::isOverdue(const Task &task,
                                   const QDateTime &nowUtc) noexcept
{
    // 终态和归档任务不再产生行动提醒，即使保留了过去的截止时间。
    if (task.status() != TaskStatus::Todo
        && task.status() != TaskStatus::InProgress) {
        return false;
    }
    // 无当前时间、无截止时间或损坏时间时不能可靠判定逾期。
    if (!nowUtc.isValid() || !task.deadline().has_value()
        || !task.deadline()->isValid()) {
        return false;
    }

    // 严格小于保证截止时间恰好等于当前时刻时仍可按时完成。
    return *task.deadline() < nowUtc;
}

} // namespace smartmate::model
