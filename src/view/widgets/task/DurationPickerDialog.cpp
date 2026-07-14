#include "DurationPickerDialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>

#include <algorithm>

namespace smartmate::view::widgets {

DurationPickerDialog::DurationPickerDialog(const int minimumMinutes,
                                           const int maximumMinutes,
                                           QWidget *parent)
    : QDialog(parent)
    , m_minimumMinutes(std::max(0, minimumMinutes))
    , m_maximumMinutes(std::max(m_minimumMinutes, maximumMinutes))
    , m_days(new QSpinBox(this))
    , m_hours(new QSpinBox(this))
    , m_minutes(new QSpinBox(this))
{
    setObjectName(QStringLiteral("durationPickerDialog"));
    setWindowTitle(tr("选择预计用时"));
    setModal(true);
    m_days->setObjectName(QStringLiteral("durationDaysSpinBox"));
    m_hours->setObjectName(QStringLiteral("durationHoursSpinBox"));
    m_minutes->setObjectName(QStringLiteral("durationMinutesSpinBox"));
    m_days->setRange(0, m_maximumMinutes / (24 * 60));

    auto *layout = new QGridLayout(this);
    layout->addWidget(new QLabel(tr("天"), this), 0, 0);
    layout->addWidget(new QLabel(tr("小时"), this), 0, 1);
    layout->addWidget(new QLabel(tr("分钟"), this), 0, 2);
    layout->addWidget(m_days, 1, 0);
    layout->addWidget(m_hours, 1, 1);
    layout->addWidget(m_minutes, 1, 2);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)
        ->setObjectName(QStringLiteral("confirmDurationSelectionButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons, 2, 0, 1, 3);

    connect(m_days, &QSpinBox::valueChanged, this,
            &DurationPickerDialog::synchronizeRanges);
    connect(m_hours, &QSpinBox::valueChanged, this,
            &DurationPickerDialog::synchronizeRanges);
    synchronizeRanges();
}

void DurationPickerDialog::setDuration(const int days, const int hours,
                                       const int minutes)
{
    const int requested = days * 24 * 60 + hours * 60 + minutes;
    const int total = std::clamp(requested, m_minimumMinutes, m_maximumMinutes);
    const QSignalBlocker daysBlocker(m_days);
    const QSignalBlocker hoursBlocker(m_hours);
    m_days->setValue(total / (24 * 60));
    synchronizeRanges();
    m_hours->setValue((total % (24 * 60)) / 60);
    synchronizeRanges();
    m_minutes->setValue(total % 60);
}

int DurationPickerDialog::selectedDays() const { return m_days->value(); }
int DurationPickerDialog::selectedHours() const { return m_hours->value(); }
int DurationPickerDialog::selectedMinutes() const { return m_minutes->value(); }
int DurationPickerDialog::totalMinutes() const
{
    return selectedDays() * 24 * 60 + selectedHours() * 60 + selectedMinutes();
}

void DurationPickerDialog::synchronizeRanges()
{
    const QSignalBlocker hoursBlocker(m_hours);
    const QSignalBlocker minutesBlocker(m_minutes);
    const int dayMinutes = m_days->value() * 24 * 60;
    const int maximumHours = std::clamp(
        (m_maximumMinutes - dayMinutes) / 60, 0, 23);
    m_hours->setRange(0, maximumHours);

    const int baseMinutes = dayMinutes + m_hours->value() * 60;
    const int minimumPart = std::clamp(m_minimumMinutes - baseMinutes, 0, 59);
    const int maximumPart = std::clamp(m_maximumMinutes - baseMinutes, 0, 59);
    m_minutes->setRange(std::min(minimumPart, maximumPart), maximumPart);
}

} // namespace smartmate::view::widgets
