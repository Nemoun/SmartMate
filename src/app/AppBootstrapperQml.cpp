#include "AppBootstrapper.h"

#include "AppViewModel.h"

#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QVariant>
#include <QVariantMap>

namespace smartmate::app {

void AppBootstrapper::configure(QQmlApplicationEngine &engine)
{
    // QML 只能观察这些对象，生命周期仍由 C++ 管理；否则 QML 引擎可能误删
    // AppViewModel 内部拥有的子 ViewModel。
    QQmlEngine::setObjectOwnership(m_appViewModel.get(), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_appViewModel->taskList(), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_appViewModel->taskEditor(), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_appViewModel->taskDependencies(),
                                   QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_appViewModel->taskGraph(),
                                   QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_appViewModel->taskCategories(),
                                   QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(m_appViewModel->appearanceSettings(),
                                   QQmlEngine::CppOwnership);

    QVariantMap initialProperties;
    initialProperties.insert(QStringLiteral("appViewModel"),
                             QVariant::fromValue(m_appViewModel.get()));
    engine.setInitialProperties(initialProperties);
}

} // namespace smartmate::app
