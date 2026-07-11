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

/// 由任务快照和依赖关系即时计算的状态，不得作为冗余字段写入数据库。
struct TaskDependencyState final {
    /// 全部直接前置，始终按稳定 TaskId 排序。
    QList<TaskId> predecessorIds;
    /// 当前尚未满足 Finish-to-Start 条件的直接前置。
    QList<TaskId> unsatisfiedPredecessorIds;
    /// 若本任务变为 Done，将被直接解锁的 Todo 后继数量。
    int unlockCount{0};
    /// 仅 Todo/InProgress 会被标为阻塞；终态仍保留上述诊断列表。
    bool blocked{false};

    friend bool operator==(const TaskDependencyState &,
                           const TaskDependencyState &) = default;
};

} // namespace smartmate::model
