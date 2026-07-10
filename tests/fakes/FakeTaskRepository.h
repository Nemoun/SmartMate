#pragma once

#include "repositories/ITaskRepository.h"

#include <QList>

#include <utility>

namespace smartmate::tests {

class FakeTaskRepository final : public model::ITaskRepository {
public:
    explicit FakeTaskRepository(QList<model::Task> tasks = {})
        : m_tasks(std::move(tasks))
    {
    }

    [[nodiscard]] QList<model::Task> findAll() const override
    {
        return m_tasks;
    }

private:
    QList<model::Task> m_tasks;
};

} // namespace smartmate::tests
