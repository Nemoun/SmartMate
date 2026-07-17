#pragma once

#include "domain/FocusSession.h"

#include <QList>

#include <optional>

namespace smartmate::model {

/// 原子专注写入的结构化结果；持久化故障仍通过 RepositoryException 抛出。
enum class FocusSessionWriteStatus : int {
    Success = 0,
    TaskNotInProgress,
    ActiveSessionExists,
    SessionNotFound,
    StateConflict,
};

struct FocusSessionWriteResult final {
    FocusSessionWriteStatus status{FocusSessionWriteStatus::Success};

    [[nodiscard]] bool ok() const noexcept
    {
        return status == FocusSessionWriteStatus::Success;
    }

    friend bool operator==(const FocusSessionWriteResult &,
                           const FocusSessionWriteResult &) = default;
};

/// 专注会话原始事实端口；不返回统计汇总、SQL类型或展示文案。
class IFocusSessionRepository {
public:
    virtual ~IFocusSessionRepository() = default;

    /// 返回全库唯一 Running/Paused 会话；不存在时返回空值。
    [[nodiscard]] virtual std::optional<FocusSession> findActiveFocusSession() const = 0;
    [[nodiscard]] virtual std::optional<FocusSession> findFocusSessionById(
        const FocusSessionId &sessionId) const = 0;
    /// 按从零开始的 sequence 稳定排序。
    [[nodiscard]] virtual QList<FocusInterval> findFocusIntervals(
        const FocusSessionId &sessionId) const = 0;
    /// 返回最近完成会话；limit 非正时返回空列表。
    [[nodiscard]] virtual QList<FocusSession> findRecentCompletedFocusSessions(
        int limit) const = 0;

    /// 校验任务仍为 InProgress，并在同一事务中插入会话与首个时间段。
    [[nodiscard]] virtual FocusSessionWriteResult startFocusSessionAtomically(
        const FocusSession &session) = 0;
    /// 期望 Running；关闭当前时间段并切换为 Paused。
    [[nodiscard]] virtual FocusSessionWriteResult pauseFocusSessionAtomically(
        const FocusSessionId &sessionId,
        FocusSessionState expectedState,
        const QDateTime &pausedAtUtc) = 0;
    /// 期望 Paused；切换为 Running 并创建下一序号时间段。
    [[nodiscard]] virtual FocusSessionWriteResult resumeFocusSessionAtomically(
        const FocusSessionId &sessionId,
        FocusSessionState expectedState,
        const QDateTime &resumedAtUtc) = 0;
    /// 允许按显式期望 Running/Paused 完成；Running 会先关闭当前时间段。
    [[nodiscard]] virtual FocusSessionWriteResult completeFocusSessionAtomically(
        const FocusSessionId &sessionId,
        FocusSessionState expectedState,
        const QDateTime &completedAtUtc) = 0;
    /// 物理删除期望状态仍匹配的活动会话，时间段由外键级联清理。
    [[nodiscard]] virtual FocusSessionWriteResult abandonFocusSessionAtomically(
        const FocusSessionId &sessionId,
        FocusSessionState expectedState) = 0;
    /// 只推进 Running 时间段的可靠检查点，不改变业务状态。
    [[nodiscard]] virtual FocusSessionWriteResult updateFocusCheckpointAtomically(
        const FocusSessionId &sessionId,
        FocusSessionState expectedState,
        const QDateTime &checkpointAtUtc) = 0;
    /// 将异常遗留的 Running 会话关闭在最后检查点并恢复为 Paused。
    [[nodiscard]] virtual FocusSessionWriteResult recoverRunningFocusSessionAtomically() = 0;
};

} // namespace smartmate::model
