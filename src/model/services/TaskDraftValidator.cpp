#include "services/TaskDraftValidator.h"

#include "domain/TaskConstraints.h"

namespace smartmate::model {
namespace {

/// 文本长度上限属于 Model 业务规则，Widget 只能据此展示校验结果。
constexpr int maximumTitleLength = 200;
constexpr int maximumDescriptionLength = 5000;

[[nodiscard]] bool isValidPriority(const TaskPriority priority) noexcept
{
    switch (priority) {
    case TaskPriority::Low:
    case TaskPriority::Normal:
    case TaskPriority::High:
    case TaskPriority::Urgent:
        return true;
    }
    return false;
}

} // namespace

TaskValidationResult validateTaskEstimatedMinutes(const int minutes)
{
    if (minutes < TaskConstraints::minimumEstimatedMinutes
        || minutes > TaskConstraints::maximumEstimatedMinutes) {
        return TaskValidationResult::failure(
            TaskError::InvalidEstimate,
            QStringLiteral("Estimated minutes must be between 1 and 525600."));
    }
    return TaskValidationResult::success();
}

TaskValidationResult validateTaskDraft(const TaskDraft &draft)
{
    // 标题持久化前统一去除首尾空白；描述保留用户输入的原始排版。
    const QString normalizedTitle = draft.title.trimmed();
    if (normalizedTitle.isEmpty()) {
        return TaskValidationResult::failure(
            TaskError::EmptyTitle, QStringLiteral("Task title must not be empty."));
    }
    if (normalizedTitle.size() > maximumTitleLength) {
        return TaskValidationResult::failure(
            TaskError::TitleTooLong,
            QStringLiteral("Task title exceeds 200 characters."));
    }
    if (draft.description.size() > maximumDescriptionLength) {
        return TaskValidationResult::failure(
            TaskError::DescriptionTooLong,
            QStringLiteral("Task description exceeds 5000 characters."));
    }
    if (draft.deadline.has_value() && !draft.deadline->isValid()) {
        return TaskValidationResult::failure(
            TaskError::InvalidDeadline, QStringLiteral("Task deadline is invalid."));
    }
    if (draft.estimatedMinutes.has_value()) {
        const TaskValidationResult estimate =
            validateTaskEstimatedMinutes(*draft.estimatedMinutes);
        if (!estimate.ok()) {
            return estimate;
        }
    }
    if (!isValidPriority(draft.priority)) {
        return TaskValidationResult::failure(
            TaskError::InvalidPriority, QStringLiteral("Task priority is invalid."));
    }
    return TaskValidationResult::success();
}

} // namespace smartmate::model
