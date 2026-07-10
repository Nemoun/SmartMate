#include "AppViewModel.h"

namespace smartmate::viewmodel {

AppViewModel::AppViewModel(QObject *parent)
    : QObject(parent)
{
}

QString AppViewModel::applicationName() const
{
    return QStringLiteral("SmartMate");
}

} // namespace smartmate::viewmodel
