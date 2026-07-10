#include "AppBootstrapper.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>

#include <cstdlib>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("imperfect-jade"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("github.com/imperfect-jade"));
    QCoreApplication::setApplicationName(QStringLiteral("SmartMate"));

    smartmate::app::AppBootstrapper bootstrapper;
    QQmlApplicationEngine engine;
    bootstrapper.configure(engine);
    engine.loadFromModule(QStringLiteral("SmartMate.View"), QStringLiteral("Main"));

    if (engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    if (QCoreApplication::arguments().contains(QStringLiteral("--smoke-test"))) {
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
    }

    return application.exec();
}
