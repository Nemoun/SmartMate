#pragma once

#include "domain/Task.h"

#include <QString>

namespace smartmate::viewmodel {

/// 将领域枚举和值对象映射为统一中文文案，不包含任何业务资格判断。
[[nodiscard]] QString taskStatusText(model::TaskStatus status);
[[nodiscard]] QString taskPriorityText(model::TaskPriority priority);
/// 将 UTC 截止时间转换为本地时间文本；未设置时返回调用方提供的占位文案。
[[nodiscard]] QString taskDeadlineText(const model::Task &task,
                                       const QString &emptyText);
/// 将总分钟拆分为天/小时/分钟文案；未设置时返回占位文案。
[[nodiscard]] QString taskDurationText(const model::Task &task,
                                       const QString &emptyText);

} // namespace smartmate::viewmodel
