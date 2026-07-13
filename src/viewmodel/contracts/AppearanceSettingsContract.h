#pragma once

#include "common/presentation/UiNotification.h"

#include <QObject>
#include <QStringList>

namespace smartmate::viewmodel {

/// 外观设置 ViewModel 向 View 公开的稳定展示契约。
class AppearanceSettingsContract : public QObject {
    Q_OBJECT
    Q_PROPERTY(int accentThemeIndex READ accentThemeIndex WRITE setAccentThemeIndex
                   NOTIFY appearanceChanged)
    Q_PROPERTY(int fontFamilyIndex READ fontFamilyIndex WRITE setFontFamilyIndex
                   NOTIFY appearanceChanged)
    Q_PROPERTY(int fontScaleIndex READ fontScaleIndex WRITE setFontScaleIndex
                   NOTIFY appearanceChanged)
    Q_PROPERTY(QStringList accentThemeOptions READ accentThemeOptions CONSTANT)
    Q_PROPERTY(QStringList fontFamilyOptions READ fontFamilyOptions CONSTANT)
    Q_PROPERTY(QStringList fontScaleOptions READ fontScaleOptions CONSTANT)
    Q_PROPERTY(QString fontFamilyName READ fontFamilyName NOTIFY appearanceChanged)
    Q_PROPERTY(qreal fontScale READ fontScale NOTIFY appearanceChanged)

public:
    ~AppearanceSettingsContract() override = default;

    [[nodiscard]] virtual int accentThemeIndex() const noexcept = 0;
    [[nodiscard]] virtual int fontFamilyIndex() const noexcept = 0;
    [[nodiscard]] virtual int fontScaleIndex() const noexcept = 0;
    [[nodiscard]] virtual QStringList accentThemeOptions() const = 0;
    [[nodiscard]] virtual QStringList fontFamilyOptions() const = 0;
    [[nodiscard]] virtual QStringList fontScaleOptions() const = 0;
    [[nodiscard]] virtual QString fontFamilyName() const = 0;
    [[nodiscard]] virtual qreal fontScale() const noexcept = 0;
    virtual void setAccentThemeIndex(int index) = 0;
    virtual void setFontFamilyIndex(int index) = 0;
    virtual void setFontScaleIndex(int index) = 0;

public slots:
    virtual void resetDefaults() = 0;

signals:
    void appearanceChanged();
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    explicit AppearanceSettingsContract(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
};

} // namespace smartmate::viewmodel
