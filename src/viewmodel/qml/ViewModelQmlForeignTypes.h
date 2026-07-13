#pragma once

#include "AppViewModel.h"
#include "AppearanceSettingsViewModel.h"
#include "TaskCategoryViewModel.h"
#include "TaskDependencyViewModel.h"
#include "TaskEditorViewModel.h"
#include "TaskGraphViewModel.h"
#include "TaskListViewModel.h"
#include "viewmodel/contracts/AppearanceSettingsContract.h"
#include "viewmodel/contracts/TaskCategoryContract.h"
#include "viewmodel/contracts/TaskDependencyContract.h"
#include "viewmodel/contracts/TaskEditorContract.h"
#include "viewmodel/contracts/TaskGraphContract.h"
#include "viewmodel/contracts/TaskListContract.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace smartmate::viewmodel::qmlregistration {

#define SMARTMATE_QML_ANONYMOUS_FOREIGN(WrapperName, ForeignType) \
    struct WrapperName {                                           \
        Q_GADGET                                                   \
        QML_FOREIGN(ForeignType)                                   \
        QML_ANONYMOUS                                              \
    }

SMARTMATE_QML_ANONYMOUS_FOREIGN(AppearanceSettingsContractForeign,
                                 smartmate::viewmodel::AppearanceSettingsContract);
SMARTMATE_QML_ANONYMOUS_FOREIGN(TaskCategoryContractForeign,
                                 smartmate::viewmodel::TaskCategoryContract);
SMARTMATE_QML_ANONYMOUS_FOREIGN(TaskDependencyContractForeign,
                                 smartmate::viewmodel::TaskDependencyContract);
SMARTMATE_QML_ANONYMOUS_FOREIGN(TaskEditorContractForeign,
                                 smartmate::viewmodel::TaskEditorContract);
SMARTMATE_QML_ANONYMOUS_FOREIGN(TaskGraphContractForeign,
                                 smartmate::viewmodel::TaskGraphContract);
SMARTMATE_QML_ANONYMOUS_FOREIGN(TaskListContractForeign,
                                 smartmate::viewmodel::TaskListContract);

#undef SMARTMATE_QML_ANONYMOUS_FOREIGN

#define SMARTMATE_QML_UNCREATABLE_FOREIGN(WrapperName, ForeignType, ElementName, Reason) \
    struct WrapperName {                                                                   \
        Q_GADGET                                                                           \
        QML_FOREIGN(ForeignType)                                                           \
        QML_NAMED_ELEMENT(ElementName)                                                      \
        QML_UNCREATABLE(Reason)                                                            \
    }

SMARTMATE_QML_UNCREATABLE_FOREIGN(
    AppViewModelForeign, smartmate::viewmodel::AppViewModel, AppViewModel,
    "AppViewModel is created and owned by AppBootstrapper");
SMARTMATE_QML_UNCREATABLE_FOREIGN(
    AppearanceSettingsViewModelForeign,
    smartmate::viewmodel::AppearanceSettingsViewModel,
    AppearanceSettingsViewModel,
    "AppearanceSettingsViewModel is owned by AppViewModel");
SMARTMATE_QML_UNCREATABLE_FOREIGN(
    TaskCategoryViewModelForeign, smartmate::viewmodel::TaskCategoryViewModel,
    TaskCategoryViewModel, "TaskCategoryViewModel is owned by AppViewModel");
SMARTMATE_QML_UNCREATABLE_FOREIGN(
    TaskDependencyViewModelForeign, smartmate::viewmodel::TaskDependencyViewModel,
    TaskDependencyViewModel, "TaskDependencyViewModel is owned by AppViewModel");
SMARTMATE_QML_UNCREATABLE_FOREIGN(
    TaskEditorViewModelForeign, smartmate::viewmodel::TaskEditorViewModel,
    TaskEditorViewModel, "TaskEditorViewModel is owned by AppViewModel");
SMARTMATE_QML_UNCREATABLE_FOREIGN(
    TaskGraphViewModelForeign, smartmate::viewmodel::TaskGraphViewModel,
    TaskGraphViewModel, "TaskGraphViewModel is owned by AppViewModel");
SMARTMATE_QML_UNCREATABLE_FOREIGN(
    TaskListViewModelForeign, smartmate::viewmodel::TaskListViewModel,
    TaskListViewModel, "TaskListViewModel is owned by AppViewModel");

#undef SMARTMATE_QML_UNCREATABLE_FOREIGN

} // namespace smartmate::viewmodel::qmlregistration
