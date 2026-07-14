#pragma once

#include <QWidget>

class QButtonGroup;
class QComboBox;

namespace smartmate::viewmodel {
class AppearanceSettingsContract;
}

namespace smartmate::view::widgets {

/// 外观设置页只绑定 AppearanceSettingsContract，不读取具体 ViewModel 兼容 API。
class SettingsPage final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(viewmodel::AppearanceSettingsContract &settings,
                          QWidget *parent = nullptr);

private:
    viewmodel::AppearanceSettingsContract &m_settings;
    QButtonGroup *m_accentButtons;
    QComboBox *m_fontFamilyComboBox;
    QButtonGroup *m_fontScaleButtons;
};

} // namespace smartmate::view::widgets
