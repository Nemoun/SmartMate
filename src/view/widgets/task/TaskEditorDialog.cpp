#include "TaskEditorDialog.h"

#include "DeadlinePickerDialog.h"
#include "DurationPickerDialog.h"

#include "viewmodel/contracts/TaskEditorContract.h"

#include <QComboBox>
#include <QDate>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTime>
#include <QVBoxLayout>

namespace smartmate::view::widgets {

TaskEditorDialog::TaskEditorDialog(viewmodel::TaskEditorContract &editor,
                                   QWidget *parent)
    : QDialog(parent)
    , m_editor(editor)
    , m_title(new QLineEdit(this))
    , m_description(new QPlainTextEdit(this))
    , m_status(new QLabel(this))
    , m_priority(new QComboBox(this))
    , m_category(new QComboBox(this))
    , m_deadline(new QLabel(this))
    , m_duration(new QLabel(this))
    , m_validation(new QLabel(this))
    , m_save(new QPushButton(tr("保存"), this))
{
    setObjectName(QStringLiteral("taskEditorDialog"));
    setWindowModality(Qt::WindowModal);
    setModal(true);
    resize(650, 600);
    setMinimumSize(480, 480);

    m_title->setObjectName(QStringLiteral("taskTitleField"));
    m_description->setObjectName(QStringLiteral("taskDescriptionArea"));
    m_status->setObjectName(QStringLiteral("taskCurrentStatusLabel"));
    m_priority->setObjectName(QStringLiteral("taskPriorityComboBox"));
    m_category->setObjectName(QStringLiteral("taskCategoryComboBox"));
    m_save->setObjectName(QStringLiteral("saveTaskButton"));
    m_description->setMinimumHeight(130);
    m_validation->setWordWrap(true);

    auto *root = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    form->addRow(tr("标题"), m_title);
    form->addRow(tr("描述"), m_description);
    form->addRow(tr("状态"), m_status);
    form->addRow(tr("优先级"), m_priority);
    form->addRow(tr("类别"), m_category);

    auto *deadlineRow = new QWidget(this);
    auto *deadlineLayout = new QHBoxLayout(deadlineRow);
    deadlineLayout->setContentsMargins(0, 0, 0, 0);
    deadlineLayout->addWidget(m_deadline, 1);
    auto *deadlineChoose = new QPushButton(tr("选择"), deadlineRow);
    deadlineChoose->setObjectName(QStringLiteral("openDeadlinePickerButton"));
    auto *deadlineClear = new QPushButton(tr("清除"), deadlineRow);
    deadlineClear->setObjectName(QStringLiteral("clearDeadlineButton"));
    deadlineLayout->addWidget(deadlineChoose);
    deadlineLayout->addWidget(deadlineClear);
    form->addRow(tr("截止时间"), deadlineRow);

    auto *durationRow = new QWidget(this);
    auto *durationLayout = new QHBoxLayout(durationRow);
    durationLayout->setContentsMargins(0, 0, 0, 0);
    durationLayout->addWidget(m_duration, 1);
    auto *durationChoose = new QPushButton(tr("选择"), durationRow);
    durationChoose->setObjectName(QStringLiteral("openDurationPickerButton"));
    auto *durationClear = new QPushButton(tr("清除"), durationRow);
    durationClear->setObjectName(QStringLiteral("clearDurationButton"));
    durationLayout->addWidget(durationChoose);
    durationLayout->addWidget(durationClear);
    form->addRow(tr("预计用时"), durationRow);
    root->addLayout(form);
    root->addWidget(m_validation);
    root->addStretch();
    auto *buttons = new QDialogButtonBox(this);
    auto *cancel = buttons->addButton(tr("取消"), QDialogButtonBox::RejectRole);
    buttons->addButton(m_save, QDialogButtonBox::AcceptRole);
    root->addWidget(buttons);

    connect(m_title, &QLineEdit::textEdited, &m_editor,
            &viewmodel::TaskEditorContract::setTitle);
    connect(m_description, &QPlainTextEdit::textChanged, this, [this] {
        m_editor.setDescription(m_description->toPlainText());
    });
    connect(m_priority, &QComboBox::activated, &m_editor,
            &viewmodel::TaskEditorContract::setPriorityIndex);
    connect(m_category, &QComboBox::activated, this, [this](const int index) {
        m_editor.setSelectedCategoryId(m_category->itemData(index).toString());
    });
    connect(deadlineChoose, &QPushButton::clicked, this, &TaskEditorDialog::chooseDeadline);
    connect(deadlineClear, &QPushButton::clicked, &m_editor,
            &viewmodel::TaskEditorContract::clearDeadline);
    connect(durationChoose, &QPushButton::clicked, this, &TaskEditorDialog::chooseDuration);
    connect(durationClear, &QPushButton::clicked, &m_editor,
            &viewmodel::TaskEditorContract::clearEstimatedDuration);
    connect(cancel, &QPushButton::clicked, this, &TaskEditorDialog::reject);
    connect(m_save, &QPushButton::clicked, &m_editor,
            &viewmodel::TaskEditorContract::save);

    const auto sync = [this] { synchronize(); };
    connect(&m_editor, &viewmodel::TaskEditorContract::modeChanged, this, sync);
    connect(&m_editor, &viewmodel::TaskEditorContract::titleChanged, this, sync);
    connect(&m_editor, &viewmodel::TaskEditorContract::descriptionChanged, this, sync);
    connect(&m_editor, &viewmodel::TaskEditorContract::currentStatusTextChanged, this, sync);
    connect(&m_editor, &viewmodel::TaskEditorContract::priorityIndexChanged, this, sync);
    connect(&m_editor, &viewmodel::TaskEditorContract::deadlineChanged, this, sync);
    connect(&m_editor, &viewmodel::TaskEditorContract::estimatedDurationChanged, this, sync);
    connect(&m_editor, &viewmodel::TaskEditorContract::categoryOptionsChanged, this, sync);
    connect(&m_editor, &viewmodel::TaskEditorContract::categoryChanged, this, sync);
    connect(&m_editor, &viewmodel::TaskEditorContract::formStateChanged, this, sync);
    connect(&m_editor, &viewmodel::TaskEditorContract::sessionActiveChanged,
            this, &TaskEditorDialog::synchronizeSession);
    synchronize();
    synchronizeSession();
}

void TaskEditorDialog::reject()
{
    if (m_editor.sessionActive()) m_editor.cancel();
    QDialog::reject();
}

void TaskEditorDialog::synchronizeSession()
{
    if (m_editor.sessionActive()) {
        setWindowTitle(m_editor.editMode() ? tr("编辑任务") : tr("新建任务"));
        if (!isVisible()) open();
    } else if (isVisible()) {
        QDialog::accept();
    }
}

void TaskEditorDialog::synchronize()
{
    const QSignalBlocker titleBlocker(m_title);
    const QSignalBlocker descriptionBlocker(m_description);
    const QSignalBlocker priorityBlocker(m_priority);
    const QSignalBlocker categoryBlocker(m_category);
    m_title->setText(m_editor.title());
    m_description->setPlainText(m_editor.description());
    m_status->setText(m_editor.editMode() ? m_editor.currentStatusText() : tr("初始状态：待办"));
    if (m_priority->count() != m_editor.priorityOptions().size()) {
        m_priority->clear();
        m_priority->addItems(m_editor.priorityOptions());
    }
    m_priority->setCurrentIndex(m_editor.priorityIndex());
    m_category->clear();
    const QVariantList categories = m_editor.categoryOptions();
    for (const QVariant &value : categories) {
        const QVariantMap map = value.toMap();
        m_category->addItem(map.value(QStringLiteral("name")).toString(),
                            map.value(QStringLiteral("categoryId")).toString());
    }
    const int categoryIndex = m_category->findData(m_editor.selectedCategoryId());
    m_category->setCurrentIndex(categoryIndex < 0 ? 0 : categoryIndex);
    m_deadline->setText(m_editor.deadlineDisplayText());
    m_duration->setText(m_editor.estimatedDurationDisplayText());
    m_validation->setText(m_editor.validationMessage());
    m_save->setEnabled(m_editor.canSave());
}

void TaskEditorDialog::chooseDeadline()
{
    DeadlinePickerDialog picker(this);
    if (m_editor.hasDeadline()) {
        picker.setSelection(m_editor.deadlineYear(), m_editor.deadlineMonth(),
                            m_editor.deadlineDay(), m_editor.deadlineHour(),
                            m_editor.deadlineMinute());
    } else {
        const QDate tomorrow = QDate::currentDate().addDays(1);
        const QTime now = QTime::currentTime();
        picker.setSelection(tomorrow.year(), tomorrow.month(), tomorrow.day(),
                            now.hour(), now.minute());
    }
    if (picker.exec() == QDialog::Accepted) {
        m_editor.setDeadlineSelection(
            picker.selectedYear(), picker.selectedMonth(), picker.selectedDay(),
            picker.selectedHour(), picker.selectedMinute());
    }
}

void TaskEditorDialog::chooseDuration()
{
    DurationPickerDialog picker(m_editor.minimumEstimatedMinutes(),
                                m_editor.maximumEstimatedMinutes(), this);
    picker.setDuration(m_editor.hasEstimatedDuration() ? m_editor.estimatedDays() : 0,
                       m_editor.hasEstimatedDuration() ? m_editor.estimatedHours() : 0,
                       m_editor.hasEstimatedDuration() ? m_editor.estimatedMinutePart() : 30);
    if (picker.exec() == QDialog::Accepted) {
        m_editor.setEstimatedDuration(picker.selectedDays(), picker.selectedHours(),
                                      picker.selectedMinutes());
    }
}

} // namespace smartmate::view::widgets
