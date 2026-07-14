#pragma once

#include <QDialog>

class QCalendarWidget;
class QTimeEdit;

namespace smartmate::view::widgets {

/// 使用日历和时间控件收集截止时间候选值，不解析自由文本。
class DeadlinePickerDialog final : public QDialog {
    Q_OBJECT
public:
    explicit DeadlinePickerDialog(QWidget *parent = nullptr);

    void setSelection(int year, int month, int day, int hour, int minute);
    [[nodiscard]] int selectedYear() const;
    [[nodiscard]] int selectedMonth() const;
    [[nodiscard]] int selectedDay() const;
    [[nodiscard]] int selectedHour() const;
    [[nodiscard]] int selectedMinute() const;

private:
    QCalendarWidget *m_calendar;
    QTimeEdit *m_time;
};

} // namespace smartmate::view::widgets
