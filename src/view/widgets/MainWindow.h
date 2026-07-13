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

private:
    void applyAppearance();
    void showNotification(const common::UiNotification &notification);

    MainWindowDependencies m_dependencies;
    QFont m_baselineFont;
    QStackedWidget *m_pages;
};

} // namespace smartmate::view::widgets
