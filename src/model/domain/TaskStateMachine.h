#pragma once

#include "domain/Task.h"

#include <optional>

namespace smartmate::model {

/// 用户可执行的显式任务状态命令；普通字段编辑不得隐式触发这些转换。
enum class TaskTransition {
    Start,
    Cancel,
    Complete,
    Redo,
    Archive,
    Restore,
};

/// 任务状态转换的唯一领域规则表，不访问 Repository，也不包含界面文案。
class TaskStateMachine final {
public:
    /// 返回命令的目标状态；来源状态不允许该命令时返回空值。
    ///
    /// Restore 会把旧数据中的 Todo/InProgress/非法恢复点安全归一为 Todo，
    /// 防止旧归档数据绕过新的 Start 流程直接回到 InProgress。
    [[nodiscard]] static std::optional<TaskStatus> targetStatus(
        const Task &task,
        TaskTransition transition) noexcept;

    /// 仅判断状态矩阵资格；依赖、单进行中和持久化约束仍由 TaskService 检查。
    [[nodiscard]] static bool canApply(const Task &task,
                                       TaskTransition transition) noexcept;
};

} // namespace smartmate::model
