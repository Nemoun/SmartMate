#include "ApplicationRuntime.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

#include <stdexcept>

namespace smartmate::app {

void configureApplicationIdentity()
{
    QCoreApplication::setOrganizationName(QStringLiteral("imperfect-jade"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("github.com/imperfect-jade"));
    QCoreApplication::setApplicationName(QStringLiteral("SmartMate"));
}

ApplicationRuntime applicationRuntime()
{
    const bool smokeTest = QCoreApplication::arguments().contains(
        QStringLiteral("--smoke-test"));
    if (smokeTest) {
        return {true, QStringLiteral(":memory:")};
    }

    const QString dataDirectory =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dataDirectory.isEmpty() || !QDir{}.mkpath(dataDirectory)) {
        throw std::runtime_error("Unable to create the SmartMate application data directory");
    }
    return {false, QDir{dataDirectory}.filePath(QStringLiteral("smartmate.db"))};
}

} // namespace smartmate::app
