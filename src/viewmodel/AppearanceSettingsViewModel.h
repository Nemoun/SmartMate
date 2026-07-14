#pragma once

#include "domain/AppearanceSettings.h"
#include "viewmodel/contracts/AppearanceSettingsContract.h"


namespace smartmate::model {
class AppearanceSettingsService;
}

namespace smartmate::viewmodel {

/// 将外观偏好投影为有限索引并即时保存；不暴露 QSettings。
class AppearanceSettingsViewModel : public AppearanceSettingsContract {
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
public:
    explicit AppearanceSettingsViewModel(QObject *parent = nullptr);
    explicit AppearanceSettingsViewModel(model::AppearanceSettingsService &service,
                                         QObject *parent = nullptr);

    [[nodiscard]] int accentThemeIndex() const noexcept override;
    [[nodiscard]] int fontFamilyIndex() const noexcept override;
    [[nodiscard]] int fontScaleIndex() const noexcept override;
    [[nodiscard]] QStringList accentThemeOptions() const override;
    [[nodiscard]] QStringList fontFamilyOptions() const override;
    [[nodiscard]] QStringList fontScaleOptions() const override;
    [[nodiscard]] QString fontFamilyName() const override;
    [[nodiscard]] qreal fontScale() const noexcept override;
    [[nodiscard]] QString errorMessage() const;

    void setAccentThemeIndex(int index) override;
    void setFontFamilyIndex(int index) override;
    void setFontScaleIndex(int index) override;
    void resetDefaults() override;
    Q_INVOKABLE void clearError();

signals:
    void errorMessageChanged();
    void errorOccurred(const QString &message);

private:
    void load();
    void apply(const model::AppearanceSettings &settings);
    void saveCandidate(const model::AppearanceSettings &candidate);
    void setError(const QString &message);

    /// 测试可不注入服务而使用会话默认值；生产环境始终注入。
    model::AppearanceSettingsService *m_service{nullptr};
    model::AppearanceSettings m_settings;
    QString m_errorMessage;
};

} // namespace smartmate::viewmodel
