#include "services/AppearanceSettingsService.h"

#include "repositories/RepositoryException.h"

#include <exception>
#include <utility>

namespace smartmate::model {

bool AppearanceSettingsResult::ok() const noexcept
{
    return error == AppearanceSettingsError::None && value.has_value();
}

AppearanceSettingsResult AppearanceSettingsResult::success(
    AppearanceSettings settings)
{
    return {std::move(settings), AppearanceSettingsError::None, {}};
}

AppearanceSettingsResult AppearanceSettingsResult::failure(
    const AppearanceSettingsError error, QString detail)
{
    return {std::nullopt, error, std::move(detail)};
}

AppearanceSettingsService::AppearanceSettingsService(
    IAppearanceSettingsRepository &repository)
    : m_repository(repository)
{
}

AppearanceSettingsResult AppearanceSettingsService::load() const
{
    try {
        const AppearanceSettings settings = m_repository.load();
        // 持久化的旧值可能来自更早版本；读取时安全回退默认值，避免无效偏好阻断启动。
        return isValid(settings)
            ? AppearanceSettingsResult::success(settings)
            : AppearanceSettingsResult::success(AppearanceSettings{});
    } catch (const RepositoryException &exception) {
        return AppearanceSettingsResult::failure(
            AppearanceSettingsError::PersistenceFailure,
            QString::fromUtf8(exception.what()));
    } catch (...) {
        return AppearanceSettingsResult::failure(
            AppearanceSettingsError::PersistenceFailure,
            QStringLiteral("Unexpected appearance settings failure."));
    }
}

AppearanceSettingsResult AppearanceSettingsService::save(
    const AppearanceSettings &settings)
{
    // Service 在写入端再次校验，不能信任 ViewModel 或 Widget 已做过的输入限制。
    if (!isValid(settings)) {
        return AppearanceSettingsResult::failure(
            AppearanceSettingsError::InvalidValue,
            QStringLiteral("Appearance settings contain an invalid value."));
    }
    try {
        m_repository.save(settings);
        // 外观绑定由 ViewModel 持有：它取得成功结果后更新本地状态并发出 Contract 通知。
        // Service 不直接通知 Widget，也不依赖任何具体 ViewModel。
        return AppearanceSettingsResult::success(settings);
    } catch (const RepositoryException &exception) {
        return AppearanceSettingsResult::failure(
            AppearanceSettingsError::PersistenceFailure,
            QString::fromUtf8(exception.what()));
    } catch (...) {
        return AppearanceSettingsResult::failure(
            AppearanceSettingsError::PersistenceFailure,
            QStringLiteral("Unexpected appearance settings failure."));
    }
}

} // namespace smartmate::model
