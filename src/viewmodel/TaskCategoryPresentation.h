#pragma once

#include "domain/TaskCategory.h"

#include <QString>
#include <QStringList>

#include <optional>

namespace smartmate::viewmodel {

/// 将领域颜色映射为稳定的界面调色板索引；不参与任何类别业务判断。
[[nodiscard]] int taskCategoryColorIndex(model::TaskCategoryColor color) noexcept;

/// 返回类别徽标使用的强调色；颜色只属于展示投影，不写入任务或依赖图。
[[nodiscard]] QString taskCategoryAccent(model::TaskCategoryColor color);

/// 返回描述表定义的固定调色板中文名称，不依赖领域枚举序号。
[[nodiscard]] QStringList taskCategoryColorOptions();

/// 将界面调色板索引转换为领域枚举；非法索引返回空值。
[[nodiscard]] std::optional<model::TaskCategoryColor> taskCategoryColorFromIndex(
    int index) noexcept;

/// 未分类任务和类别选项使用的统一中性色。
[[nodiscard]] QString taskUncategorizedAccent();
/// “全部类别”筛选项使用的统一中性色。
[[nodiscard]] QString taskAllCategoriesAccent();

} // namespace smartmate::viewmodel
