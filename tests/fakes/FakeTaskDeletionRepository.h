#pragma once

#include "fakes/FakeTaskDependencyRepository.h"
#include "fakes/FakeTaskRepository.h"

#include "repositories/ITaskDeletionRepository.h"

#include <QList>

#include <algorithm>
#include <optional>
#include <utility>

namespace smartmate::tests {

/// Service测试使用的永久删除端口，可控制事务结果、故障并记录稳定TaskId调用。
class FakeTaskDeletionRepository final
    : public model::ITaskDeletionRepository {
public:
    FakeTaskDeletionRepository() = default;

    FakeTaskDeletionRepository(FakeTaskRepository &taskRepository,
                               FakeTaskDependencyRepository &dependencyRepository)
        : m_taskRepository(&taskRepository)
        , m_dependencyRepository(&dependencyRepository)
    {
    }

    [[nodiscard]] model::TaskDeletionWriteResult
    deleteArchivedTaskWithDependencies(const model::TaskId &taskId) override
    {
        if (m_failWrites) {
            throw model::RepositoryException("Fake permanent deletion failure.");
        }
        m_deletedTaskIds.append(taskId);
        if (m_forcedResult.has_value()) {
            return *m_forcedResult;
        }
        if (m_taskRepository == nullptr || m_dependencyRepository == nullptr) {
            return {true, 0};
        }

        const auto taskIterator = std::find_if(
            m_taskRepository->m_tasks.cbegin(),
            m_taskRepository->m_tasks.cend(),
            [&taskId](const model::Task &task) { return task.id() == taskId; });
        if (taskIterator == m_taskRepository->m_tasks.cend()
            || taskIterator->status() != model::TaskStatus::Archived) {
            return {};
        }

        QList<model::TaskDependency> retainedDependencies;
        retainedDependencies.reserve(m_dependencyRepository->m_dependencies.size());
        int removedDependencyCount = 0;
        for (const model::TaskDependency &dependency
             : std::as_const(m_dependencyRepository->m_dependencies)) {
            if (dependency.predecessorId == taskId
                || dependency.successorId == taskId) {
                ++removedDependencyCount;
            } else {
                retainedDependencies.append(dependency);
            }
        }

        // 两个容器只在所有资格检查完成后一起替换，模拟真实Repository的事务发布点。
        m_taskRepository->m_tasks.erase(taskIterator);
        m_dependencyRepository->m_dependencies = std::move(retainedDependencies);
        return {true, removedDependencyCount};
    }

    void setResult(const model::TaskDeletionWriteResult result) noexcept
    {
        m_forcedResult = result;
    }

    void setWriteFailure(const bool enabled) noexcept
    {
        m_failWrites = enabled;
    }

    [[nodiscard]] const QList<model::TaskId> &deletedTaskIds() const noexcept
    {
        return m_deletedTaskIds;
    }

private:
    FakeTaskRepository *m_taskRepository{nullptr};
    FakeTaskDependencyRepository *m_dependencyRepository{nullptr};
    std::optional<model::TaskDeletionWriteResult> m_forcedResult;
    QList<model::TaskId> m_deletedTaskIds;
    bool m_failWrites{false};
};

} // namespace smartmate::tests
