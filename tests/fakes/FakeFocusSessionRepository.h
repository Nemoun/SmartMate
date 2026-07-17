#pragma once

#include "repositories/IFocusSessionRepository.h"
#include "repositories/RepositoryException.h"

#include <QHash>

#include <algorithm>
#include <optional>

namespace smartmate::tests {

/// FocusService 单元测试使用的内存事实端口；只模拟 Repository 原子语义。
class FakeFocusSessionRepository final
    : public model::IFocusSessionRepository {
public:
    [[nodiscard]] std::optional<model::FocusSession> findActiveFocusSession()
        const override
    {
        throwIfReadFails();
        for (const auto &session : m_sessions) {
            if (session.state == model::FocusSessionState::Running
                || session.state == model::FocusSessionState::Paused) {
                return session;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<model::FocusSession> findFocusSessionById(
        const model::FocusSessionId &sessionId) const override
    {
        throwIfReadFails();
        const auto found = m_sessions.constFind(sessionId);
        return found == m_sessions.cend()
            ? std::nullopt : std::optional<model::FocusSession>{found.value()};
    }

    [[nodiscard]] QList<model::FocusInterval> findFocusIntervals(
        const model::FocusSessionId &sessionId) const override
    {
        throwIfReadFails();
        auto result = m_intervals.value(sessionId);
        std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
            return left.sequence < right.sequence;
        });
        return result;
    }

    [[nodiscard]] QList<model::FocusSession> findRecentCompletedFocusSessions(
        const int limit) const override
    {
        throwIfReadFails();
        if (limit <= 0) return {};
        QList<model::FocusSession> result;
        for (const auto &session : m_sessions) {
            if (session.state == model::FocusSessionState::Completed) {
                result.append(session);
            }
        }
        std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
            if (left.endedAtUtc != right.endedAtUtc) {
                return left.endedAtUtc > right.endedAtUtc;
            }
            return left.sessionId.toString(QUuid::WithoutBraces)
                > right.sessionId.toString(QUuid::WithoutBraces);
        });
        if (result.size() > limit) result.resize(limit);
        return result;
    }

    [[nodiscard]] model::FocusSessionWriteResult startFocusSessionAtomically(
        const model::FocusSession &session) override
    {
        ++m_startCalls;
        if (const auto forced = takeForcedStatus()) return {*forced};
        throwIfWriteFails();
        if (findActiveWithoutFailure().has_value()) {
            return {model::FocusSessionWriteStatus::ActiveSessionExists};
        }
        m_sessions.insert(session.sessionId, session);
        m_intervals.insert(session.sessionId,
                           {{session.sessionId,
                             0,
                             session.startedAtUtc,
                             std::nullopt,
                             session.startedAtUtc}});
        return {};
    }

    [[nodiscard]] model::FocusSessionWriteResult pauseFocusSessionAtomically(
        const model::FocusSessionId &sessionId,
        const model::FocusSessionState expectedState,
        const QDateTime &pausedAtUtc) override
    {
        ++m_pauseCalls;
        if (const auto forced = takeForcedStatus()) return {*forced};
        throwIfWriteFails();
        auto found = m_sessions.find(sessionId);
        if (found == m_sessions.end()) {
            return {model::FocusSessionWriteStatus::SessionNotFound};
        }
        if (found->state != expectedState
            || expectedState != model::FocusSessionState::Running) {
            return {model::FocusSessionWriteStatus::StateConflict};
        }
        auto &intervals = m_intervals[sessionId];
        if (intervals.isEmpty() || intervals.last().endedAtUtc.has_value()
            || pausedAtUtc < intervals.last().checkpointAtUtc) {
            return {model::FocusSessionWriteStatus::StateConflict};
        }
        intervals.last().endedAtUtc = pausedAtUtc;
        intervals.last().checkpointAtUtc = pausedAtUtc;
        found->state = model::FocusSessionState::Paused;
        return {};
    }

