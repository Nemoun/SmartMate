#include "TaskCategoryPresentation.h"
#include "TaskPresentationFormatter.h"

#include "domain/Task.h"

#include <QDateTime>
#include <QTest>
#include <QTimeZone>

using namespace smartmate;

static_assert(static_cast<int>(viewmodel::TaskStatusVisual::Todo) == 0);
static_assert(static_cast<int>(viewmodel::TaskStatusVisual::InProgress) == 1);
static_assert(static_cast<int>(viewmodel::TaskStatusVisual::Done) == 2);
static_assert(static_cast<int>(viewmodel::TaskStatusVisual::Cancelled) == 3);
static_assert(static_cast<int>(viewmodel::TaskStatusVisual::Archived) == 4);
static_assert(static_cast<int>(viewmodel::TaskPriorityVisual::Low) == 0);
static_assert(static_cast<int>(viewmodel::TaskPriorityVisual::Normal) == 1);
static_assert(static_cast<int>(viewmodel::TaskPriorityVisual::High) == 2);
static_assert(static_cast<int>(viewmodel::TaskPriorityVisual::Urgent) == 3);
static_assert(static_cast<int>(viewmodel::TaskGraphStatusFilter::All) == 0);
static_assert(static_cast<int>(viewmodel::TaskGraphStatusFilter::Todo) == 1);
static_assert(static_cast<int>(viewmodel::TaskGraphStatusFilter::InProgress) == 2);
static_assert(static_cast<int>(viewmodel::TaskGraphStatusFilter::Blocked) == 3);
static_assert(static_cast<int>(viewmodel::TaskGraphStatusFilter::Done) == 4);

class TaskPresentationTest final : public QObject {
    Q_OBJECT

private slots:
    void mapsTaskDescriptors();
    void mapsPriorityIndexesAndFilterOptions();
    void mapsCategoryDescriptorsAndFallbacks();
    void formatsDateTimeAndDuration();
};

void TaskPresentationTest::mapsTaskDescriptors()
{
    const QList<model::TaskStatus> statuses{
        model::TaskStatus::Todo, model::TaskStatus::InProgress,
        model::TaskStatus::Done, model::TaskStatus::Cancelled,
        model::TaskStatus::Archived};
    const QStringList statusTexts{QStringLiteral("待办"), QStringLiteral("进行中"),
                                  QStringLiteral("已完成"), QStringLiteral("已取消"),
                                  QStringLiteral("已归档")};
    for (int index = 0; index < statuses.size(); ++index) {
        QCOMPARE(viewmodel::taskStatusText(statuses.at(index)),
                 statusTexts.at(index));
        QCOMPARE(static_cast<int>(viewmodel::taskStatusVisual(statuses.at(index))),
                 index);
    }

    const QList<model::TaskPriority> priorities{
        model::TaskPriority::Low, model::TaskPriority::Normal,
        model::TaskPriority::High, model::TaskPriority::Urgent};
    const QStringList priorityTexts{QStringLiteral("低"), QStringLiteral("普通"),
                                    QStringLiteral("高"), QStringLiteral("紧急")};
    for (int index = 0; index < priorities.size(); ++index) {
        QCOMPARE(viewmodel::taskPriorityText(priorities.at(index)),
                 priorityTexts.at(index));
        QCOMPARE(static_cast<int>(viewmodel::taskPriorityVisual(priorities.at(index))),
                 index);
    }
    QCOMPARE(viewmodel::taskStatusText(static_cast<model::TaskStatus>(999)),
             QStringLiteral("未知"));
    QCOMPARE(viewmodel::taskPriorityText(static_cast<model::TaskPriority>(999)),
             QStringLiteral("未知"));
}

