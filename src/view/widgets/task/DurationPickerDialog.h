#pragma once

#include <QDialog>

class QSpinBox;

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

    int m_minimumMinutes;
    int m_maximumMinutes;
    QSpinBox *m_days;
    QSpinBox *m_hours;
    QSpinBox *m_minutes;
};

} // namespace smartmate::view::widgets