    [[nodiscard]] model::FocusSessionWriteResult resumeFocusSessionAtomically(
        const model::FocusSessionId &sessionId,
        const model::FocusSessionState expectedState,
        const QDateTime &resumedAtUtc) override
    {
        ++m_resumeCalls;
        if (const auto forced = takeForcedStatus()) return {*forced};
        throwIfWriteFails();
        auto found = m_sessions.find(sessionId);
        if (found == m_sessions.end()) {
            return {model::FocusSessionWriteStatus::SessionNotFound};
        }
        auto &intervals = m_intervals[sessionId];
        if (found->state != expectedState
            || expectedState != model::FocusSessionState::Paused
            || intervals.isEmpty() || !intervals.last().endedAtUtc.has_value()
            || resumedAtUtc < *intervals.last().endedAtUtc) {
            return {model::FocusSessionWriteStatus::StateConflict};
        }
        found->state = model::FocusSessionState::Running;
        intervals.append({sessionId,
                          static_cast<int>(intervals.size()),
                          resumedAtUtc,
                          std::nullopt,
                          resumedAtUtc});
        return {};
    }

    [[nodiscard]] model::FocusSessionWriteResult completeFocusSessionAtomically(
        const model::FocusSessionId &sessionId,
        const model::FocusSessionState expectedState,
        const QDateTime &completedAtUtc) override
    {
        ++m_completeCalls;
        if (const auto forced = takeForcedStatus()) return {*forced};
        throwIfWriteFails();
        auto found = m_sessions.find(sessionId);
        if (found == m_sessions.end()) {
            return {model::FocusSessionWriteStatus::SessionNotFound};
        }
        if (found->state != expectedState
            || (expectedState != model::FocusSessionState::Running
                && expectedState != model::FocusSessionState::Paused)) {
            return {model::FocusSessionWriteStatus::StateConflict};
        }
        auto &intervals = m_intervals[sessionId];
        if (expectedState == model::FocusSessionState::Running) {
            if (intervals.isEmpty() || intervals.last().endedAtUtc.has_value()
                || completedAtUtc < intervals.last().checkpointAtUtc) {
                return {model::FocusSessionWriteStatus::StateConflict};
            }
            intervals.last().endedAtUtc = completedAtUtc;
            intervals.last().checkpointAtUtc = completedAtUtc;
        }
        found->state = model::FocusSessionState::Completed;
        found->endedAtUtc = completedAtUtc;
        return {};
    }

    [[nodiscard]] model::FocusSessionWriteResult abandonFocusSessionAtomically(
        const model::FocusSessionId &sessionId,
        const model::FocusSessionState expectedState) override
    {
        ++m_abandonCalls;
        if (const auto forced = takeForcedStatus()) return {*forced};
        throwIfWriteFails();
        const auto found = m_sessions.find(sessionId);
        if (found == m_sessions.end()) {
            return {model::FocusSessionWriteStatus::SessionNotFound};
        }
        if (found->state != expectedState
            || (expectedState != model::FocusSessionState::Running
                && expectedState != model::FocusSessionState::Paused)) {
            return {model::FocusSessionWriteStatus::StateConflict};
        }
        m_sessions.erase(found);
        m_intervals.remove(sessionId);
        return {};
    }

    [[nodiscard]] model::FocusSessionWriteResult updateFocusCheckpointAtomically(
        const model::FocusSessionId &sessionId,
        const model::FocusSessionState expectedState,
        const QDateTime &checkpointAtUtc) override
    {
        ++m_checkpointCalls;
        if (m_checkpointFailuresRemaining > 0) {
            --m_checkpointFailuresRemaining;
            throw model::RepositoryException("Injected checkpoint failure.");
        }
        if (const auto forced = takeForcedStatus()) return {*forced};
        throwIfWriteFails();
        auto found = m_sessions.find(sessionId);
        auto &intervals = m_intervals[sessionId];
        if (found == m_sessions.end()) {
            return {model::FocusSessionWriteStatus::SessionNotFound};
        }
        if (found->state != expectedState
            || expectedState != model::FocusSessionState::Running
            || intervals.isEmpty() || intervals.last().endedAtUtc.has_value()
            || checkpointAtUtc < intervals.last().checkpointAtUtc) {
            return {model::FocusSessionWriteStatus::StateConflict};
        }
        intervals.last().checkpointAtUtc = checkpointAtUtc;
        return {};
    }

