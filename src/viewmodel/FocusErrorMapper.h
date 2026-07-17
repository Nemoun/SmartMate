#pragma once

#include "services/FocusResult.h"

#include <QString>

namespace smartmate::viewmodel {

/// 将稳定 FocusError 映射为用户文案，不解析 Repository 技术 detail。
[[nodiscard]] QString focusErrorMessage(model::FocusError error);

} // namespace smartmate::viewmodel
