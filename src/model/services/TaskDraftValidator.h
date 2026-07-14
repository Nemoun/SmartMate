#pragma once

#include "domain/Task.h"
#include "services/TaskResult.h"

namespace smartmate::model {

/// 纯业务校验，不访问 Repository；所有写入和编辑草稿预览必须复用。
[[nodiscard]] TaskValidationResult validateTaskDraft(const TaskDraft &draft);
/// 校验预计用时的分钟数边界，供类型化输入即时反馈和最终保存共同复用。
[[nodiscard]] TaskValidationResult validateTaskEstimatedMinutes(int minutes);

} // namespace smartmate::model
