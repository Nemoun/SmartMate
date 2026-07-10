#include "AppBootstrapper.h"

#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QVariant>
#include <QVariantMap>

namespace smartmate::app {

AppBootstrapper::AppBootstrapper()
    : m_appViewModel(std::make_unique<viewmodel::AppViewModel>())
{
}

void AppBootstrapper::configure(QQmlApplicationEngine &engine)
{
    QQmlEngine::setObjectOwnership(m_appViewModel.get(), QQmlEngine::CppOwnership);

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("appViewModel"),
                             QVariant::fromValue(m_appViewModel.get()));
    engine.setInitialProperties(initialProperties);
}

} // namespace smartmate::app
