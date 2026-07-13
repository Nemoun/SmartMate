#include "DeadlinePickerDialog.h"

#include <QCalendarWidget>
#include <QDate>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace smartmate::view::widgets {

DeadlinePickerDialog::DeadlinePickerDialog(QWidget *parent)
    : QDialog(parent)
    , m_calendar(new QCalendarWidget(this))
    , m_time(new QTimeEdit(this))
{
    setObjectName(QStringLiteral("deadlinePickerDialog"));
    setWindowTitle(tr("选择截止时间"));
    setModal(true);
    m_calendar->setObjectName(QStringLiteral("deadlineCalendar"));
    m_time->setObjectName(QStringLiteral("deadlineTimeEdit"));
    m_time->setDisplayFormat(QStringLiteral("HH:mm"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_calendar);
    layout->addWidget(m_time);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)
        ->setObjectName(QStringLiteral("confirmDeadlineSelectionButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void DeadlinePickerDialog::setSelection(const int year, const int month,
                                        const int day, const int hour,
                                        const int minute)
{
    const QDate date{year, month, day};
    const QTime time{hour, minute};
    if (date.isValid()) m_calendar->setSelectedDate(date);
    if (time.isValid()) m_time->setTime(time);
}

int DeadlinePickerDialog::selectedYear() const { return m_calendar->selectedDate().year(); }
int DeadlinePickerDialog::selectedMonth() const { return m_calendar->selectedDate().month(); }
int DeadlinePickerDialog::selectedDay() const { return m_calendar->selectedDate().day(); }
int DeadlinePickerDialog::selectedHour() const { return m_time->time().hour(); }
int DeadlinePickerDialog::selectedMinute() const { return m_time->time().minute(); }

} // namespace smartmate::view::widgets
