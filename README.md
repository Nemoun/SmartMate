# SmartMate

SmartMate is a Windows desktop task planner built with C++20, Qt Quick, and a strict MVVM architecture. The current revision is an architectural bootstrap: it proves the build boundaries, typed C++ ViewModel injection, QML loading, and Model testing without implementing task CRUD yet.

## Architecture

The required dependency direction is:

```text
View (QML) -> ViewModel -> Model Service -> Repository interface
```

Persistence is a Model implementation detail, and `src/app` is the sole composition root. See [the architecture document](docs/architecture.md) and [repository development rules](AGENTS.md).

## Toolchain

- Qt 6.10.2 with the bundled MinGW 13.1 kit
- CMake 3.24 or newer
- C++20

Do not compile against the unrelated global MinGW installation. In PowerShell, configure the Qt kit for the current session:

```powershell
$env:QT_ROOT='D:/Qt/6.10.2/mingw_64'
$env:QT_MINGW_ROOT='D:/Qt/Tools/mingw1310_64'
```

## Build and verify

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
cmake --build --preset debug --target all_qmllint
```

Use the `release` preset for a Release build. All generated files remain under `build/` and are ignored by Git.

The CTest suite includes an offscreen application bootstrap check. It loads the static QML module, injects the C++-owned `AppViewModel`, validates the required root property, and exits automatically.

## Current scope

This bootstrap intentionally contains no database implementation and no task-management UI. Those modules will be added incrementally after the MVVM boundaries and test harness are stable.
