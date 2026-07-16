#pragma once

#include "repositories/IAppearanceSettingsRepository.h"

#include <QString>

#include <memory>

class QSettings;

namespace smartmate::model::persistence {

/// 使用 QSettings 保存轻量外观偏好；空文件路径使用应用组织与名称。
/// 该 Adapter 不继承 QObject，也不发送通知；成功结果经 Service/ViewModel 转为 Contract 绑定。
class QSettingsAppearanceRepository final
    : public IAppearanceSettingsRepository {
public:
    /// iniFilePath 为空时使用应用默认设置域；显式路径主要供隔离测试使用。
    explicit QSettingsAppearanceRepository(QString iniFilePath = {});
    /// 在实现文件中析构，以允许头文件仅前置声明 QSettings。
    ~QSettingsAppearanceRepository() override;

    /// 将稳定存储文本转换为领域枚举；缺失键使用兼容默认值。
    [[nodiscard]] AppearanceSettings load() const override;
    /// 写入全部外观键并立即 sync；同步失败抛出 RepositoryException。
    void save(const AppearanceSettings &settings) override;

private:
    /// 独占 QSettings 实例；与 Repository 同生命周期，不可为空。
    std::unique_ptr<QSettings> m_settings;
};

} // namespace smartmate::model::persistence
