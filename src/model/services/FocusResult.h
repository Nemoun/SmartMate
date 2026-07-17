#pragma once

#include "domain/FocusSession.h"

#include <QList>
#include <QMetaType>
#include <QString>

#include <optional>
#include <utility>

namespace smartmate::model {

/// FocusService 对外暴露的稳定错误；Repository 技术细节只保存在 detail。
enum class FocusError : int {
    None = 0,
    NotInitialized,
    TaskNotFound,
    TaskNotInProgress,
    ActiveSessionExists,
    SessionNotFound,
    StateConflict,
    MinimumDurationNotReached,
    PersistenceFailure,
};

/// 一次会话及其原始时间段在查询时刻的 Model 聚合结果。
struct FocusRecord final {
    FocusSession session;
    QList<FocusInterval> intervals;
    /// 已扣除暂停时间的累计毫秒数；数据库不保存该派生值。
    qint64 focusedMilliseconds{0};

    friend bool operator==(const FocusRecord &, const FocusRecord &) = default;
};

/// 专注页面未来所需的完整只读快照；最近记录只包含 Completed 会话。
struct FocusSnapshot final {
    std::optional<FocusRecord> activeRecord;
    QList<FocusRecord> recentCompletedRecords;

    friend bool operator==(const FocusSnapshot &, const FocusSnapshot &) = default;
};

/// 不携带业务值的初始化、检查点和退出结果。
struct FocusOperationResult final {
    FocusError error{FocusError::None};
    QString detail;

    [[nodiscard]] bool ok() const noexcept { return error == FocusError::None; }
    [[nodiscard]] static FocusOperationResult success() { return {}; }
    [[nodiscard]] static FocusOperationResult failure(FocusError resultError,
                                                       QString resultDetail = {})
    {
        return {resultError, std::move(resultDetail)};
    }
};

template<typename T>
struct FocusValueResult final {
    std::optional<T> value;
    FocusError error{FocusError::None};
    QString detail;

    [[nodiscard]] bool ok() const noexcept
    {
        return error == FocusError::None && value.has_value();
    }
    [[nodiscard]] static FocusValueResult success(T resultValue)
    {
        FocusValueResult result;
        result.value.emplace(std::move(resultValue));
        return result;
    }
    [[nodiscard]] static FocusValueResult failure(FocusError resultError,
                                                   QString resultDetail = {})
    {
        FocusValueResult result;
        result.error = resultError;
        result.detail = std::move(resultDetail);
        return result;
    }
};

using FocusRecordResult = FocusValueResult<FocusRecord>;
using FocusSnapshotResult = FocusValueResult<FocusSnapshot>;

} // namespace smartmate::model

Q_DECLARE_METATYPE(smartmate::model::FocusError)
