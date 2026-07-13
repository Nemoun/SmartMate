#pragma once

#include "common/presentation/UiNotification.h"
#include "view/widgets/MainWindowDependencies.h"

#include <QFont>
#include <QMainWindow>

class QStackedWidget;

namespace smartmate::view::widgets {

/// 纯 Widgets 主窗口：组合页面、应用展示主题并呈现 Contract 通知。
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(MainWindowDependencies dependencies,
                        QWidget *parent = nullptr);
    /// 仅用于隔离测试设置页与窗口主题；生产组合根始终使用完整依赖构造函数。
    explicit MainWindow(viewmodel::AppearanceSettingsContract &appearanceSettings,
                        QWidget *parent = nullptr);

private:
    MainWindow(viewmodel::AppearanceSettingsContract &appearanceSettings,
               QWidget *taskPage, QWidget *parent);
    void applyAppearance();
    void showNotification(const common::UiNotification &notification);

    viewmodel::AppearanceSettingsContract &m_appearanceSettings;
    QFont m_baselineFont;
    QStackedWidget *m_pages;
};

} // namespace smartmate::view::widgets
