#include "AppBootstrapper.h"
#include "ApplicationRuntime.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QTimer>

#include <cstdlib>
#include <exception>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    smartmate::app::configureApplicationIdentity();

    try {
        const auto runtime = smartmate::app::applicationRuntime();
        // bootstrapper 先于 QML 引擎构造、后于引擎析构，保证所有 QML 绑定
        // 都在其引用的 ViewModel 被释放前结束。
        smartmate::app::AppBootstrapper bootstrapper{runtime.databasePath};
        QQmlApplicationEngine engine;
        bootstrapper.configure(engine);
        engine.loadFromModule(QStringLiteral("SmartMate.View"), QStringLiteral("Main"));

        if (engine.rootObjects().isEmpty()) {
            return EXIT_FAILURE;
        }

        if (runtime.smokeTest) {
            QTimer::singleShot(0, &application, &QCoreApplication::quit);
        }

        return application.exec();
    } catch (const std::exception &error) {
        qCritical("SmartMate startup failed: %s", error.what());
        return EXIT_FAILURE;
    }
}
