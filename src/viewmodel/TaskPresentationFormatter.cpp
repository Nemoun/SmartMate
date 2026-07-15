#include "TaskPresentationFormatter.h"

#include <QStringList>

namespace smartmate::viewmodel {

QString taskStatusText(const model::TaskStatus status)
{
    switch (status) {
    case model::TaskStatus::Todo: return QStringLiteral("待办");
    case model::TaskStatus::InProgress: return QStringLiteral("进行中");
    case model::TaskStatus::Done: return QStringLiteral("已完成");
    case model::TaskStatus::Cancelled: return QStringLiteral("已取消");
    case model::TaskStatus::Archived: return QStringLiteral("已归档");
    }
    return QStringLiteral("未知");
}

QString taskPriorityText(const model::TaskPriority priority)
{
    switch (priority) {
    case model::TaskPriority::Low: return QStringLiteral("低");
    case model::TaskPriority::Normal: return QStringLiteral("普通");
    case model::TaskPriority::High: return QStringLiteral("高");
    case model::TaskPriority::Urgent: return QStringLiteral("紧急");
    }
    return QStringLiteral("未知");
}

QString taskDeadlineText(const model::Task &task, const QString &emptyText)
{
    // Model 保存 UTC；展示边界统一转换为用户本地时间。
    return task.deadline().has_value()
        ? task.deadline()->toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"))
        : emptyText;
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
