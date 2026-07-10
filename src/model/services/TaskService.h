#pragma once

#include "repositories/ITaskRepository.h"

#include <QList>

namespace smartmate::model {

class TaskService final {
public:
    explicit TaskService(ITaskRepository &repository);

    [[nodiscard]] QList<Task> listTasks() const;

private:
    ITaskRepository &m_repository;
};

} // namespace smartmate::model
