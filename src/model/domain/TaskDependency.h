#pragma once

#include "domain/TaskTypes.h"

#include <QList>

namespace smartmate::model {

/// Finish-to-Start 关系；predecessor 完成后 successor 才能开始。
struct TaskDependency final {
    TaskId predecessorId;
    TaskId successorId;

    friend bool operator==(const TaskDependency &, const TaskDependency &) = default;
};

/// 前置关系的派生解析结果；Cancelled 表示关系保留但暂时不阻塞后继。
enum class TaskDependencyResolution {
    Pending,
    Satisfied,
    Cancelled,
};

/// 由任务快照和依赖关系即时计算的状态，不得作为冗余字段写入数据库。
struct TaskDependencyState final {
    /// 全部直接前置，始终按稳定 TaskId 排序。
    QList<TaskId> predecessorIds;
    /// 当前仍会阻塞 Finish-to-Start 的直接前置；已取消关系不属于阻塞项。
    QList<TaskId> unsatisfiedPredecessorIds;
    /// 关系仍存在但因前置已取消而暂时失效的直接前置。
    QList<TaskId> cancelledPredecessorIds;
    /// 若本任务变为 Done 或 Cancelled，将被直接解锁的 Todo 后继数量。
    int unlockCount{0};
    /// 仅 Todo/InProgress 会被标为阻塞；终态仍保留上述诊断列表。
    bool blocked{false};

    friend bool operator==(const TaskDependencyState &,
                           const TaskDependencyState &) = default;
};

} // namespace smartmate::model
