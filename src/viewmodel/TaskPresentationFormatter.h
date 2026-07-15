#pragma once

#include "domain/Task.h"
#include "viewmodel/contracts/TaskPresentationTypes.h"

#include <QString>
#include <QStringList>

#include <optional>

namespace smartmate::viewmodel {

/// 将领域枚举和值对象映射为统一中文文案，不包含任何业务资格判断。
[[nodiscard]] QString taskStatusText(model::TaskStatus status);
[[nodiscard]] QString taskPriorityText(model::TaskPriority priority);
/// 将领域状态显式映射为 Contract 视觉值；非法领域值使用 Archived 安全回退。
[[nodiscard]] TaskStatusVisual taskStatusVisual(model::TaskStatus status) noexcept;
/// 将领域优先级显式映射为 Contract 视觉值；非法领域值使用 Low 安全回退。
[[nodiscard]] TaskPriorityVisual taskPriorityVisual(
    model::TaskPriority priority) noexcept;
/// 返回编辑器使用的零基稳定优先级索引；非法领域值返回 -1。
[[nodiscard]] int taskPriorityIndex(model::TaskPriority priority) noexcept;
/// 将编辑器零基索引转换为领域优先级；非法索引返回空值。
[[nodiscard]] std::optional<model::TaskPriority> taskPriorityFromIndex(
    int index) noexcept;
[[nodiscard]] QStringList taskPriorityOptions();
[[nodiscard]] QStringList taskPriorityFilterOptions();
[[nodiscard]] QStringList taskGraphStatusFilterOptions();
/// 按既有范围规则规范化图筛选索引，小于零回到 All，大于上限收敛为 Done。
[[nodiscard]] TaskGraphStatusFilter taskGraphStatusFilterFromIndex(int index) noexcept;
/// 使用统一格式输出给定时区的时间，不执行任何时区转换。
[[nodiscard]] QString taskDateTimeText(const QDateTime &dateTime,
                                       const QString &emptyText = {});
/// 将 UTC 截止时间转换为本地时间文本；未设置时返回调用方提供的占位文案。
[[nodiscard]] QString taskDeadlineText(const model::Task &task,
                                       const QString &emptyText);
/// 将总分钟拆分为天/小时/分钟文案；未设置时返回占位文案。
[[nodiscard]] QString taskDurationText(const model::Task &task,
                                       const QString &emptyText);

} // namespace smartmate::viewmodel
