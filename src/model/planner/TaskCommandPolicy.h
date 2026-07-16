#pragma once

#include "domain/Task.h"
#include "domain/TaskCommandAvailability.h"
#include "domain/TaskDependency.h"

#include <QHash>
#include <QList>

namespace smartmate::model {

/// 基于同一份任务与依赖快照统一计算展示用命令资格，不访问 Repository。
/// 资格可能在下一次写入前过期，因此 TaskService 执行命令时仍须重新读取并复核。
[[nodiscard]] QHash<TaskId, TaskCommandAvailability> taskCommandAvailabilities(
    const QList<Task> &tasks,
    const QList<TaskDependency> &dependencies);

} // namespace smartmate::model
