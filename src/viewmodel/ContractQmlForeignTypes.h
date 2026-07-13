#pragma once

#include "viewmodel/contracts/AppearanceSettingsContract.h"
#include "viewmodel/contracts/TaskCategoryContract.h"
#include "viewmodel/contracts/TaskDependencyContract.h"
#include "viewmodel/contracts/TaskEditorContract.h"
#include "viewmodel/contracts/TaskGraphContract.h"
#include "viewmodel/contracts/TaskListContract.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace smartmate::viewmodel::qmlregistration {

/// 迁移期仅用于让 QML 注册器识别抽象 Contract 的继承元数据。
struct AppearanceSettingsContractForeign {
    Q_GADGET
    QML_FOREIGN(smartmate::viewmodel::AppearanceSettingsContract)
    QML_ANONYMOUS
};

struct TaskCategoryContractForeign {
    Q_GADGET
    QML_FOREIGN(smartmate::viewmodel::TaskCategoryContract)
    QML_ANONYMOUS
};

struct TaskDependencyContractForeign {
    Q_GADGET
    QML_FOREIGN(smartmate::viewmodel::TaskDependencyContract)
    QML_ANONYMOUS
};

struct TaskEditorContractForeign {
    Q_GADGET
    QML_FOREIGN(smartmate::viewmodel::TaskEditorContract)
    QML_ANONYMOUS
};

struct TaskGraphContractForeign {
    Q_GADGET
    QML_FOREIGN(smartmate::viewmodel::TaskGraphContract)
    QML_ANONYMOUS
};

struct TaskListContractForeign {
    Q_GADGET
    QML_FOREIGN(smartmate::viewmodel::TaskListContract)
    QML_ANONYMOUS
};

} // namespace smartmate::viewmodel::qmlregistration
