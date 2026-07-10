#pragma once

#include "domain/Task.h"

#include <QList>

namespace smartmate::model {

class ITaskRepository {
public:
    virtual ~ITaskRepository() = default;

    [[nodiscard]] virtual QList<Task> findAll() const = 0;
};

} // namespace smartmate::model
