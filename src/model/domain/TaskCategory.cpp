#include "domain/TaskCategory.h"

namespace smartmate::model {

QString taskCategoryNameKey(const QString &name)
{
    // NFKC 同时合并兼容字符，随后进行Unicode大小写折叠，保证界面预检与唯一索引一致。
    return name.trimmed()
        .normalized(QString::NormalizationForm_KC)
        .toCaseFolded();
}

bool isValidTaskCategoryColor(const TaskCategoryColor color) noexcept
{
    switch (color) {
    case TaskCategoryColor::Blue:
    case TaskCategoryColor::Teal:
    case TaskCategoryColor::Green:
    case TaskCategoryColor::Amber:
    case TaskCategoryColor::Orange:
    case TaskCategoryColor::Rose:
    case TaskCategoryColor::Violet:
    case TaskCategoryColor::Slate:
        return true;
    }
    return false;
}

} // namespace smartmate::model
