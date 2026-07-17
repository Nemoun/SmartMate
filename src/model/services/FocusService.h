#pragma once

#include "repositories/IFocusSessionRepository.h"
#include "repositories/ITaskActivityRepository.h"
#include "repositories/ITaskCategoryRepository.h"
#include "repositories/ITaskRepository.h"
#include "services/FocusResult.h"

#include <QObject>
#include <QTimer>

#include <functional>

namespace smartmate::model {

/// 自由正计时专注的唯一业务入口；不拥有注入的 Repository。
///
/// Service 负责状态资格、累计时长、最短完成时长、检查点和生命周期恢复。
/// ViewModel 只能消费结果并映射展示，不得重新汇总 FocusInterval。
class FocusService final : public QObject {
    Q_OBJECT

public:
    using NowProvider = std::function<QDateTime()>;
    static constexpr int defaultCheckpointIntervalMs = 5'000;
    static constexpr qint64 minimumCompletedDurationMs = 1'000;

    FocusService(ITaskRepository &taskRepository,
                 ITaskCategoryRepository &categoryRepository,
                 ITaskActivityRepository &activityRepository,
                 IFocusSessionRepository &focusRepository,
                 QObject *parent = nullptr);
    /// 测试构造允许固定 UTC 时钟并缩短检查点周期；正式组合根使用默认构造。
    FocusService(ITaskRepository &taskRepository,
                 ITaskCategoryRepository &categoryRepository,
                 ITaskActivityRepository &activityRepository,
                 IFocusSessionRepository &focusRepository,
                 NowProvider nowProvider,
                 int checkpointIntervalMs,
                 QObject *parent = nullptr);

    /// 恢复异常 Running 会话并清理不足一秒的遗留活动会话；可重复调用。
    [[nodiscard]] FocusOperationResult initialize();
    /// 返回开始候选、权威命令资格、活动会话与最近完成记录；limit 非正时历史列表为空。
    [[nodiscard]] FocusSnapshotResult snapshot(int limit = 50) const;

    [[nodiscard]] FocusRecordResult startFocus(const TaskId &taskId);
    [[nodiscard]] FocusRecordResult pauseFocus(const FocusSessionId &sessionId);
    [[nodiscard]] FocusRecordResult resumeFocus(const FocusSessionId &sessionId);
    [[nodiscard]] FocusRecordResult completeFocus(const FocusSessionId &sessionId);
    /// 成功值是删除前最后一份记录，持久化层不会保留该会话。
    [[nodiscard]] FocusRecordResult abandonFocus(const FocusSessionId &sessionId);

    /// 正常退出前暂停有效 Running 会话，并放弃累计不足一秒的活动会话。
    [[nodiscard]] FocusOperationResult prepareForShutdown();
    /// 立即写入一次检查点，主要用于生命周期编排和确定性测试。
    [[nodiscard]] FocusOperationResult writeCheckpoint();

signals:
    /// 活动会话的身份、状态或存在性发生变化。
    void focusChanged();
    /// 完成命令新增了一条历史记录。
    void historyChanged();
    /// 持续后台故障只发送一次；detail 仅用于日志和后续 ViewModel 映射。
    void backgroundFailureRaised(FocusError error, const QString &detail);
    /// 后续成功持久化已经恢复检查点可靠性。
    void backgroundFailureCleared();

private slots:
    void handleCheckpointTimeout();

private:
    [[nodiscard]] FocusRecord loadRecord(const FocusSession &session,
                                         const QDateTime &nowUtc) const;
    [[nodiscard]] FocusOperationResult requireInitialized() const;
    [[nodiscard]] QDateTime nowUtc() const;
    [[nodiscard]] FocusError mapWriteStatus(FocusSessionWriteStatus status) const;
    void startCheckpointTimer(const FocusSessionId &sessionId);
    void stopCheckpointTimer();
    void raiseBackgroundFailure(FocusError error, const QString &detail);
    void clearBackgroundFailure();

    ITaskRepository &m_taskRepository;
    ITaskCategoryRepository &m_categoryRepository;
    ITaskActivityRepository &m_activityRepository;
    IFocusSessionRepository &m_focusRepository;
    NowProvider m_nowProvider;
    QTimer m_checkpointTimer;
    FocusSessionId m_runningSessionId;
    bool m_initialized{false};
    bool m_shutdownPrepared{false};
    bool m_backgroundFailureActive{false};
};

} // namespace smartmate::model
