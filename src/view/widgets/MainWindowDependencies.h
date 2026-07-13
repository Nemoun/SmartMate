#pragma once

#include "viewmodel/contracts/AppearanceSettingsContract.h"

namespace smartmate::view::widgets {

/// 主窗口当前纵向切片所需的最小依赖集合。
///
/// 组合根负责保证所有 Contract 的生命周期长于 MainWindow；后续页面迁移时
/// 只按实际使用增加新的窄 Contract，不提前注入具体 ViewModel。
struct MainWindowDependencies {
    viewmodel::AppearanceSettingsContract &appearanceSettings;
};

} // namespace smartmate::view::widgets
