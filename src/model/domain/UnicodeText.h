#pragma once

#include <QString>

namespace smartmate::model {

/// 返回文本包含的 Unicode code point 数量，避免把补充平面字符计为两个 UTF-16 单元。
[[nodiscard]] inline qsizetype unicodeCodePointCount(const QString &text)
{
    return text.toUcs4().size();
}

} // namespace smartmate::model
