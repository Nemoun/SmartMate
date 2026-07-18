SmartMate 第三方库说明：Qt

一、库名称与版本
Qt 6.10.2（MinGW 64-bit Kit）
配套编译器：MinGW 13.1.0 64-bit
项目使用的 Qt 模块：
- Qt Core
- Qt GUI
- Qt Widgets
- Qt SQL
- Qt Charts
- Qt Test（仅测试使用）
项目通过 CMake 的 find_package(Qt6 6.10.2 EXACT ...) 锁定该版本。
Qt 源码、头文件、二进制库和运行时文件不直接存放在本仓库中。

二、获取方式
Qt 官方开源版下载页面：
https://www.qt.io/download-open-source
Qt Online Installer：
https://www.qt.io/download-qt-installer-oss
安装时请选择 Qt 6.10.2 的 MinGW 64-bit Kit，并安装配套的 MinGW 13.1.0 64-bit 工具链。

三、引入原因及用途
SmartMate 是纯 C++20 Qt Widgets 桌面任务管理应用。Qt 用于：
- Core：对象模型、信号槽、时间和基础设施；
- GUI / Widgets：唯一正式桌面界面、窗口和桌宠绘制；
- SQL：SQLite 持久化及数据库驱动；
- Charts：统计页面图表；
- Test：单元测试和 Widgets 集成测试。
Release 发布由 scripts/deploy.ps1 调用 windeployqt 收集所需 Qt 动态库、平台插件、
SQLite 驱动及 MinGW 运行时。详细步骤参见 docs/windows-deployment.md。