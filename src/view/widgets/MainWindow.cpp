#include "view/widgets/MainWindow.h"

#include "view/widgets/settings/SettingsPage.h"
#include "view/widgets/task/TaskPage.h"
#include "view/widgets/theme/WidgetTheme.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCoreApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>

namespace smartmate::view::widgets {
namespace {

QWidget *migrationPlaceholder(const QString &title, const QString &description)
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("pageSurface"));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(36, 30, 36, 30);
    auto *titleLabel = new QLabel{title};
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    auto *descriptionLabel = new QLabel{description};
    descriptionLabel->setObjectName(QStringLiteral("secondaryText"));
    descriptionLabel->setWordWrap(true);
    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    layout->addStretch();
    return page;
}

QPushButton *navigationButton(const QString &text,
                              const QString &objectName,
                              QButtonGroup &group,
                              QVBoxLayout &layout,
                              const int index)
{
    auto *button = new QPushButton{text};
    button->setObjectName(objectName);
    button->setCheckable(true);
    group.addButton(button, index);
    layout.addWidget(button);
    return button;
}

} // namespace

MainWindow::MainWindow(MainWindowDependencies dependencies, QWidget *parent)
    : MainWindow(dependencies.appearanceSettings,
                 new TaskPage{{dependencies.taskList,
                               dependencies.taskFocus,
                               dependencies.taskDetails,
                               dependencies.taskEditor,
                               dependencies.taskCategories,
                               dependencies.taskDependencies}}, parent)
{
    auto *taskPage = qobject_cast<TaskPage *>(m_pages->widget(0));
    connect(taskPage, &TaskPage::showDependencyGraphRequested, m_pages,
            [this] { m_pages->setCurrentIndex(1); });
    connect(&dependencies.taskList, &viewmodel::TaskListContract::notificationRaised,
            this, &MainWindow::showNotification);
    connect(&dependencies.taskFocus, &viewmodel::TaskFocusContract::notificationRaised,
            this, &MainWindow::showNotification);
    connect(&dependencies.taskDetails, &viewmodel::TaskDetailsContract::notificationRaised,
            this, &MainWindow::showNotification);
    connect(&dependencies.taskEditor, &viewmodel::TaskEditorContract::notificationRaised,
            this, &MainWindow::showNotification);
    connect(&dependencies.taskCategories,
            &viewmodel::TaskCategoryContract::notificationRaised,
            this, &MainWindow::showNotification);
    connect(&dependencies.taskDependencies,
            &viewmodel::TaskDependencyContract::notificationRaised,
            this, &MainWindow::showNotification);
}

MainWindow::MainWindow(viewmodel::AppearanceSettingsContract &appearanceSettings,
                       QWidget *parent)
    : MainWindow(appearanceSettings,
                 migrationPlaceholder(tr("任务"), tr("任务页面未注入测试依赖。")),
                 parent)
{
}

MainWindow::MainWindow(viewmodel::AppearanceSettingsContract &appearanceSettings,
                       QWidget *taskPage, QWidget *parent)
    : QMainWindow(parent)
    , m_appearanceSettings(appearanceSettings)
    , m_baselineFont(QApplication::font())
    , m_pages(new QStackedWidget(this))
{
    setObjectName(QStringLiteral("mainWindow"));
    setWindowTitle(QCoreApplication::applicationName().isEmpty()
                       ? QStringLiteral("SmartMate")
                       : QCoreApplication::applicationName());
    resize(1180, 760);
    setMinimumSize(900, 620);

    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("pageSurface"));
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    setCentralWidget(central);

    auto *navigation = new QFrame(central);
    navigation->setObjectName(QStringLiteral("navigationPanel"));
    navigation->setFixedWidth(208);
    auto *navigationLayout = new QVBoxLayout(navigation);
    navigationLayout->setContentsMargins(14, 18, 14, 18);
    navigationLayout->setSpacing(8);

    auto *brand = new QLabel{QStringLiteral("SmartMate")};
    brand->setObjectName(QStringLiteral("sectionTitle"));
    navigationLayout->addWidget(brand);
    navigationLayout->addSpacing(18);

    auto *navigationGroup = new QButtonGroup(this);
    navigationGroup->setExclusive(true);
    auto *taskButton = navigationButton(tr("任务"),
                                        QStringLiteral("taskNavigationButton"),
                                        *navigationGroup,
                                        *navigationLayout, 0);
    taskButton->setAccessibleName(tr("任务"));
    navigationButton(tr("依赖图"), QStringLiteral("graphNavigationButton"),
                     *navigationGroup, *navigationLayout, 1);
    navigationButton(tr("设置"), QStringLiteral("settingsNavigationButton"),
                     *navigationGroup, *navigationLayout, 2);
    navigationLayout->addStretch();

    rootLayout->addWidget(navigation);
    rootLayout->addWidget(m_pages, 1);

    m_pages->addWidget(taskPage);
    m_pages->addWidget(migrationPlaceholder(
        tr("依赖图"), tr("依赖图将在后续阶段使用 QGraphicsView 迁移。")));
    m_pages->addWidget(new SettingsPage{appearanceSettings, m_pages});

    connect(navigationGroup, &QButtonGroup::idClicked, m_pages,
            [this](const int index) { m_pages->setCurrentIndex(index); });
    taskButton->setChecked(true);
    m_pages->setCurrentIndex(0);

    statusBar()->setObjectName(QStringLiteral("notificationStatusBar"));
    connect(&appearanceSettings,
            &viewmodel::AppearanceSettingsContract::appearanceChanged,
            this, &MainWindow::applyAppearance);
    connect(&appearanceSettings,
            &viewmodel::AppearanceSettingsContract::notificationRaised,
            this, &MainWindow::showNotification);
    applyAppearance();
}

void MainWindow::applyAppearance()
{
    const auto &settings = m_appearanceSettings;
    const WidgetTheme theme = WidgetTheme::fromAccentIndex(
        settings.accentThemeIndex());
    setPalette(theme.palette());
    setFont(appearanceFont(m_baselineFont, settings));
    setStyleSheet(theme.styleSheet());
}

void MainWindow::showNotification(const common::UiNotification &notification)
{
    const QString text = notification.title.isEmpty()
        ? notification.message
        : QStringLiteral("%1：%2").arg(notification.title, notification.message);
    const WidgetTheme theme = WidgetTheme::fromAccentIndex(
        m_appearanceSettings.accentThemeIndex());
    QPalette notificationPalette = statusBar()->palette();
    switch (notification.severity) {
    case common::UiSeverity::Information:
        notificationPalette.setColor(QPalette::WindowText, theme.textBody);
        break;
    case common::UiSeverity::Warning:
        notificationPalette.setColor(QPalette::WindowText, theme.warning);
        break;
    case common::UiSeverity::Error:
        notificationPalette.setColor(QPalette::WindowText, theme.danger);
        break;
    }
    statusBar()->setPalette(notificationPalette);
    statusBar()->showMessage(text);
}

} // namespace smartmate::view::widgets
