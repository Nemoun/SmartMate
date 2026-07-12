#include "services/TaskService.h"

#include "dependencies/TaskDependencyGraph.h"
#include "planner/TaskOrderingPolicy.h"
#include "planner/TaskCommandPolicy.h"
#include "services/TaskDraftValidator.h"
#include "services/TaskServiceSupport.h"

#include <QDateTime>
#include <QSet>

#include <algorithm>
#include <exception>
#include <optional>
#include <utility>

namespace smartmate::model {

TaskService::TaskService(ITaskRepository &repository,
                         ITaskDependencyRepository &dependencyRepository,
                         ITaskCreationRepository &creationRepository,
                         ITaskDeletionRepository &deletionRepository,
                         QObject *parent)
    : QObject(parent)
    , m_repository(repository)
    , m_dependencyRepository(dependencyRepository)
    , m_creationRepository(creationRepository)
    , m_deletionRepository(deletionRepository)
{
}

TaskValidationResult TaskService::validateDraft(const TaskDraft &draft) const
{
    return validateTaskDraft(draft);
}

TaskValidationResult TaskService::validateEstimatedMinutes(const int minutes) const
{
    return validateTaskEstimatedMinutes(minutes);
}

TaskListResult TaskService::listTasks() const
{
    try {
        return TaskListResult::success(m_repository.findAll());
    } catch (const RepositoryException &exception) {
        return persistenceListFailure(exception);
    } catch (...) {
        return unexpectedPersistenceListFailure();
    }
}

TaskListResult TaskService::listEligibleCreationPredecessors() const
{
    try {
        const QList<Task> tasks = m_repository.findAll();
        const QList<PlannedTask> recommended = orderTasks(
            tasks, QDateTime::currentDateTimeUtc());
        QList<Task> eligibleTasks;
        eligibleTasks.reserve(recommended.size());
        for (const PlannedTask &plannedTask : recommended) {
            if (plannedTask.task.status() != TaskStatus::Archived
                && plannedTask.task.status() != TaskStatus::Cancelled) {
                eligibleTasks.append(plannedTask.task);
            }
        }
        return TaskListResult::success(std::move(eligibleTasks));
    } catch (const RepositoryException &exception) {
        return persistenceListFailure(exception);
    } catch (...) {
        return unexpectedPersistenceListFailure();
    }
}

TaskPlanResult TaskService::listRecommendedTasks() const
{
    try {
        const QList<Task> tasks = m_repository.findAll();
        const QList<TaskDependency> dependencies =
            m_dependencyRepository.findAllDependencies();
        const TaskDependencyGraph graph{tasks, dependencies};
        const DependencyGraphValidation validation = graph.validation();
        if (!validation.ok()) {
            return graphFailure<QList<PlannedTask>>(validation);
        }
        const QList<ProtectedDependencyViolation> stateViolations =
            protectedStateViolations(tasks, graph);
        if (!stateViolations.isEmpty()) {
            return TaskPlanResult::failure(
                TaskError::DependencyStateConflict,
                QStringLiteral("Stored task dependencies violate an active/completed state."),
                stateViolationContext(stateViolations));
        }
        QList<PlannedTask> plan = orderTasks(
            tasks, dependencies, QDateTime::currentDateTimeUtc());
        const auto availabilities = taskCommandAvailabilities(tasks, dependencies);
        for (PlannedTask &planned : plan) {
            planned.availability = availabilities.value(planned.task.id());
        }
        return TaskPlanResult::success(std::move(plan));
    } catch (const RepositoryException &exception) {
        return persistencePlanFailure(exception);
    } catch (...) {
        return unexpectedPersistencePlanFailure();
    }
}

TaskDependencyListResult TaskService::listDependencies() const
{
    try {
        return TaskDependencyListResult::success(
            m_dependencyRepository.findAllDependencies());
    } catch (const RepositoryException &exception) {
        return dependencyPersistenceFailure(exception);
    } catch (...) {
        return unexpectedDependencyPersistenceFailure();
    }
}

TaskDependencyEditContextResult TaskService::taskDependencyEditContext(
    const TaskId &taskId) const
{
    try {
        const QList<Task> tasks = m_repository.findAll();
        const QList<TaskDependency> dependencies =
            m_dependencyRepository.findAllDependencies();
        const Task *target = findTaskInList(tasks, taskId);
        if (target == nullptr) {
            return TaskDependencyEditContextResult::failure(
                TaskError::DependencyEndpointNotFound,
                QStringLiteral("Dependency target task was not found."),
                TaskErrorContext{{}, {taskId}, {}});
        }
        if (target->status() != TaskStatus::Todo) {
            return TaskDependencyEditContextResult::failure(
                TaskError::DependencyTargetNotEditable,
                QStringLiteral("Only an active Todo task can edit predecessors."),
                TaskErrorContext{{}, {taskId}, {}});
        }

        const TaskDependencyGraph graph{tasks, dependencies};
        const DependencyGraphValidation validation = graph.validation();
        if (!validation.ok()) {
            return graphFailure<TaskDependencyEditContext>(validation);
        }

        const QList<TaskId> currentPredecessors = graph.predecessorIds(taskId);
        const QSet<TaskId> selected(currentPredecessors.cbegin(),
                                    currentPredecessors.cend());
        TaskDependencyEditContext context{*target, {}, {}};
        context.taskTitles.reserve(tasks.size());
        for (const Task &task : tasks) {
            context.taskTitles.insert(task.id(), task.title());
        }

        const QList<PlannedTask> ordered = orderTasks(
            tasks, dependencies, QDateTime::currentDateTimeUtc());
        context.candidates.reserve(ordered.size());
        for (const PlannedTask &planned : ordered) {
            const Task &candidate = planned.task;
            if (candidate.id() == taskId) {
                continue;
            }
            const bool alreadySelected = selected.contains(candidate.id());
            const bool eligible = candidate.status() != TaskStatus::Archived
                && candidate.status() != TaskStatus::Cancelled;
            if (eligible || alreadySelected) {
                context.candidates.append(
                    {candidate, alreadySelected, eligible || alreadySelected});
            }
        }
        return TaskDependencyEditContextResult::success(std::move(context));
    } catch (const RepositoryException &exception) {
        return TaskDependencyEditContextResult::failure(
            TaskError::PersistenceFailure, QString::fromUtf8(exception.what()));
    } catch (...) {
        return TaskDependencyEditContextResult::failure(
            TaskError::PersistenceFailure,
            QStringLiteral("Unexpected dependency repository failure."));
    }
}

TaskGraphResult TaskService::taskGraphSnapshot() const
{
    try {
        const QList<Task> tasks = m_repository.findAll();
        const QList<TaskDependency> dependencies =
            m_dependencyRepository.findAllDependencies();
        const TaskDependencyGraph graph{tasks, dependencies};
        const DependencyGraphValidation validation = graph.validation();
        if (!validation.ok()) {
            return graphFailure<TaskGraphSnapshot>(validation);
        }

        const QList<ProtectedDependencyViolation> stateViolations =
            protectedStateViolations(tasks, graph);
        if (!stateViolations.isEmpty()) {
            return TaskGraphResult::failure(
                TaskError::DependencyStateConflict,
                QStringLiteral("Stored task dependencies violate an active/completed state."),
                stateViolationContext(stateViolations));
        }

        QList<TaskId> activeTaskIds;
        for (const Task &task : tasks) {
            if (task.status() != TaskStatus::Archived) {
                activeTaskIds.append(task.id());
            }
        }
        const QList<TaskId> connectedIds = graph.connectedTaskIds(activeTaskIds);
        const QSet<TaskId> visibleIds(connectedIds.cbegin(), connectedIds.cend());
        const QHash<TaskId, int> levels = graph.dependencyLevels();

        TaskGraphSnapshot snapshot;
        const QList<PlannedTask> recommended = orderTasks(
            tasks, dependencies, QDateTime::currentDateTimeUtc());
        const auto availabilities = taskCommandAvailabilities(tasks, dependencies);
        snapshot.nodes.reserve(visibleIds.size());
        for (const PlannedTask &plannedTask : recommended) {
            if (!visibleIds.contains(plannedTask.task.id())) {
                continue;
            }
            snapshot.nodes.append({plannedTask.task,
                                   plannedTask.dependencyState,
                                   availabilities.value(plannedTask.task.id()),
                                   levels.value(plannedTask.task.id(), 0),
                                   graph.predecessorClosure(plannedTask.task.id()),
                                   graph.successorClosure(plannedTask.task.id())});
        }

        QList<TaskDependency> visibleDependencies;
        for (const TaskDependency &dependency : dependencies) {
            if (visibleIds.contains(dependency.predecessorId)
                && visibleIds.contains(dependency.successorId)) {
                visibleDependencies.append(dependency);
            }
        }
        std::sort(visibleDependencies.begin(), visibleDependencies.end(),
                  [](const TaskDependency &left,
                     const TaskDependency &right) {
            const QString leftPredecessor = stableId(left.predecessorId);
            const QString rightPredecessor = stableId(right.predecessorId);
            if (leftPredecessor != rightPredecessor) {
                return leftPredecessor < rightPredecessor;
            }
            return stableId(left.successorId) < stableId(right.successorId);
        });
        snapshot.edges.reserve(visibleDependencies.size());
        for (const TaskDependency &dependency : visibleDependencies) {
            const Task *predecessor = findTaskInList(
                tasks, dependency.predecessorId);
            snapshot.edges.append(
                {dependency,
                 predecessor != nullptr
                     ? TaskDependencyGraph::dependencyResolution(*predecessor)
                     : TaskDependencyResolution::Pending});
        }
        return TaskGraphResult::success(std::move(snapshot));
    } catch (const RepositoryException &exception) {
        return persistenceGraphFailure(exception);
    } catch (...) {
        return unexpectedPersistenceGraphFailure();
    }
}

TaskDependencyListResult TaskService::replaceTaskPredecessors(
    const TaskId &taskId,
    const QList<TaskId> &predecessorIds)
{
    try {
        const QList<Task> tasks = m_repository.findAll();
        const Task *target = findTaskInList(tasks, taskId);
        if (target == nullptr) {
            return TaskDependencyListResult::failure(
                TaskError::DependencyEndpointNotFound,
                QStringLiteral("Dependency target task was not found."),
                TaskErrorContext{{}, {taskId}, {}});
        }
        if (target->status() != TaskStatus::Todo) {
            return TaskDependencyListResult::failure(
                TaskError::DependencyTargetNotEditable,
                QStringLiteral("Only an active Todo task can replace predecessors."),
                TaskErrorContext{{}, {taskId}, {}});
        }

        QList<TaskId> normalizedPredecessors = predecessorIds;
        normalizeIds(normalizedPredecessors);
        if (normalizedPredecessors.size() != predecessorIds.size()) {
            QList<TaskId> duplicateIds;
            QSet<TaskId> seenIds;
            for (const TaskId &predecessorId : predecessorIds) {
                if (seenIds.contains(predecessorId)) {
                    duplicateIds.append(predecessorId);
                } else {
                    seenIds.insert(predecessorId);
                }
            }
            normalizeIds(duplicateIds);
            return TaskDependencyListResult::failure(
                TaskError::DependencyDuplicate,
                QStringLiteral("Task predecessor list contains duplicates."),
                TaskErrorContext{{}, duplicateIds, {}});
        }
        if (normalizedPredecessors.contains(taskId)) {
            return TaskDependencyListResult::failure(
                TaskError::DependencySelfReference,
                QStringLiteral("A task cannot depend on itself."),
                TaskErrorContext{{}, {taskId}, {}});
        }

        QList<TaskId> missingIds;
        for (const TaskId &predecessorId : normalizedPredecessors) {
            if (findTaskInList(tasks, predecessorId) == nullptr) {
                missingIds.append(predecessorId);
            }
        }
        normalizeIds(missingIds);
        if (!missingIds.isEmpty()) {
            return TaskDependencyListResult::failure(
                TaskError::DependencyEndpointNotFound,
                QStringLiteral("One or more predecessor tasks were not found."),
                TaskErrorContext{{}, missingIds, {}});
        }

        const QList<TaskDependency> currentDependencies =
            m_dependencyRepository.findAllDependencies();
        const QList<TaskDependency> currentIncoming =
            dependenciesForSuccessor(currentDependencies, taskId);
        QList<TaskId> currentPredecessors;
        currentPredecessors.reserve(currentIncoming.size());
        for (const TaskDependency &dependency : currentIncoming) {
            currentPredecessors.append(dependency.predecessorId);
        }

        QList<TaskId> ineligibleIds;
        for (const TaskId &predecessorId : normalizedPredecessors) {
            const Task *predecessor = findTaskInList(tasks, predecessorId);
            if (!currentPredecessors.contains(predecessorId)
                && predecessor != nullptr
                && (predecessor->status() == TaskStatus::Archived
                    || predecessor->status() == TaskStatus::Cancelled)) {
                ineligibleIds.append(predecessorId);
            }
        }
        normalizeIds(ineligibleIds);
        if (!ineligibleIds.isEmpty()) {
            return TaskDependencyListResult::failure(
                TaskError::DependencyPredecessorNotEligible,
                QStringLiteral("A newly selected predecessor must not be archived or cancelled."),
                TaskErrorContext{{}, ineligibleIds, {}});
        }

        QList<TaskDependency> replacementDependencies;
        replacementDependencies.reserve(
            currentDependencies.size() - currentIncoming.size()
            + normalizedPredecessors.size());
        for (const TaskDependency &dependency : currentDependencies) {
            if (dependency.successorId != taskId) {
                replacementDependencies.append(dependency);
            }
        }
        for (const TaskId &predecessorId : normalizedPredecessors) {
            replacementDependencies.append({predecessorId, taskId});
        }

        const DependencyGraphValidation validation =
            TaskDependencyGraph{tasks, replacementDependencies}.validation();
        if (!validation.ok()) {
            return graphFailure<QList<TaskDependency>>(validation);
        }

        if (currentPredecessors == normalizedPredecessors) {
            return TaskDependencyListResult::success(currentIncoming);
        }

        m_dependencyRepository.replacePredecessors(taskId, normalizedPredecessors);
        QList<TaskDependency> replacedIncoming;
        replacedIncoming.reserve(normalizedPredecessors.size());
        for (const TaskId &predecessorId : normalizedPredecessors) {
            replacedIncoming.append({predecessorId, taskId});
        }
        emit dependenciesChanged();
        return TaskDependencyListResult::success(std::move(replacedIncoming));
    } catch (const RepositoryException &exception) {
        return dependencyPersistenceFailure(exception);
    } catch (...) {
        return unexpectedDependencyPersistenceFailure();
    }
}

TaskResult TaskService::findTask(const TaskId &id) const
{
    try {
        const std::optional<Task> task = m_repository.findById(id);
        if (!task.has_value()) {
            return TaskResult::failure(TaskError::NotFound,
                                       QStringLiteral("Task was not found."));
        }
        return TaskResult::success(*task);
    } catch (const RepositoryException &exception) {
        return persistenceFailure(exception);
    } catch (...) {
        return unexpectedPersistenceFailure();
    }
}

TaskResult TaskService::findEditableTask(const TaskId &id) const
{
    TaskResult result = findTask(id);
    if (!result.ok() || result.value->canEditDetails()) {
        return result;
    }
    return TaskResult::failure(
        TaskError::TaskDetailsNotEditable,
        QStringLiteral("Only a Todo task can edit details."),
        TaskErrorContext{{}, {id}, {}});
}

TaskResult TaskService::createTask(const TaskDraft &draft)
{
    return createTask(TaskCreationRequest{draft, {}});
}

TaskResult TaskService::createTask(const TaskCreationRequest &request)
{
    const TaskValidationResult validation = validateDraft(request.task);
    if (!validation.ok()) {
        return TaskResult::failure(validation.error, validation.detail);
    }

    try {
        const QList<Task> tasks = m_repository.findAll();
        QList<TaskId> normalizedPredecessors = request.predecessorIds;
        normalizeIds(normalizedPredecessors);
        if (normalizedPredecessors.size() != request.predecessorIds.size()) {
            QList<TaskId> duplicateIds;
            QSet<TaskId> seenIds;
            for (const TaskId &predecessorId : request.predecessorIds) {
                if (seenIds.contains(predecessorId)) {
                    duplicateIds.append(predecessorId);
                } else {
                    seenIds.insert(predecessorId);
                }
            }
            normalizeIds(duplicateIds);
            return TaskResult::failure(
                TaskError::DependencyDuplicate,
                QStringLiteral("Task predecessor list contains duplicates."),
                TaskErrorContext{{}, duplicateIds, {}});
        }

        QList<TaskId> missingIds;
        QList<TaskId> ineligibleIds;
        for (const TaskId &predecessorId : normalizedPredecessors) {
            const Task *predecessor = findTaskInList(tasks, predecessorId);
            if (predecessor == nullptr) {
                missingIds.append(predecessorId);
            } else if (predecessor->status() == TaskStatus::Archived
                       || predecessor->status() == TaskStatus::Cancelled) {
                ineligibleIds.append(predecessorId);
            }
        }
        normalizeIds(missingIds);
        if (!missingIds.isEmpty()) {
            return TaskResult::failure(
                TaskError::DependencyEndpointNotFound,
                QStringLiteral("One or more creation predecessors were not found."),
                TaskErrorContext{{}, missingIds, {}});
        }
        normalizeIds(ineligibleIds);
        if (!ineligibleIds.isEmpty()) {
            return TaskResult::failure(
                TaskError::DependencyPredecessorNotEligible,
                QStringLiteral("A creation predecessor must not be archived or cancelled."),
                TaskErrorContext{{}, ineligibleIds, {}});
        }

        TaskId taskId;
        do {
            taskId = QUuid::createUuid();
        } while (findTaskInList(tasks, taskId) != nullptr);
        Task task = makeNewTask(taskId,
                                request.task,
                                QDateTime::currentDateTimeUtc());

        QList<Task> hypotheticalTasks = tasks;
        hypotheticalTasks.append(task);
        QList<TaskDependency> hypotheticalDependencies =
            m_dependencyRepository.findAllDependencies();
        for (const TaskId &predecessorId : normalizedPredecessors) {
            hypotheticalDependencies.append({predecessorId, task.id()});
        }
        const TaskDependencyGraph graph{hypotheticalTasks,
                                        hypotheticalDependencies};
        const DependencyGraphValidation graphValidation = graph.validation();
        if (!graphValidation.ok()) {
            return graphFailure<Task>(graphValidation);
        }
        const QList<ProtectedDependencyViolation> stateViolations =
            protectedStateViolations(hypotheticalTasks, graph);
        if (!stateViolations.isEmpty()) {
            return TaskResult::failure(
                TaskError::DependencyStateConflict,
                QStringLiteral("Stored task dependencies violate an active/completed state."),
                stateViolationContext(stateViolations));
        }

        try {
            m_creationRepository.insertTaskWithPredecessors(
                task, normalizedPredecessors);
        } catch (const RepositoryException &exception) {
            return persistenceFailure(exception);
        }
        emit tasksChanged();
        if (!normalizedPredecessors.isEmpty()) {
            emit dependenciesChanged();
        }
        return TaskResult::success(std::move(task));
    } catch (const RepositoryException &exception) {
        return persistenceFailure(exception);
    } catch (...) {
        return unexpectedPersistenceFailure();
    }
}

TaskResult TaskService::updateTask(const TaskId &id, const TaskDraft &draft)
{
    try {
        const QList<Task> tasks = m_repository.findAll();
        const Task *current = findTaskInList(tasks, id);
        if (current == nullptr) {
            return TaskResult::failure(TaskError::NotFound,
                                       QStringLiteral("Task was not found."));
        }
        // 编辑资格必须在Model最终判定，避免View隐藏按钮后仍可绕过界面更新非待办任务。
        if (!current->canEditDetails()) {
            return TaskResult::failure(
                TaskError::TaskDetailsNotEditable,
                QStringLiteral("Only a Todo task can edit details."),
                TaskErrorContext{{}, {id}, {}});
        }
        const TaskValidationResult validation = validateDraft(draft);
        if (!validation.ok()) {
            return TaskResult::failure(validation.error, validation.detail);
        }
        Task updated = makeTaskWithDetails(
            *current, draft, QDateTime::currentDateTimeUtc());
        if (!m_repository.update(updated)) {
            return TaskResult::failure(TaskError::NotFound,
                                       QStringLiteral("Task was not found during update."));
        }
        emit tasksChanged();
        return TaskResult::success(std::move(updated));
    } catch (const RepositoryException &exception) {
        return persistenceFailure(exception);
    } catch (...) {
        return unexpectedPersistenceFailure();
    }
}

TaskResult TaskService::startTask(const TaskId &id)
{
    return applyTransition(id, TaskTransition::Start);
}

TaskResult TaskService::cancelTask(const TaskId &id)
{
    return applyTransition(id, TaskTransition::Cancel);
}

TaskResult TaskService::completeTask(const TaskId &id)
{
    return applyTransition(id, TaskTransition::Complete);
}

TaskResult TaskService::redoTask(const TaskId &id)
{
    return applyTransition(id, TaskTransition::Redo);
}

TaskResult TaskService::archiveTask(const TaskId &id)
{
    return applyTransition(id, TaskTransition::Archive);
}

TaskResult TaskService::restoreTask(const TaskId &id)
{
    return applyTransition(id, TaskTransition::Restore);
}

TaskResult TaskService::deleteArchivedTask(const TaskId &id)
{
    try {
        const std::optional<Task> current = m_repository.findById(id);
        if (!current.has_value()) {
            return TaskResult::failure(TaskError::NotFound,
                                       QStringLiteral("Task was not found."));
        }
        if (!current->canDeletePermanently()) {
            return TaskResult::failure(
                TaskError::TaskDeletionNotAllowed,
                QStringLiteral("Only an archived task can be permanently deleted."),
                TaskErrorContext{{}, {id}, {}});
        }

        const TaskDeletionWriteResult writeResult =
            m_deletionRepository.deleteArchivedTaskWithDependencies(id);
        if (!writeResult.taskDeleted) {
            // 端口的条件删除会在状态竞争或目标消失时完整回滚；重读后返回稳定业务错误。
            const std::optional<Task> latest = m_repository.findById(id);
            if (!latest.has_value()) {
                return TaskResult::failure(
                    TaskError::NotFound,
                    QStringLiteral("Task was not found during permanent deletion."));
            }
            if (!latest->canDeletePermanently()) {
                return TaskResult::failure(
                    TaskError::TaskDeletionNotAllowed,
                    QStringLiteral("Task is no longer archived and cannot be permanently deleted."),
                    TaskErrorContext{{}, {id}, {}});
            }
            return TaskResult::failure(
                TaskError::PersistenceFailure,
                QStringLiteral("Deletion repository did not delete the archived task."));
        }

        emit tasksChanged();
        if (writeResult.removedDependencyCount > 0) {
            emit dependenciesChanged();
        }
        return TaskResult::success(*current);
    } catch (const RepositoryException &exception) {
        return persistenceFailure(exception);
    } catch (...) {
        return unexpectedPersistenceFailure();
    }
}

TaskResult TaskService::applyTransition(const TaskId &id,
                                        const TaskTransition transition)
{
    try {
        const QList<Task> tasks = m_repository.findAll();
        const Task *current = findTaskInList(tasks, id);
        if (current == nullptr) {
            return TaskResult::failure(TaskError::NotFound,
                                       QStringLiteral("Task was not found."));
        }

        const std::optional<TaskStatus> targetStatus =
            TaskStateMachine::targetStatus(*current, transition);
        if (!targetStatus.has_value()) {
            return TaskResult::failure(
                TaskError::InvalidTaskTransition,
                QStringLiteral("The task state does not allow this transition."),
                TaskErrorContext{{}, {id}, {}});
        }

        if (*targetStatus == TaskStatus::InProgress) {
            const QList<TaskId> conflictingIds = otherInProgressTaskIds(tasks, id);
            if (!conflictingIds.isEmpty()) {
                return TaskResult::failure(
                    TaskError::InProgressConflict,
                    QStringLiteral("Another task is already in progress."),
                    TaskErrorContext{{}, conflictingIds, {}});
            }
        }

        const std::optional<TaskStatus> statusBeforeArchive =
            transition == TaskTransition::Archive
            ? std::optional<TaskStatus>{current->status()}
            : std::nullopt;
        Task transitioned = makeTaskWithStatus(
            *current,
            *targetStatus,
            statusBeforeArchive,
            QDateTime::currentDateTimeUtc());

        // Cancel 会让依赖边停止阻塞，只可能修复而不会制造后继冲突；
        // 因此必须绕过受保护后继检查，避免旧的无关异常阻止用户取消任务。
        if (transition != TaskTransition::Cancel) {
            QList<Task> transitionedTasks = tasks;
            replaceTaskSnapshot(transitionedTasks, transitioned);
            const QList<TaskDependency> dependencies =
                m_dependencyRepository.findAllDependencies();
            if (const auto dependencyFailure = dependencyStateFailure(
                    transitionedTasks, dependencies, id)) {
                return *dependencyFailure;
            }
        }

        try {
            if (!m_repository.update(transitioned)) {
                return TaskResult::failure(
                    TaskError::NotFound,
                    QStringLiteral("Task was not found during state transition."));
            }
        } catch (const RepositoryException &exception) {
            if (*targetStatus == TaskStatus::InProgress) {
                return mapInProgressWriteFailure(m_repository, id, exception);
            }
            return persistenceFailure(exception);
        }

        emit tasksChanged();
        return TaskResult::success(std::move(transitioned));
    } catch (const RepositoryException &exception) {
        return persistenceFailure(exception);
    } catch (...) {
        return unexpectedPersistenceFailure();
    }
}

} // namespace smartmate::model
