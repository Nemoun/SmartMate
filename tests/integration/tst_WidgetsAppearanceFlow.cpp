#include "AppearanceSettingsViewModel.h"
#include "fakes/FakeAppearanceSettingsRepository.h"
#include "services/AppearanceSettingsService.h"
#include "view/widgets/MainWindow.h"

#include <QPushButton>
#include <QStatusBar>
#include <QTest>

using namespace smartmate;

class WidgetsAppearanceFlowTest final : public QObject {
    Q_OBJECT

private slots:
    void widgetCommandPersistsThroughContractAndService();
    void failedCommandRaisesWindowNotification();
};

void WidgetsAppearanceFlowTest::widgetCommandPersistsThroughContractAndService()
{
    tests::FakeAppearanceSettingsRepository repository;
    model::AppearanceSettingsService service{repository};
    viewmodel::AppearanceSettingsViewModel viewModel{service};
    view::widgets::MainWindow window{{viewModel}};

    auto *button = window.findChild<QPushButton *>(
        QStringLiteral("accentThemeButton_1"));
    QVERIFY(button != nullptr);
    QTest::mouseClick(button, Qt::LeftButton);

    QCOMPARE(repository.settings.accentTheme, model::AccentTheme::Blue);
    QCOMPARE(repository.saveCount, 1);
    QCOMPARE(viewModel.accentThemeIndex(), 1);
}

void WidgetsAppearanceFlowTest::failedCommandRaisesWindowNotification()
{
    tests::FakeAppearanceSettingsRepository repository;
    model::AppearanceSettingsService service{repository};
    viewmodel::AppearanceSettingsViewModel viewModel{service};
    view::widgets::MainWindow window{{viewModel}};
    repository.failSave = true;

    auto *button = window.findChild<QPushButton *>(
        QStringLiteral("fontScaleButton_2"));
    QVERIFY(button != nullptr);
    QTest::mouseClick(button, Qt::LeftButton);

    QVERIFY(window.statusBar()->currentMessage().startsWith(
        QStringLiteral("外观设置失败：")));
    QCOMPARE(viewModel.fontScaleIndex(), 1);
}

QTEST_MAIN(WidgetsAppearanceFlowTest)
#include "tst_WidgetsAppearanceFlow.moc"
