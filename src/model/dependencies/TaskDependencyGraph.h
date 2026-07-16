#pragma once

#include "domain/Task.h"
#include "domain/TaskDependency.h"

#include <QHash>
#include <QList>

namespace smartmate::model {

/// 依赖快照本身不合法时的结构化类别；具体关联 ID 由 validation() 返回。
enum class DependencyGraphError {
    /// 图结构合法。
    None,
    /// 至少一个边端点不在任务快照中。
    MissingTask,
    /// 任务把自己作为直接前置。
    SelfDependency,
    /// 同一前置和后继重复出现。
    DuplicateDependency,
    /// 有向关系中存在闭合路径。
    Cycle,
};

/// 图校验上下文；cyclePath 在成环时包含首尾相同的完整闭合路径。
struct DependencyGraphValidation final {
    /// 首个被稳定校验顺序发现的错误类别。
    DependencyGraphError error{DependencyGraphError::None};
    /// 与错误有关的稳定 TaskId 集合，按 ID 排序且不重复。
    QList<TaskId> conflictingTaskIds;
    /// 有向环的实际经过顺序；非循环错误时为空。
    QList<TaskId> cyclePath;

    [[nodiscard]] bool ok() const noexcept
    {
        return error == DependencyGraphError::None;
    }
};

/// 任务与 Finish-to-Start 关系的纯 C++ 快照算法，不访问 Repository 或界面。
class TaskDependencyGraph final {
public:
    /// 获取任务与边的独立快照，并建立稳定 TaskId 到任务下标的查询索引。
    TaskDependencyGraph(QList<Task> tasks, QList<TaskDependency> dependencies);

    /// 依次检查悬空端点、自依赖、重复边和有向环。
    [[nodiscard]] DependencyGraphValidation validation() const;
    /// 判断稳定 TaskId 是否存在于当前任务快照。
    [[nodiscard]] bool containsTask(const TaskId &taskId) const;
    /// 返回直接前置集合，结果按稳定 TaskId 排序并去重。
    [[nodiscard]] QList<TaskId> predecessorIds(const TaskId &taskId) const;
    /// 返回直接后继集合，结果按稳定 TaskId 排序并去重。
    [[nodiscard]] QList<TaskId> successorIds(const TaskId &taskId) const;
    /// 返回解析状态仍为 Pending 的直接前置，实现多个前置的 AND 语义。
    [[nodiscard]] QList<TaskId> unsatisfiedPredecessorIds(const TaskId &taskId) const;
    /// 返回关系仍保留但因前置取消而暂时失效的直接前置。
    [[nodiscard]] QList<TaskId> cancelledPredecessorIds(const TaskId &taskId) const;
    /// 汇总指定任务的直接前置、阻塞和可直接解锁数量。
    [[nodiscard]] TaskDependencyState dependencyState(const TaskId &taskId) const;
    /// 返回构造时保存的原始边快照，不授予修改权限。
    [[nodiscard]] const QList<TaskDependency> &dependencies() const noexcept;

    /// 返回每个节点的最长前置链层级；根节点为 0，非法图返回空映射。
    [[nodiscard]] QHash<TaskId, int> dependencyLevels() const;
    /// 返回指定节点全部传递前置，结果按稳定 TaskId 排序；非法或缺失节点返回空集合。
    [[nodiscard]] QList<TaskId> predecessorClosure(const TaskId &taskId) const;
    /// 返回指定节点全部传递后继，结果按稳定 TaskId 排序；非法或缺失节点返回空集合。
    [[nodiscard]] QList<TaskId> successorClosure(const TaskId &taskId) const;
    /// 将有向边视为无向连接，从一组种子求稳定闭包；悬空端点会被忽略。
    [[nodiscard]] QList<TaskId> connectedTaskIds(
        const QList<TaskId> &seedTaskIds) const;

    /// 返回前置任务对 Finish-to-Start 边的结构化解析结果。
    [[nodiscard]] static TaskDependencyResolution dependencyResolution(
        const Task &task) noexcept;

    /// 仅判断前置是否以 Done（含从 Done 归档）满足关系；阻塞判断应使用解析状态。
    [[nodiscard]] static bool satisfiesDependency(const Task &task) noexcept;

private:
    /// 通过预建下标查找任务；缺失时返回 nullptr。
    [[nodiscard]] const Task *findTask(const TaskId &taskId) const;

    /// 算法拥有的任务值快照，生命周期与图对象一致。
    QList<Task> m_tasks;
    /// 算法拥有的有向边值快照。
    QList<TaskDependency> m_dependencies;
    /// 稳定 TaskId 到快照下标的索引，使长链校验保持 O(V+E) 查找复杂度。
    QHash<TaskId, qsizetype> m_taskIndexes;
};

} // namespace smartmate::model