    [[nodiscard]] model::FocusSessionWriteResult
    recoverRunningFocusSessionAtomically() override
    {
        ++m_recoverCalls;
        if (const auto forced = takeForcedStatus()) return {*forced};
        throwIfWriteFails();
        const auto active = findActiveWithoutFailure();
        if (!active.has_value() || active->state != model::FocusSessionState::Running) {
            return {};
        }
        auto found = m_sessions.find(active->sessionId);
        auto &intervals = m_intervals[active->sessionId];
        if (intervals.isEmpty() || intervals.last().endedAtUtc.has_value()) {
            return {model::FocusSessionWriteStatus::StateConflict};
        }
        intervals.last().endedAtUtc = intervals.last().checkpointAtUtc;
        found->state = model::FocusSessionState::Paused;
        return {};
    }

    void seed(model::FocusSession session, QList<model::FocusInterval> intervals)
    {
        m_intervals.insert(session.sessionId, std::move(intervals));
        m_sessions.insert(session.sessionId, std::move(session));
    }
    void setReadFailure(const bool enabled) noexcept { m_failReads = enabled; }
    void setWriteFailure(const bool enabled) noexcept { m_failWrites = enabled; }
    void setCheckpointFailures(const int count) noexcept
    {
        m_checkpointFailuresRemaining = std::max(0, count);
    }
    void forceNextStatus(const model::FocusSessionWriteStatus status)
    {
        m_forcedStatus = status;
    }

    [[nodiscard]] int checkpointCalls() const noexcept { return m_checkpointCalls; }
    [[nodiscard]] int recoverCalls() const noexcept { return m_recoverCalls; }
    [[nodiscard]] int abandonCalls() const noexcept { return m_abandonCalls; }
    [[nodiscard]] int pauseCalls() const noexcept { return m_pauseCalls; }
    [[nodiscard]] int completeCalls() const noexcept { return m_completeCalls; }

private:
    [[nodiscard]] std::optional<model::FocusSession> findActiveWithoutFailure() const
    {
        for (const auto &session : m_sessions) {
            if (session.state == model::FocusSessionState::Running
                || session.state == model::FocusSessionState::Paused) {
                return session;
            }
        }
        return std::nullopt;
    }
    [[nodiscard]] std::optional<model::FocusSessionWriteStatus> takeForcedStatus()
    {
        const auto status = m_forcedStatus;
        m_forcedStatus.reset();
        return status;
    }
    void throwIfReadFails() const
    {
        if (m_failReads) throw model::RepositoryException("Fake focus read failure.");
    }
    void throwIfWriteFails() const
    {
        if (m_failWrites) throw model::RepositoryException("Fake focus write failure.");
    }

    QHash<model::FocusSessionId, model::FocusSession> m_sessions;
    QHash<model::FocusSessionId, QList<model::FocusInterval>> m_intervals;
    std::optional<model::FocusSessionWriteStatus> m_forcedStatus;
    int m_checkpointFailuresRemaining{0};
    int m_startCalls{0};
    int m_pauseCalls{0};
    int m_resumeCalls{0};
    int m_completeCalls{0};
    int m_abandonCalls{0};
    int m_checkpointCalls{0};
    int m_recoverCalls{0};
    bool m_failReads{false};
    bool m_failWrites{false};
};

} // namespace smartmate::tests
