#pragma once

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

namespace smartmate::viewmodel {

class AppViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)
    QML_NAMED_ELEMENT(AppViewModel)
    QML_UNCREATABLE("AppViewModel is created and owned by AppBootstrapper")

public:
    explicit AppViewModel(QObject *parent = nullptr);

    [[nodiscard]] QString applicationName() const;
};

} // namespace smartmate::viewmodel
