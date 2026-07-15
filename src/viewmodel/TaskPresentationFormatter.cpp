#include "TaskPresentationFormatter.h"

#include <QStringList>

#include <algorithm>
#include <array>

namespace smartmate::viewmodel {
namespace {

struct TaskStatusDescriptor final {
    model::TaskStatus value;
    TaskStatusVisual visual;
    QString label;
};

struct TaskPriorityDescriptor final {
    model::TaskPriority value;
    TaskPriorityVisual visual;
    QString label;
};

struct TaskGraphFilterDescriptor final {
    TaskGraphStatusFilter value;
    QString label;
};

const auto &statusDescriptors()
{
    static const std::array<TaskStatusDescriptor, 5> descriptors{{
        {model::TaskStatus::Todo, TaskStatusVisual::Todo, QStringLiteral("待办")},
        {model::TaskStatus::InProgress, TaskStatusVisual::InProgress,
         QStringLiteral("进行中")},
        {model::TaskStatus::Done, TaskStatusVisual::Done, QStringLiteral("已完成")},
        {model::TaskStatus::Cancelled, TaskStatusVisual::Cancelled,
         QStringLiteral("已取消")},
        {model::TaskStatus::Archived, TaskStatusVisual::Archived,
         QStringLiteral("已归档")},
    }};
    return descriptors;
}

const auto &priorityDescriptors()
{
    static const std::array<TaskPriorityDescriptor, 4> descriptors{{
        {model::TaskPriority::Low, TaskPriorityVisual::Low, QStringLiteral("低")},
        {model::TaskPriority::Normal, TaskPriorityVisual::Normal,
         QStringLiteral("普通")},
        {model::TaskPriority::High, TaskPriorityVisual::High, QStringLiteral("高")},
        {model::TaskPriority::Urgent, TaskPriorityVisual::Urgent,
         QStringLiteral("紧急")},
    }};
    return descriptors;
}

const auto &graphFilterDescriptors()
{
    static const std::array<TaskGraphFilterDescriptor, 5> descriptors{{
        {TaskGraphStatusFilter::All, QStringLiteral("全部状态")},
        {TaskGraphStatusFilter::Todo, QStringLiteral("待办")},
        {TaskGraphStatusFilter::InProgress, QStringLiteral("进行中")},
        {TaskGraphStatusFilter::Blocked, QStringLiteral("阻塞")},
        {TaskGraphStatusFilter::Done, QStringLiteral("已完成")},
    }};
    return descriptors;
}

} // namespace

QString taskStatusText(const model::TaskStatus status)
{
    const auto &descriptors = statusDescriptors();
    const auto iterator = std::find_if(
        descriptors.cbegin(), descriptors.cend(),
        [status](const TaskStatusDescriptor &descriptor) {
            return descriptor.value == status;
        });
    return iterator == descriptors.cend() ? QStringLiteral("未知")
                                           : iterator->label;
}

QString taskPriorityText(const model::TaskPriority priority)
{
    const auto &descriptors = priorityDescriptors();
    const auto iterator = std::find_if(
        descriptors.cbegin(), descriptors.cend(),
        [priority](const TaskPriorityDescriptor &descriptor) {
            return descriptor.value == priority;
        });
    return iterator == descriptors.cend() ? QStringLiteral("未知")
                                           : iterator->label;
}

TaskStatusVisual taskStatusVisual(const model::TaskStatus status) noexcept
{
    const auto &descriptors = statusDescriptors();
    const auto iterator = std::find_if(
        descriptors.cbegin(), descriptors.cend(),
        [status](const TaskStatusDescriptor &descriptor) {
            return descriptor.value == status;
        });
    return iterator == descriptors.cend() ? TaskStatusVisual::Archived
                                           : iterator->visual;
}

TaskPriorityVisual taskPriorityVisual(const model::TaskPriority priority) noexcept
{
    const auto &descriptors = priorityDescriptors();
    const auto iterator = std::find_if(
        descriptors.cbegin(), descriptors.cend(),
        [priority](const TaskPriorityDescriptor &descriptor) {
            return descriptor.value == priority;
        });
    return iterator == descriptors.cend() ? TaskPriorityVisual::Low
                                           : iterator->visual;
}

int taskPriorityIndex(const model::TaskPriority priority) noexcept
{
    const auto &descriptors = priorityDescriptors();
    const auto iterator = std::find_if(
        descriptors.cbegin(), descriptors.cend(),
        [priority](const TaskPriorityDescriptor &descriptor) {
            return descriptor.value == priority;
        });
    return iterator == descriptors.cend()
        ? -1 : static_cast<int>(std::distance(descriptors.cbegin(), iterator));
}

std::optional<model::TaskPriority> taskPriorityFromIndex(const int index) noexcept
{
    const auto &descriptors = priorityDescriptors();
    if (index < 0 || index >= static_cast<int>(descriptors.size())) {
        return std::nullopt;
    }
    return descriptors.at(static_cast<std::size_t>(index)).value;
}

QStringList taskPriorityOptions()
{
    QStringList result;
    result.reserve(static_cast<qsizetype>(priorityDescriptors().size()));
    for (const auto &descriptor : priorityDescriptors()) {
        result.append(descriptor.label);
    }
    return result;
}

QStringList taskPriorityFilterOptions()
{
    QStringList result{QStringLiteral("全部优先级")};
    result.append(taskPriorityOptions());
    return result;
}

QStringList taskGraphStatusFilterOptions()
{
    QStringList result;
    result.reserve(static_cast<qsizetype>(graphFilterDescriptors().size()));
    for (const auto &descriptor : graphFilterDescriptors()) {
        result.append(descriptor.label);
    }
    return result;
}

TaskGraphStatusFilter taskGraphStatusFilterFromIndex(const int index) noexcept
{
    const auto &descriptors = graphFilterDescriptors();
    const int normalized = std::clamp(
        index, 0, static_cast<int>(descriptors.size()) - 1);
    return descriptors.at(static_cast<std::size_t>(normalized)).value;
}

QString taskDateTimeText(const QDateTime &dateTime, const QString &emptyText)
{
    return dateTime.isValid()
        ? dateTime.toString(QStringLiteral("yyyy-MM-dd HH:mm")) : emptyText;
}

QString taskDeadlineText(const model::Task &task, const QString &emptyText)
{
    // Model 保存 UTC；展示边界统一转换为用户本地时间。
    return task.deadline().has_value()
        ? taskDateTimeText(task.deadline()->toLocalTime(), emptyText) : emptyText;
}

QString taskDurationText(const model::Task &task, const QString &emptyText)
{
    if (!task.estimatedMinutes().has_value()) {
        return emptyText;
    }
    const int minutes = *task.estimatedMinutes();
    const int days = minutes / (24 * 60);
    const int hours = (minutes % (24 * 60)) / 60;
    const int minutePart = minutes % 60;
    QStringList parts;
    if (days > 0) parts.append(QStringLiteral("%1天").arg(days));
    if (hours > 0) parts.append(QStringLiteral("%1小时").arg(hours));
    if (minutePart > 0 || parts.isEmpty()) {
        parts.append(QStringLiteral("%1分钟").arg(minutePart));
    }
    return parts.join(QLatin1Char(' '));
}

} // namespace smartmate::viewmodel
