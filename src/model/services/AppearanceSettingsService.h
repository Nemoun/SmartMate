#pragma once

#include "domain/AppearanceSettings.h"
#include "repositories/IAppearanceSettingsRepository.h"

#include <QString>

#include <optional>

namespace smartmate::model {

/// 外观设置用例的稳定错误分类，供 ViewModel 映射为展示文案。
enum class AppearanceSettingsError {
    /// 操作成功。
    None,
    /// 待保存设置不满足领域取值约束。
    InvalidValue,
    /// Repository 读取或写入失败。
    PersistenceFailure,
};

/// 外观设置用例结果；成功时 value 有值，失败时通过 error/detail 说明原因。
struct AppearanceSettingsResult final {
    /// 成功返回的设置快照；失败时为空。
    std::optional<AppearanceSettings> value;
    /// 机器可读错误，ViewModel 不需要解析技术文本。
    AppearanceSettingsError error{AppearanceSettingsError::None};
    /// 面向日志与诊断的技术细节，不直接作为固定界面文案。
    QString detail;

    /// 判断结果是否同时满足“无错误且存在返回值”。
    [[nodiscard]] bool ok() const noexcept;
    /// 构造包含有效设置快照的成功结果。
    [[nodiscard]] static AppearanceSettingsResult success(AppearanceSettings settings);
    /// 构造不含设置快照的失败结果。
    [[nodiscard]] static AppearanceSettingsResult failure(
        AppearanceSettingsError error, QString detail = {});
};

/// 校验并编排外观偏好读写，ViewModel 不直接接触 Repository。
///
/// 本 Service 返回同步结果但不建立 Qt 数据绑定：具体 ViewModel 将成功快照保存为
/// 可观察状态，再通过 Contract 的 getter 与 appearanceChanged() 通知 Widget。
class AppearanceSettingsService final {
public:
    /// 注入设置持久化端口；该构造过程不读取设置，也不触发任何展示通知。
    explicit AppearanceSettingsService(IAppearanceSettingsRepository &repository);

    /// 查询命令：返回当前设置快照，由 ViewModel 完成首次 getter 同步。
    [[nodiscard]] AppearanceSettingsResult load() const;
    /// 保存命令：校验并持久化完整设置；调用方根据返回结果决定是否发布 Contract 通知。
    [[nodiscard]] AppearanceSettingsResult save(const AppearanceSettings &settings);

private:
    /// 非拥有端口引用；由组合根创建，其生命周期必须长于本 Service。
    IAppearanceSettingsRepository &m_repository;
};

} // namespace smartmate::model
