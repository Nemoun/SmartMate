#pragma once

#include "services/TaskResult.h"

#include <QString>

namespace smartmate::viewmodel {

/// 将与界面语言无关的领域错误翻译为用户可读的中文展示文本。
/// 只做文案映射，不决定命令成功与否；未知错误使用安全兜底文案。
[[nodiscard]] QString taskErrorMessage(model::TaskError error);

} // namespace smartmate::viewmodel
