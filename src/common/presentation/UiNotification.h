#pragma once

#include <QMetaType>
#include <QString>

namespace smartmate::common {

/// 展示级通知的严重程度；不承载业务错误分类或领域状态。
enum class UiSeverity {
    Information,
    Warning,
    Error,
};

/// ViewModel 发给 View 的结构化展示消息。
///
/// View 决定使用状态栏、对话框或其他控件呈现；该值类型不包含任何 View 控制语义。
struct UiNotification {
    UiSeverity severity{UiSeverity::Information};
    QString title;
    QString message;

    bool operator==(const UiNotification &) const = default;
};

} // namespace smartmate::common

Q_DECLARE_METATYPE(smartmate::common::UiSeverity)
Q_DECLARE_METATYPE(smartmate::common::UiNotification)
