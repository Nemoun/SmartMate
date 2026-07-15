#pragma once

namespace smartmate::viewmodel {

/// Contract 边界使用的任务状态视觉值；显式数值必须与既有 Role 协议保持兼容。
enum class TaskStatusVisual : int {
    Todo = 0,
    InProgress = 1,
    Done = 2,
    Cancelled = 3,
    Archived = 4,
};

/// Contract 边界使用的任务优先级视觉值；不依赖 Model 枚举序号。
enum class TaskPriorityVisual : int {
    Low = 0,
    Normal = 1,
    High = 2,
    Urgent = 3,
};

/// 依赖图状态筛选的稳定选项值；Blocked 是展示筛选而非领域状态。
enum class TaskGraphStatusFilter : int {
    All = 0,
    Todo = 1,
    InProgress = 2,
    Blocked = 3,
    Done = 4,
};

} // namespace smartmate::viewmodel
