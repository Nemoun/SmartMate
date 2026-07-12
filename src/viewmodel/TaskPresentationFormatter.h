#pragma once

#include "domain/Task.h"

#include <QString>

namespace smartmate::viewmodel {

/// 将领域枚举和值对象映射为统一中文文案，不包含任何业务资格判断。
[[nodiscard]] QString taskStatusText(model::TaskStatus status);
[[nodiscard]] QString taskPriorityText(model::TaskPriority priority);
[[nodiscard]] QString taskDeadlineText(const model::Task &task,
                                       const QString &emptyText);
[[nodiscard]] QString taskDurationText(const model::Task &task,
                                       const QString &emptyText);

} // namespace smartmate::viewmodel
