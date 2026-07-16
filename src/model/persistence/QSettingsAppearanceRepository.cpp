#include "QSettingsAppearanceRepository.h"

#include "repositories/RepositoryException.h"

#include <QSettings>

#include <utility>

namespace smartmate::model::persistence {
namespace {
// 键名和英文枚举文本属于持久化格式，不能随界面中文文案或枚举显示顺序变化。
constexpr auto accentKey = "appearance/accent";
constexpr auto fontFamilyKey = "appearance/fontFamily";
constexpr auto fontScaleKey = "appearance/fontScale";

[[nodiscard]] AccentTheme accentFromString(const QString &value)
{
    // 未识别的旧值安全回退 Green；领域 Service 仍会复核完整设置是否合法。
    return value == QStringLiteral("blue") ? AccentTheme::Blue
                                            : AccentTheme::Green;
}

[[nodiscard]] UiFontFamily familyFromString(const QString &value)
{
    if (value == QStringLiteral("microsoft-yahei-ui")) {
        return UiFontFamily::MicrosoftYaHeiUI;
    }
    if (value == QStringLiteral("dengxian")) {
        return UiFontFamily::DengXian;
    }
    return UiFontFamily::System;
}

[[nodiscard]] UiFontScale scaleFromString(const QString &value)
{
    if (value == QStringLiteral("small")) {
        return UiFontScale::Small;
    }
    if (value == QStringLiteral("large")) {
        return UiFontScale::Large;
    }
    return UiFontScale::Standard;
}

[[nodiscard]] QString accentString(const AccentTheme value)
{
    return value == AccentTheme::Blue ? QStringLiteral("blue")
                                      : QStringLiteral("green");
}

[[nodiscard]] QString familyString(const UiFontFamily value)
{
    switch (value) {
    case UiFontFamily::MicrosoftYaHeiUI:
        return QStringLiteral("microsoft-yahei-ui");
    case UiFontFamily::DengXian:
        return QStringLiteral("dengxian");
    case UiFontFamily::System:
        return QStringLiteral("system");
    }
    return QStringLiteral("system");
}

[[nodiscard]] QString scaleString(const UiFontScale value)
{
    switch (value) {
    case UiFontScale::Small:
        return QStringLiteral("small");
    case UiFontScale::Large:
        return QStringLiteral("large");
    case UiFontScale::Standard:
        return QStringLiteral("standard");
    }
    return QStringLiteral("standard");
}
}

QSettingsAppearanceRepository::QSettingsAppearanceRepository(QString iniFilePath)
{
    if (iniFilePath.isEmpty()) {
        // 正式程序使用 QApplication 配置的组织名和应用名定位平台设置存储。
        m_settings = std::make_unique<QSettings>();
    } else {
        // 测试注入独立 INI 文件，避免读写用户真实偏好。
        m_settings = std::make_unique<QSettings>(std::move(iniFilePath),
                                                 QSettings::IniFormat);
    }
}

QSettingsAppearanceRepository::~QSettingsAppearanceRepository() = default;

AppearanceSettings QSettingsAppearanceRepository::load() const
{
    // Repository 只返回领域快照；它不缓存 ViewModel 状态，也不触发属性绑定通知。
    if (m_settings->status() != QSettings::NoError) {
        throw RepositoryException{"Unable to read appearance settings."};
    }
    return {
        accentFromString(m_settings->value(accentKey, QStringLiteral("green")).toString()),
        familyFromString(m_settings->value(fontFamilyKey, QStringLiteral("system")).toString()),
        scaleFromString(m_settings->value(fontScaleKey, QStringLiteral("standard")).toString()),
    };
}

void QSettingsAppearanceRepository::save(const AppearanceSettings &settings)
{
    // 三个键构成一个完整设置快照；参数来自 Service 校验后的领域值。
    m_settings->setValue(accentKey, accentString(settings.accentTheme));
    m_settings->setValue(fontFamilyKey, familyString(settings.fontFamily));
    m_settings->setValue(fontScaleKey, scaleString(settings.fontScale));
    m_settings->sync();
    if (m_settings->status() != QSettings::NoError) {
        throw RepositoryException{"Unable to save appearance settings."};
    }
    // 只有 sync 成功才返回；随后由 ViewModel 更新 Contract 并发送 appearanceChanged()。
}

} // namespace smartmate::model::persistence
