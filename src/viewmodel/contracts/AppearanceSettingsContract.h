#pragma once

#include "common/presentation/UiNotification.h"

#include <QObject>
#include <QStringList>

namespace smartmate::viewmodel {

/// 外观设置 ViewModel 向 View 公开的稳定展示契约。
///
/// Widget 只依赖本抽象类型：先读取 getter 同步当前外观，再监听 appearanceChanged()。
/// 三个可写索引构成显式双向绑定，setter 表示用户命令，具体实现负责持久化与错误映射。
class AppearanceSettingsContract : public QObject {
    Q_OBJECT
    // 可写属性把 Widget 选择转发为强类型 setter；统一通知保证相关派生外观一起刷新。
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

    // 当前选择索引、固定选项及派生字体参数只提供展示读取，不暴露 Model 领域枚举。
    [[nodiscard]] virtual int accentThemeIndex() const noexcept = 0;
    [[nodiscard]] virtual int fontFamilyIndex() const noexcept = 0;
    [[nodiscard]] virtual int fontScaleIndex() const noexcept = 0;
    [[nodiscard]] virtual QStringList accentThemeOptions() const = 0;
    [[nodiscard]] virtual QStringList fontFamilyOptions() const = 0;
    [[nodiscard]] virtual QStringList fontScaleOptions() const = 0;
    [[nodiscard]] virtual QString fontFamilyName() const = 0;
    [[nodiscard]] virtual qreal fontScale() const noexcept = 0;
    /// 用户选择主题的命令；越界和持久化失败由具体 ViewModel 处理。
    virtual void setAccentThemeIndex(int index) = 0;
    /// 用户选择字体的命令。
    virtual void setFontFamilyIndex(int index) = 0;
    /// 用户选择缩放级别的命令。
    virtual void setFontScaleIndex(int index) = 0;

public slots:
    /// 恢复并保存全部默认外观，而不是由 Widget 逐项设置属性。
    virtual void resetDefaults() = 0;

signals:
    /// 任一外观值实际变化后发送；绑定方收到后重新读取全部相关 getter。
    void appearanceChanged();
    /// 请求 View 展示一次性通知；只携带展示 DTO，不允许 Contract 操纵 QMessageBox。
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    /// 仅允许具体 ViewModel 派生构造，确保对象只有一个 QObject 基类。
    explicit AppearanceSettingsContract(QObject *parent = nullptr)
        : QObject(parent)
    {
    }
};

} // namespace smartmate::viewmodel
