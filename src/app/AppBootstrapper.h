#pragma once

#include "AppViewModel.h"

#include <memory>

class QQmlApplicationEngine;

namespace smartmate::app {

class AppBootstrapper final {
public:
    AppBootstrapper();

    void configure(QQmlApplicationEngine &engine);

private:
    std::unique_ptr<viewmodel::AppViewModel> m_appViewModel;
};

} // namespace smartmate::app
