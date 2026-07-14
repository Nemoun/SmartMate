#pragma once

#include "domain/TaskCategory.h"

#include <QList>
#include <QString>

#include <optional>
#include <utility>

namespace smartmate::model {

/// 类别业务错误使用独立枚举，避免 ViewModel 解析 Repository 技术文本。
enum class TaskCategoryError {
    /// 操作成功。
    None,
    /// 规范化后的类别名称为空。
    EmptyName,
    /// 类别名称超过领域允许长度。
    NameTooLong,
    /// 已存在规范化后同名的类别。
    DuplicateName,
    /// 颜色值不符合领域格式。
    InvalidColor,
    /// 指定稳定 TaskCategoryId 不存在或在写入前消失。
    NotFound,
    /// Repository 操作失败。
    PersistenceFailure,
};

/// 类别 Service 的结构化结果；成功时必须同时拥有 value 且 error 为 None。
template<typename T>
struct TaskCategoryServiceResult final {
    /// 成功时的领域快照；失败时为空。
    std::optional<T> value;
    /// 供 ViewModel 稳定映射的业务错误。
    TaskCategoryError error{TaskCategoryError::None};
    /// 仅用于诊断的补充信息。
    QString detail;

    [[nodiscard]] bool ok() const noexcept
    {
        return error == TaskCategoryError::None && value.has_value();
    }

    [[nodiscard]] static TaskCategoryServiceResult success(T resultValue)
    {
        TaskCategoryServiceResult result;
        result.value.emplace(std::move(resultValue));
        return result;
    }

    [[nodiscard]] static TaskCategoryServiceResult failure(
        TaskCategoryError resultError,
        QString resultDetail = {})
    {
        TaskCategoryServiceResult result;
        result.error = resultError;
        result.detail = std::move(resultDetail);
        return result;
    }
};

/// 删除成功后返回被删除实体和解除归属的任务数量，供确认结果投影。
struct TaskCategoryDeletionOutcome final {
    /// 删除前的类别快照，供调用方展示操作结果。
    TaskCategory category;
    /// 本次原子删除中被转为“未分类”的任务数量。
    int unassignedTaskCount{0};
};

using TaskCategoryResult = TaskCategoryServiceResult<TaskCategory>;
using TaskCategoryListResult = TaskCategoryServiceResult<QList<TaskCategory>>;
using TaskCategoryDeletionResult =
    TaskCategoryServiceResult<TaskCategoryDeletionOutcome>;

} // namespace smartmate::model
