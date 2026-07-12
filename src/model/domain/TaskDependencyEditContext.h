#pragma once

#include "domain/Task.h"

#include <QHash>
#include <QList>

namespace smartmate::model {

/// 一个前置候选及其由 Model 判定的初始选择和可选择资格。
struct TaskDependencyCandidate final {
    Task task;
    bool selected{false};
    bool selectable{false};

    friend bool operator==(const TaskDependencyCandidate &,
                           const TaskDependencyCandidate &) = default;
};

/// 打开依赖编辑器所需的原子业务快照；不包含任何界面文案或列表行号。
struct TaskDependencyEditContext final {
    Task targetTask;
    QList<TaskDependencyCandidate> candidates;
    /// 保留全图标题，确保循环路径经过隐藏任务时 ViewModel 仍可解释错误。
    QHash<TaskId, QString> taskTitles;

    friend bool operator==(const TaskDependencyEditContext &,
                           const TaskDependencyEditContext &) = default;
};

} // namespace smartmate::model
