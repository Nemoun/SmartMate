#pragma once

#include <QUuid>

namespace smartmate::model {

using TaskId = QUuid;

enum class TaskStatus {
    Todo,
    InProgress,
    Done,
    Cancelled,
    Archived,
};

} // namespace smartmate::model
