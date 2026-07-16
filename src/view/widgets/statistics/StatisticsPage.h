#pragma once

#include "viewmodel/contracts/StatisticsContract.h"

#include <QScrollArea>
#include <QStringList>

class QBarSeries;
class QChart;
class QChartView;
class QEvent;
class QFrame;
class QGridLayout;
class QLabel;
class QPieSeries;
class QPushButton;
class QResizeEvent;
class QShowEvent;
class QStackedWidget;
class QTimer;
class QVBoxLayout;
class QWidget;

namespace smartmate::view::widgets {

/// 纯 Widgets 统计仪表盘；只消费 Statistics Contract，并把稳定 Role 转换为 Chart series。
class StatisticsPage final : public QScrollArea {
    Q_OBJECT

public:
    explicit StatisticsPage(viewmodel::StatisticsContract &statistics,
                            QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void scheduleRefresh();
    void refreshAll();
    void refreshOverview();
    void refreshTrend();
    void refreshCategories();
    void refreshHealth();
    void syncRangeButtons();
    void applyTheme();
    void applyResponsiveLayout();
    void connectModel(QAbstractItemModel *model);

    viewmodel::StatisticsContract &m_statistics;
    QWidget *m_content;
    QGridLayout *m_overviewLayout;
    QGridLayout *m_sectionsLayout;
    QFrame *m_todayCard;
    QFrame *m_weekCard;
    QFrame *m_onTimeCard;
    QFrame *m_overdueCard;
    QLabel *m_todayValue;
    QLabel *m_todayDetail;
    QLabel *m_weekValue;
    QLabel *m_weekDetail;
    QLabel *m_onTimeValue;
    QLabel *m_onTimeDetail;
    QLabel *m_overdueValue;
    QLabel *m_overdueDetail;
    QPushButton *m_last7Days;
    QPushButton *m_last30Days;
    QPushButton *m_last12Weeks;
    QPushButton *m_refresh;
    QFrame *m_trendCard;
    QFrame *m_categoryCard;
    QFrame *m_healthCard;
    QStackedWidget *m_trendStack;
    QStackedWidget *m_categoryStack;
    QLabel *m_trendEmpty;
    QLabel *m_categoryEmpty;
    QLabel *m_trendSummary;
    QLabel *m_categorySummary;
    QLabel *m_healthSummary;
    QVBoxLayout *m_healthRows;
    QChart *m_trendChart;
    QChartView *m_trendView;
    QBarSeries *m_trendSeries;
    QChart *m_categoryChart;
    QChartView *m_categoryView;
    QChart *m_onTimeChart;
    QChartView *m_onTimeView;
    QPieSeries *m_onTimeSeries;
    QStringList m_trendTooltips;
    QTimer *m_refreshTimer;
    bool m_hasBeenShown{false};
    bool m_wideLayout{false};
};

} // namespace smartmate::view::widgets
