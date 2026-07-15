#pragma once

#include "domain/AppearanceSettings.h"

namespace smartmate::model {

/// 外观偏好的持久化端口；接口不暴露 QSettings 或其他存储类型。
/// 仅由 Model Service 调用，不提供 Qt 信号或 Widget 数据绑定；保存成功返回后，
/// 具体 ViewModel 才能更新 Contract 状态并发布 appearanceChanged()。
class IAppearanceSettingsRepository {
public:
    virtual ~IAppearanceSettingsRepository() = default;

    /// 读取完整设置快照；不存在的键由具体实现提供兼容默认值，故障抛出 RepositoryException。
    [[nodiscard]] virtual AppearanceSettings load() const = 0;
    /// 覆盖保存完整设置快照；只有持久化同步成功才可正常返回。
    virtual void save(const AppearanceSettings &settings) = 0;
};

} // namespace smartmate::model
