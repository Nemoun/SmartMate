#pragma once

#include "domain/Task.h"
#include "domain/TaskCommandAvailability.h"
#include "domain/TaskDependency.h"

#include <QHash>
#include <QList>

namespace smartmate::model {

/// 基于同一份任务与依赖快照统一计算全部命令资格，不访问 Repository。
[[nodiscard]] QHash<TaskId, TaskCommandAvailability> taskCommandAvailabilities(
    const QList<Task> &tasks,
    const QList<TaskDependency> &dependencies);

} // namespace smartmate::model
