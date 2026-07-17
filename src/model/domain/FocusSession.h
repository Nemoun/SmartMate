#pragma once

#include "domain/TaskActivityEvent.h"
#include "domain/TaskCategory.h"
#include "domain/TaskTypes.h"

#include <QDateTime>
#include <QString>
#include <QUuid>

#include <optional>

namespace smartmate::model {

/// 一次专注会话的稳定身份；列表行号和任务 ID 都不能替代它。
using FocusSessionId = QUuid;

/// 持久化会话状态；放弃会话会被物理删除，因此不保留 Abandoned 状态。
enum class FocusSessionState : int {
    Running = 0,
    Paused = 1,
    Completed = 2,
};

/// 绑定任务执行周期的一次专注会话事实；所有时间均使用 UTC。
///
/// 任务、预计用时和类别字段是开始专注时的快照，后续任务重做或类别变更
/// 不得改写历史。旧数据库缺少 Start 事件时 taskStartEventId 可以为空。
struct FocusSession final {
    FocusSessionId sessionId;
    TaskId taskId;
    FocusSessionState state{FocusSessionState::Running};
    QDateTime startedAtUtc;
    std::optional<QDateTime> endedAtUtc;
    QString taskTitleSnapshot;
    std::optional<int> estimatedMinutesSnapshot;
    std::optional<TaskCategoryId> categoryIdSnapshot;
    std::optional<QString> categoryNameSnapshot;
    std::optional<TaskCategoryColor> categoryColorSnapshot;
    std::optional<TaskActivityEventId> taskStartEventId;

    friend bool operator==(const FocusSession &, const FocusSession &) = default;
};

/// 一段连续有效专注时间；暂停会关闭当前段，继续会创建下一序号段。
///
/// Running 时间段的 endedAtUtc 为空，checkpointAtUtc 用于异常退出时把
/// 未关闭时间段恢复到最后一次可靠写入位置。
struct FocusInterval final {
    FocusSessionId sessionId;
    int sequence{0};
    QDateTime startedAtUtc;
    std::optional<QDateTime> endedAtUtc;
    QDateTime checkpointAtUtc;

    friend bool operator==(const FocusInterval &, const FocusInterval &) = default;
};

} // namespace smartmate::model
