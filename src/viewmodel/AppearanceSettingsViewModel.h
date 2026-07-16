#pragma once

#include "domain/AppearanceSettings.h"
#include "viewmodel/contracts/AppearanceSettingsContract.h"


namespace smartmate::model {
class AppearanceSettingsService;
}

namespace smartmate::viewmodel {

/// 将外观偏好投影为有限索引并即时保存；不暴露 QSettings。
/// setter 先形成完整候选快照，Service 保存成功后才替换可观察状态并通知 Widget。
class AppearanceSettingsViewModel : public AppearanceSettingsContract {
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
public:
    /// 无 Service 构造仅供隔离测试，使用会话默认值且不持久化。
    explicit AppearanceSettingsViewModel(QObject *parent = nullptr);
    /// 生产构造注入 Service，并立即加载当前持久化设置。
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
    /// errorMessage 属性实际变化后发送。
    void errorMessageChanged();
    /// 兼容旧调用方的错误流程信号；新 Widget 优先监听 notificationRaised()。
    void errorOccurred(const QString &message);

private:
    /// 从 Service 加载初始快照；失败保留默认值并发布展示通知。
    void load();
    /// 幂等替换设置并统一发送 appearanceChanged()。
    void apply(const model::AppearanceSettings &settings);
    /// 保存完整候选；失败时不污染当前 getter 可见状态。
    void saveCandidate(const model::AppearanceSettings &candidate);
    /// 去重错误属性通知，并把非空错误转换为 UiNotification。
    void setError(const QString &message);

    /// 测试可不注入服务而使用会话默认值；生产环境始终注入。
    model::AppearanceSettingsService *m_service{nullptr};
    /// 当前已确认设置快照，是全部外观 getter 的唯一数据源。
    model::AppearanceSettings m_settings;
    /// 最近一次展示错误；空字符串表示当前无错误。
    QString m_errorMessage;
};

} // namespace smartmate::viewmodel
