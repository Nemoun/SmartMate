#include "services/TaskDraftValidator.h"

#include "domain/TaskConstraints.h"
#include "domain/UnicodeText.h"

namespace smartmate::model {
namespace {

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
            QStringLiteral("Estimated minutes must be between %1 and %2.")
                .arg(TaskConstraints::minimumEstimatedMinutes)
                .arg(TaskConstraints::maximumEstimatedMinutes));
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
    if (unicodeCodePointCount(normalizedTitle)
        > TaskConstraints::maximumTitleLength) {
        return TaskValidationResult::failure(
            TaskError::TitleTooLong,
            QStringLiteral("Task title exceeds %1 characters.")
                .arg(TaskConstraints::maximumTitleLength));
    }
    if (unicodeCodePointCount(draft.description)
        > TaskConstraints::maximumDescriptionLength) {
        return TaskValidationResult::failure(
            TaskError::DescriptionTooLong,
            QStringLiteral("Task description exceeds %1 characters.")
                .arg(TaskConstraints::maximumDescriptionLength));
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
