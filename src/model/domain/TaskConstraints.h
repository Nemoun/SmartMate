#pragma once

#include <QtTypes>

namespace smartmate::model::TaskConstraints {

/// 任务标题允许的最大 Unicode code point 数量。
inline constexpr qsizetype maximumTitleLength = 200;
/// 任务描述允许的最大 Unicode code point 数量。
inline constexpr qsizetype maximumDescriptionLength = 5000;
/// 预计用时的领域边界，所有输入方式和业务校验必须引用同一组常量。
inline constexpr int minimumEstimatedMinutes = 1;
/// 最大值为 365 天，防止无意义的大数进入输入、显示和持久化流程。
inline constexpr int maximumEstimatedMinutes = 525600;

} // namespace smartmate::model::TaskConstraints
