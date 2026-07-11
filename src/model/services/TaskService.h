#pragma once

#include "domain/TaskCreationRequest.h"
#include "domain/TaskStateMachine.h"
#include "repositories/ITaskCreationRepository.h"
#include "repositories/ITaskDependencyRepository.h"
#include "repositories/ITaskRepository.h"
#include "services/TaskResult.h"

#include <QObject>

namespace smartmate::model {

/// 任务业务规则与持久化编排的唯一入口，不拥有注入的 Repository。
class TaskService final : public QObject {
    Q_OBJECT

public:
    TaskService(ITaskRepository &repository,
                ITaskDependencyRepository &dependencyRepository,
                ITaskCreationRepository &creationRepository,
                QObject *parent = nullptr);

    /// 执行无副作用的权威业务校验：不访问 Repository，也不发出状态通知。
    [[nodiscard]] TaskValidationResult validateDraft(const TaskDraft &draft) const;
    [[nodiscard]] TaskListResult listTasks() const;
    /// 返回新建关系可选的前置任务，排序稳定且不包含归档或取消任务。
    [[nodiscard]] TaskListResult listEligibleCreationPredecessors() const;
    /// 读取任务快照并应用 Model 排序策略，不持久化随时间变化的推荐排名。
    [[nodiscard]] TaskPlanResult listRecommendedTasks() const;
    [[nodiscard]] TaskDependencyListResult listDependencies() const;
    /// 生成依赖图领域快照；只保留活动节点及与其处于同一依赖组件的归档节点。
    [[nodiscard]] TaskGraphResult taskGraphSnapshot() const;
    /// 原子替换活动 Todo 任务的全部前置；空列表清空关系，失败不会部分写入。
    [[nodiscard]] TaskDependencyListResult replaceTaskPredecessors(
        const TaskId &taskId,
        const QList<TaskId> &predecessorIds);
    [[nodiscard]] TaskResult findTask(const TaskId &id) const;
    /// 加载可编辑任务；归档任务返回 ArchivedTaskNotEditable，调用方不得建立编辑草稿。
    [[nodiscard]] TaskResult findEditableTask(const TaskId &id) const;
    /// 校验草稿、生成稳定 TaskId，并持久化新的领域快照。
    [[nodiscard]] TaskResult createTask(const TaskDraft &draft);
    /// 原子新建任务与全部前置关系；任意校验或写入失败均不发送状态通知。
    [[nodiscard]] TaskResult createTask(const TaskCreationRequest &request);
    /// 按稳定 TaskId 更新活动任务；归档任务必须恢复后才能修改。
    [[nodiscard]] TaskResult updateTask(const TaskId &id, const TaskDraft &draft);
    /// 将 Todo 转换为 InProgress；同时检查前置关系和单进行中约束。
    [[nodiscard]] TaskResult startTask(const TaskId &id);
    /// 将 Todo/InProgress 转换为 Cancelled；依赖边保留但立即停止阻塞后继。
    [[nodiscard]] TaskResult cancelTask(const TaskId &id);
    /// 将 InProgress 转换为 Done；禁止 Todo 绕过开始阶段直接完成。
    [[nodiscard]] TaskResult completeTask(const TaskId &id);
    /// 将 Done/Cancelled 转换为 Todo；若重新激活依赖会破坏后继状态则拒绝。
    [[nodiscard]] TaskResult redoTask(const TaskId &id);
    /// 仅将 Done/Cancelled 软归档，不做物理删除。
    [[nodiscard]] TaskResult archiveTask(const TaskId &id);
    /// 恢复正常归档前状态；旧 Todo/InProgress 恢复点统一安全降级为 Todo。
    [[nodiscard]] TaskResult restoreTask(const TaskId &id);

signals:
    /// 仅在一次实际写入成功后发出；失败或无写入的幂等操作不会通知。
    void tasksChanged();
    /// 仅在前置集合实际替换成功后发出一次。
    void dependenciesChanged();

private:
    /// 复用唯一状态机执行转换，并确保一次成功命令只写入和通知一次。
    [[nodiscard]] TaskResult applyTransition(const TaskId &id,
                                             TaskTransition transition);

    // 非拥有引用，其生命周期必须长于 TaskService。
    ITaskRepository &m_repository;
    ITaskDependencyRepository &m_dependencyRepository;
    /// 独立命令端口保证跨 tasks 与 task_dependencies 的写入具有事务边界。
    ITaskCreationRepository &m_creationRepository;
};

} // namespace smartmate::model
