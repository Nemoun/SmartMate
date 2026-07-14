#pragma once

#include <QDialog>

class QSpinBox;
class QLabel;
class QGridLayout;
class QResizeEvent;
class QShowEvent;
class QWidget;

namespace smartmate::view::widgets {

/// 依据 Contract 投影的分钟边界收集类型化预计用时。
class DurationPickerDialog final : public QDialog {
    Q_OBJECT
public:
    DurationPickerDialog(int minimumMinutes, int maximumMinutes,
                         QWidget *parent = nullptr);

    void setDuration(int days, int hours, int minutes);
    [[nodiscard]] int selectedDays() const;
    [[nodiscard]] int selectedHours() const;
    [[nodiscard]] int selectedMinutes() const;
    [[nodiscard]] int totalMinutes() const;

private:
    void synchronizeRanges();
    void updateSummary();
    void updateResponsiveLayout();
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

    int m_minimumMinutes;
    int m_maximumMinutes;
    QSpinBox *m_days;
    QSpinBox *m_hours;
    QSpinBox *m_minutes;
    QGridLayout *m_valuesGrid;
    QWidget *m_daysField;
    QWidget *m_hoursField;
    QWidget *m_minutesField;
    QLabel *m_summary;
};

} // namespace smartmate::view::widgets
