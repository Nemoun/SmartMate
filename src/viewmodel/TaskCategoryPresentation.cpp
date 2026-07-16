#include "TaskCategoryPresentation.h"

#include <algorithm>
#include <array>

namespace smartmate::viewmodel {
namespace {

struct TaskCategoryColorDescriptor final {
    model::TaskCategoryColor value;
    QString label;
    QString accent;
};

const auto &colorDescriptors()
{
    static const std::array<TaskCategoryColorDescriptor, 8> descriptors{{
        {model::TaskCategoryColor::Blue, QStringLiteral("蓝色"),
         QStringLiteral("#2563eb")},
        {model::TaskCategoryColor::Teal, QStringLiteral("青色"),
         QStringLiteral("#0f766e")},
        {model::TaskCategoryColor::Green, QStringLiteral("绿色"),
         QStringLiteral("#15803d")},
        {model::TaskCategoryColor::Amber, QStringLiteral("琥珀色"),
         QStringLiteral("#b45309")},
        {model::TaskCategoryColor::Orange, QStringLiteral("橙色"),
         QStringLiteral("#c2410c")},
        {model::TaskCategoryColor::Rose, QStringLiteral("玫红色"),
         QStringLiteral("#be123c")},
        {model::TaskCategoryColor::Violet, QStringLiteral("紫色"),
         QStringLiteral("#7c3aed")},
        {model::TaskCategoryColor::Slate, QStringLiteral("灰蓝色"),
         QStringLiteral("#475569")},
    }};
    return descriptors;
}

} // namespace

int taskCategoryColorIndex(const model::TaskCategoryColor color) noexcept
{
    const auto &descriptors = colorDescriptors();
    const auto iterator = std::find_if(
        descriptors.cbegin(), descriptors.cend(),
        [color](const TaskCategoryColorDescriptor &descriptor) {
            return descriptor.value == color;
        });
    return iterator == descriptors.cend()
        ? 0 : static_cast<int>(std::distance(descriptors.cbegin(), iterator));
}

QString taskCategoryAccent(const model::TaskCategoryColor color)
{
    const auto &descriptors = colorDescriptors();
    const auto iterator = std::find_if(
        descriptors.cbegin(), descriptors.cend(),
        [color](const TaskCategoryColorDescriptor &descriptor) {
            return descriptor.value == color;
        });
    return iterator == descriptors.cend() ? descriptors.back().accent
                                           : iterator->accent;
}

QStringList taskCategoryColorOptions()
{
    QStringList result;
    result.reserve(static_cast<qsizetype>(colorDescriptors().size()));
    for (const auto &descriptor : colorDescriptors()) {
        result.append(descriptor.label);
    }
    return result;
}

std::optional<model::TaskCategoryColor> taskCategoryColorFromIndex(
    const int index) noexcept
{
    const auto &descriptors = colorDescriptors();
    if (index < 0 || index >= static_cast<int>(descriptors.size())) {
        return std::nullopt;
    }
    return descriptors.at(static_cast<std::size_t>(index)).value;
}

QString taskUncategorizedAccent()
{
    return QStringLiteral("#94a3b8");
}

QString taskAllCategoriesAccent()
{
    return QStringLiteral("#64748b");
}

} // namespace smartmate::viewmodel