void TaskPresentationTest::mapsPriorityIndexesAndFilterOptions()
{
    const QList<model::TaskPriority> priorities{
        model::TaskPriority::Low, model::TaskPriority::Normal,
        model::TaskPriority::High, model::TaskPriority::Urgent};
    for (int index = 0; index < priorities.size(); ++index) {
        QCOMPARE(viewmodel::taskPriorityIndex(priorities.at(index)), index);
        QCOMPARE(viewmodel::taskPriorityFromIndex(index),
                 std::optional<model::TaskPriority>{priorities.at(index)});
    }
    QCOMPARE(viewmodel::taskPriorityIndex(static_cast<model::TaskPriority>(999)), -1);
    QVERIFY(!viewmodel::taskPriorityFromIndex(-1).has_value());
    QVERIFY(!viewmodel::taskPriorityFromIndex(4).has_value());
    QCOMPARE(viewmodel::taskPriorityOptions(),
             QStringList({QStringLiteral("低"), QStringLiteral("普通"),
                          QStringLiteral("高"), QStringLiteral("紧急")}));
    QCOMPARE(viewmodel::taskPriorityFilterOptions(),
             QStringList({QStringLiteral("全部优先级"), QStringLiteral("低"),
                          QStringLiteral("普通"), QStringLiteral("高"),
                          QStringLiteral("紧急")}));
    QCOMPARE(viewmodel::taskGraphStatusFilterOptions(),
             QStringList({QStringLiteral("全部状态"), QStringLiteral("待办"),
                          QStringLiteral("进行中"), QStringLiteral("阻塞"),
                          QStringLiteral("已完成")}));
    QCOMPARE(viewmodel::taskGraphStatusFilterFromIndex(-1),
             viewmodel::TaskGraphStatusFilter::All);
    QCOMPARE(viewmodel::taskGraphStatusFilterFromIndex(99),
             viewmodel::TaskGraphStatusFilter::Done);
}

void TaskPresentationTest::mapsCategoryDescriptorsAndFallbacks()
{
    const QList<model::TaskCategoryColor> colors{
        model::TaskCategoryColor::Blue, model::TaskCategoryColor::Teal,
        model::TaskCategoryColor::Green, model::TaskCategoryColor::Amber,
        model::TaskCategoryColor::Orange, model::TaskCategoryColor::Rose,
        model::TaskCategoryColor::Violet, model::TaskCategoryColor::Slate};
    const QStringList accents{QStringLiteral("#2563eb"), QStringLiteral("#0f766e"),
                              QStringLiteral("#15803d"), QStringLiteral("#b45309"),
                              QStringLiteral("#c2410c"), QStringLiteral("#be123c"),
                              QStringLiteral("#7c3aed"), QStringLiteral("#475569")};
    QCOMPARE(viewmodel::taskCategoryColorOptions(),
             QStringList({QStringLiteral("蓝色"), QStringLiteral("青色"),
                          QStringLiteral("绿色"), QStringLiteral("琥珀色"),
                          QStringLiteral("橙色"), QStringLiteral("玫红色"),
                          QStringLiteral("紫色"), QStringLiteral("灰蓝色")}));
    for (int index = 0; index < colors.size(); ++index) {
        QCOMPARE(viewmodel::taskCategoryColorIndex(colors.at(index)), index);
        QCOMPARE(viewmodel::taskCategoryColorFromIndex(index),
                 std::optional<model::TaskCategoryColor>{colors.at(index)});
        QCOMPARE(viewmodel::taskCategoryAccent(colors.at(index)), accents.at(index));
    }
    QVERIFY(!viewmodel::taskCategoryColorFromIndex(-1).has_value());
    QVERIFY(!viewmodel::taskCategoryColorFromIndex(8).has_value());
    QCOMPARE(viewmodel::taskUncategorizedAccent(), QStringLiteral("#94a3b8"));
    QCOMPARE(viewmodel::taskAllCategoriesAccent(), QStringLiteral("#64748b"));
}

void TaskPresentationTest::formatsDateTimeAndDuration()
{
    const QDateTime visible = QDateTime::fromString(
        QStringLiteral("2030-06-15T08:30:00+08:00"), Qt::ISODate);
    QCOMPARE(viewmodel::taskDateTimeText(visible),
             QStringLiteral("2030-06-15 08:30"));
    QCOMPARE(viewmodel::taskDateTimeText(QDateTime{}, QStringLiteral("未设置")),
             QStringLiteral("未设置"));

    const QDateTime now = QDateTime::fromMSecsSinceEpoch(
        1'900'000'000'000LL, QTimeZone::UTC);
    const model::Task task{QUuid::createUuid(), QStringLiteral("格式测试"), {},
                           model::TaskPriority::Normal, model::TaskStatus::Todo,
                           std::nullopt, std::nullopt, 1501, now, now};
    QCOMPARE(viewmodel::taskDurationText(task, {}),
             QStringLiteral("1天 1小时 1分钟"));
}

QTEST_GUILESS_MAIN(TaskPresentationTest)

#include "tst_TaskPresentation.moc"
