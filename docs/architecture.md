# SmartMate Architecture

## Architectural goal

SmartMate uses MVVM to keep business behavior independent from Qt Quick presentation. The architecture is considered valid only when both source dependencies and runtime data flow follow:

```text
View -> ViewModel -> Model Service -> Repository interface
                                      ^
                                      |
                          Persistence implementation
```

`src/app` is the composition root. It constructs concrete objects, injects them, and starts the QML engine; it is not a Controller or business layer.

## Layer boundaries

### Model

Model owns domain entities, validation, state transitions, dependency and planning algorithms, statistics, service orchestration, and repository interfaces. Domain values may use Qt Core value types, but domain and service code cannot depend on Qt Quick, the QML runtime, ViewModel, or View.

Persistence will live under `src/model/persistence` and implement repository interfaces. It is the only production area allowed to use Qt SQL or `QSettings`.

### ViewModel

ViewModel owns observable presentation state, commands, form drafts, selection/filter state, and mapping structured Model errors into user-facing text. C++ owns every ViewModel. ViewModel may expose QML registration metadata but cannot control QML objects or depend on concrete persistence.

`AppViewModel` owns and coordinates child ViewModels. Child ViewModels do not call each other.

### View

View consists of QML layout, bindings, styling, animation, and event forwarding. A View never calls a Repository or Service and never implements business validation, state transitions, dependency checks, or planning scores.

## Build targets

```text
smartmate_model       -> Qt6::Core
smartmate_viewmodel   -> smartmate_model + Qt6::Core + QML registration metadata
smartmate_ui          -> Qt6::Quick + Qt6::QuickControls2
SmartMate             -> smartmate_viewmodel + smartmate_ui plugins
```

When persistence is introduced:

```text
smartmate_persistence -> smartmate_model + Qt6::Sql
SmartMate             -> smartmate_persistence (composition only)
```

`smartmate_viewmodel` must never link to `smartmate_persistence`.

## Bootstrap object flow

1. `main.cpp` creates `QGuiApplication`, `AppBootstrapper`, and `QQmlApplicationEngine`.
2. `AppBootstrapper` owns `AppViewModel` and provides it as an initial root property.
3. The static `SmartMate.ViewModel` module declares `AppViewModel` as uncreatable from QML.
4. `Main.qml` declares `required property AppViewModel appViewModel` and binds its title to `applicationName`.

This is the smallest typed View-to-ViewModel chain. Task data is deliberately not wired into the UI in the bootstrap commit.
The `view.qml_bootstrap` CTest executes this chain on the offscreen Qt platform and exits after the root object is created.

## Target structure

Modules are created only when they contain real code; empty folders and `.gitkeep` files are not committed.

```text
src/
  app/
  model/
    domain/
    repositories/
    services/
    persistence/       # added with SQLite/QSettings work
  viewmodel/
    tasks/             # added with CRUD work
    planner/
    pet/
    focus/
    statistics/
  view/
    qml/
      pages/
      dialogs/
      components/
      pet/
      focus/
      theme/
```

Future dependency graphs add a graph projection ViewModel and QML page; Windows notifications add a Model gateway interface and a Windows persistence/platform adapter; richer pet animation changes only QML assets while continuing to consume semantic states from `PetViewModel`.

## Enforcement

The project combines three controls:

1. CMake target visibility restricts which libraries each layer can link.
2. `tests/architecture/check_mvvm_boundaries.cmake` checks forbidden production includes, links, QML access patterns, and Controller types.
3. `AGENTS.md` requires every future change to identify its layer, data flow, and tests.
