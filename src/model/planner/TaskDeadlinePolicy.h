#pragma once

#include "domain/Task.h"

#include <QDateTime>

namespace smartmate::model {

/// 统一计算任务是否逾期；结果是随当前时间变化的派生状态，不得持久化。
class TaskDeadlinePolicy final {
public:
    /// 仅待办或进行中任务在截止时间早于 `nowUtc` 时逾期；相等时不逾期。
    [[nodiscard]] static bool isOverdue(const Task &task,
                                        const QDateTime &nowUtc) noexcept;
};

} // namespace smartmate::model
