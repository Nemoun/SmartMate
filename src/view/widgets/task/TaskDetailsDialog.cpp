#include "TaskDetailsDialog.h"

#include "view/widgets/theme/WidgetTheme.h"
#include "viewmodel/contracts/TaskDetailsContract.h"

#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>

namespace smartmate::view::widgets {
namespace {

QLabel *sectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("taskDetailsSectionTitle"));
    return label;
}

QFrame *sectionCard(QWidget *parent, const QString &objectName,
                    QVBoxLayout *&layout)
{
    auto *card = new QFrame(parent);
    card->setObjectName(objectName);
    layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(10);
    return card;
}

QWidget *metadataField(QWidget *parent, const QString &caption,
                       const QString &valueObjectName, QLabel *&value)
{
    auto *field = new QFrame(parent);
    field->setObjectName(QStringLiteral("taskDetailsMetadataItem"));
    auto *layout = new QVBoxLayout(field);
    layout->setContentsMargins(12, 9, 12, 10);
    layout->setSpacing(3);
    auto *captionLabel = new QLabel(caption, field);
    captionLabel->setObjectName(QStringLiteral("taskDetailsMetadataCaption"));
    value = new QLabel(field);
    value->setObjectName(valueObjectName);
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(captionLabel);
    layout->addWidget(value);
    return field;
}

QFrame *metricCard(QWidget *parent, const QString &caption,
                   const QString &valueObjectName, QLabel *&value)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("taskDetailsMetric"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 9, 12, 9);
    layout->setSpacing(2);
    value = new QLabel(card);
    value->setObjectName(valueObjectName);
    auto *captionLabel = new QLabel(caption, card);
    captionLabel->setObjectName(QStringLiteral("taskDetailsMetricCaption"));
    layout->addWidget(value);
    layout->addWidget(captionLabel);
    return card;
}

QString rgba(const QColor &color, const int alpha)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(color.red()).arg(color.green()).arg(color.blue()).arg(alpha);
}

void styleBadge(QLabel *badge, const QColor &color)
{
    badge->setStyleSheet(QStringLiteral(
        "QLabel#%1 { color: %2; background: %3; border: 1px solid %2; "
        "border-radius: 9px; padding: 3px 8px; font-size: 11px; font-weight: 700; }")
        .arg(badge->objectName(), color.name(), rgba(color, 24)));
}

} // namespace

TaskDetailsDialog::TaskDetailsDialog(viewmodel::TaskDetailsContract &details,
                                     QWidget *parent)
    : QDialog(parent)
    , m_details(details)
    , m_title(new QLabel(this))
    , m_statusBadge(new QLabel(this))
    , m_priorityBadge(new QLabel(this))
    , m_categoryBadge(new QLabel(this))
    , m_overdueBadge(new QLabel(tr("已逾期"), this))
    , m_scroll(new QScrollArea(this))
    , m_description(new QLabel(this))
    , m_scheduleGrid(new QGridLayout)
    , m_deadlineField(nullptr)
    , m_estimateField(nullptr)
    , m_createdField(nullptr)
    , m_updatedField(nullptr)
    , m_deadline(nullptr)
    , m_estimate(nullptr)
    , m_created(nullptr)
    , m_updated(nullptr)
    , m_predecessorCount(nullptr)
    , m_unlockCount(nullptr)
    , m_recommendationBlock(nullptr)
    , m_recommendation(nullptr)
    , m_blockingBlock(nullptr)
    , m_blocking(nullptr)
    , m_edit(new QPushButton(tr("编辑任务"), this))
    , m_editDependencies(new QPushButton(tr("编辑前置任务"), this))
{
    setObjectName(QStringLiteral("taskDetailsDialog"));
    setWindowTitle(tr("任务详情"));
    setModal(true);
    resize(640, 620);
    setMinimumSize(480, 480);

    m_title->setObjectName(QStringLiteral("taskDetailsTitle"));
    m_title->setWordWrap(true);
    m_statusBadge->setObjectName(QStringLiteral("taskDetailsStatusBadge"));
    m_priorityBadge->setObjectName(QStringLiteral("taskDetailsPriorityBadge"));
    m_categoryBadge->setObjectName(QStringLiteral("taskDetailsCategoryBadge"));
    m_overdueBadge->setObjectName(QStringLiteral("taskDetailsOverdueBadge"));
    m_scroll->setObjectName(QStringLiteral("taskDetailsScrollView"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_description->setObjectName(QStringLiteral("taskDetailsDescription"));
    m_description->setWordWrap(true);
    m_description->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_edit->setObjectName(QStringLiteral("editSelectedTaskButton"));
    m_editDependencies->setObjectName(
        QStringLiteral("editSelectedDependenciesButton"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *header = new QFrame(this);
    header->setObjectName(QStringLiteral("taskDetailsHeader"));
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(22, 14, 22, 15);
    headerLayout->setSpacing(7);
    auto *eyebrow = new QLabel(tr("任务详情"), header);
    eyebrow->setObjectName(QStringLiteral("taskDetailsEyebrow"));
    headerLayout->addWidget(eyebrow);
    headerLayout->addWidget(m_title);
    auto *badges = new QHBoxLayout;
    badges->setContentsMargins(0, 0, 0, 0);
    badges->setSpacing(7);
    badges->addWidget(m_statusBadge);
    badges->addWidget(m_priorityBadge);
    badges->addWidget(m_categoryBadge);
    badges->addWidget(m_overdueBadge);
    badges->addStretch();
    headerLayout->addLayout(badges);
    root->addWidget(header);

    auto *content = new QWidget(m_scroll);
    content->setObjectName(QStringLiteral("taskDetailsContent"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(18, 16, 18, 18);
    contentLayout->setSpacing(14);

    QVBoxLayout *descriptionLayout = nullptr;
    auto *descriptionCard = sectionCard(
        content, QStringLiteral("taskDetailsDescriptionSection"),
        descriptionLayout);
    descriptionLayout->addWidget(sectionTitle(tr("任务说明"), descriptionCard));
    descriptionLayout->addWidget(m_description);
    contentLayout->addWidget(descriptionCard);

    QVBoxLayout *scheduleLayout = nullptr;
    auto *scheduleCard = sectionCard(
        content, QStringLiteral("taskDetailsScheduleSection"), scheduleLayout);
    scheduleLayout->addWidget(sectionTitle(tr("时间信息"), scheduleCard));
    m_scheduleGrid->setContentsMargins(0, 0, 0, 0);
    m_scheduleGrid->setHorizontalSpacing(10);
    m_scheduleGrid->setVerticalSpacing(10);
    m_deadlineField = metadataField(
        scheduleCard, tr("截止时间"),
        QStringLiteral("taskDetailsDeadlineValue"), m_deadline);
    m_estimateField = metadataField(
        scheduleCard, tr("预计用时"),
        QStringLiteral("taskDetailsEstimateValue"), m_estimate);
    m_createdField = metadataField(
        scheduleCard, tr("创建时间"),
        QStringLiteral("taskDetailsCreatedValue"), m_created);
    m_updatedField = metadataField(
        scheduleCard, tr("更新时间"),
        QStringLiteral("taskDetailsUpdatedValue"), m_updated);
    scheduleLayout->addLayout(m_scheduleGrid);
    contentLayout->addWidget(scheduleCard);

    QVBoxLayout *insightLayout = nullptr;
    auto *insightCard = sectionCard(
        content, QStringLiteral("taskDetailsInsightSection"), insightLayout);
    insightLayout->addWidget(sectionTitle(tr("计划洞察"), insightCard));
    auto *metrics = new QHBoxLayout;
    metrics->setContentsMargins(0, 0, 0, 0);
    metrics->setSpacing(10);
    metrics->addWidget(metricCard(insightCard, tr("直接前置"),
                                  QStringLiteral("taskDetailsPredecessorCount"),
                                  m_predecessorCount));
    metrics->addWidget(metricCard(insightCard, tr("可解锁"),
                                  QStringLiteral("taskDetailsUnlockCount"),
                                  m_unlockCount));
    metrics->addStretch();
    insightLayout->addLayout(metrics);

    m_recommendationBlock = new QFrame(insightCard);
    m_recommendationBlock->setObjectName(
        QStringLiteral("taskDetailsRecommendationBlock"));
    auto *recommendationLayout = new QVBoxLayout(m_recommendationBlock);
    recommendationLayout->setContentsMargins(12, 9, 12, 10);
    recommendationLayout->setSpacing(3);
    auto *recommendationCaption = new QLabel(tr("推荐依据"), m_recommendationBlock);
    recommendationCaption->setObjectName(
        QStringLiteral("taskDetailsInsightCaption"));
    m_recommendation = new QLabel(m_recommendationBlock);
    m_recommendation->setObjectName(
        QStringLiteral("taskDetailsRecommendationText"));
    m_recommendation->setWordWrap(true);
    recommendationLayout->addWidget(recommendationCaption);
    recommendationLayout->addWidget(m_recommendation);
    insightLayout->addWidget(m_recommendationBlock);

    m_blockingBlock = new QFrame(insightCard);
    m_blockingBlock->setObjectName(QStringLiteral("taskDetailsBlockingBlock"));
    auto *blockingLayout = new QVBoxLayout(m_blockingBlock);
    blockingLayout->setContentsMargins(12, 9, 12, 10);
    blockingLayout->setSpacing(3);
    auto *blockingCaption = new QLabel(tr("当前阻塞"), m_blockingBlock);
    blockingCaption->setObjectName(QStringLiteral("taskDetailsInsightCaption"));
    m_blocking = new QLabel(m_blockingBlock);
    m_blocking->setObjectName(QStringLiteral("taskDetailsBlockingText"));
    m_blocking->setWordWrap(true);
    blockingLayout->addWidget(blockingCaption);
    blockingLayout->addWidget(m_blocking);
    insightLayout->addWidget(m_blockingBlock);
    contentLayout->addWidget(insightCard);
    contentLayout->addStretch();

    m_scroll->setWidget(content);
    root->addWidget(m_scroll, 1);

    auto *footer = new QFrame(this);
    footer->setObjectName(QStringLiteral("taskDetailsFooter"));
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(18, 11, 18, 11);
    footerLayout->setSpacing(9);
    auto *close = new QPushButton(tr("关闭"), footer);
    close->setObjectName(QStringLiteral("closeTaskDetailsButton"));
    footerLayout->addStretch();
    footerLayout->addWidget(close);
    footerLayout->addWidget(m_editDependencies);
    footerLayout->addWidget(m_edit);
    root->addWidget(footer);

    updateResponsiveLayout();
    connect(close, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_edit, &QPushButton::clicked, this, [this] {
        const QString id = m_details.selectedTaskId();
        accept();
        emit editRequested(id);
    });
    connect(m_editDependencies, &QPushButton::clicked, this, [this] {
        const QString id = m_details.selectedTaskId();
        accept();
        emit editDependenciesRequested(id);
    });
    // Contract 通知只触发展示 getter 重读，不允许详情窗口访问具体 ViewModel。
    connect(&m_details, &viewmodel::TaskDetailsContract::selectionChanged,
            this, &TaskDetailsDialog::synchronize);
    synchronize();
}

bool TaskDetailsDialog::openTask(const QString &taskId)
{
    if (!m_details.selectTask(taskId)) return false;
    if (parentWidget()) {
        const int availableWidth = std::max(minimumWidth(),
                                            parentWidget()->width() - 48);
        const int availableHeight = std::max(minimumHeight(),
                                             parentWidget()->height() - 48);
        resize(std::min(640, availableWidth), std::min(620, availableHeight));
    }
    updateResponsiveLayout();
    open();
    return true;
}

void TaskDetailsDialog::setActionsVisible(const bool visible)
{
    m_actionsVisible = visible;
    synchronize();
}

void TaskDetailsDialog::done(const int result)
{
    QDialog::done(result);
    // 关闭即释放详情会话选择，避免下次打开短暂显示上一任务。
    m_details.clearSelection();
}

void TaskDetailsDialog::changeEvent(QEvent *event)
{
    QDialog::changeEvent(event);
    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::FontChange) {
        applyPresentationStyle();
    }
}

void TaskDetailsDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    updateResponsiveLayout();
}

void TaskDetailsDialog::updateResponsiveLayout()
{
    if (!m_deadlineField || !m_scroll) return;
    for (QWidget *field : {m_deadlineField, m_estimateField,
                           m_createdField, m_updatedField}) {
        m_scheduleGrid->removeWidget(field);
    }
    // 对话框外宽扣除内容区左右边距后近似可用宽度；避免首次 show 前
    // viewport 尚未完成布局时错误进入单列并停留在该状态。
    const int contentWidth = width() - 36;
    if (contentWidth < 560) {
        m_scheduleGrid->addWidget(m_deadlineField, 0, 0);
        m_scheduleGrid->addWidget(m_estimateField, 1, 0);
        m_scheduleGrid->addWidget(m_createdField, 2, 0);
        m_scheduleGrid->addWidget(m_updatedField, 3, 0);
        m_scheduleGrid->setColumnStretch(0, 1);
        m_scheduleGrid->setColumnStretch(1, 0);
    } else {
        m_scheduleGrid->addWidget(m_deadlineField, 0, 0);
        m_scheduleGrid->addWidget(m_estimateField, 0, 1);
        m_scheduleGrid->addWidget(m_createdField, 1, 0);
        m_scheduleGrid->addWidget(m_updatedField, 1, 1);
        m_scheduleGrid->setColumnStretch(0, 1);
        m_scheduleGrid->setColumnStretch(1, 1);
    }
}

void TaskDetailsDialog::synchronize()
{
    // 文字、语义色索引和命令资格均只读 Contract，View 不解析状态文案。
    m_title->setText(m_details.selectedTitle());
    m_statusBadge->setText(m_details.selectedStatusText());
    m_priorityBadge->setText(
        tr("%1优先级").arg(m_details.selectedPriorityText()));
    m_categoryBadge->setText(m_details.selectedCategoryName());
    m_categoryBadge->setVisible(m_details.selectedHasCategory());
    m_overdueBadge->setVisible(m_details.selectedOverdue());
    m_description->setText(m_details.selectedDescription().isEmpty()
        ? tr("暂无描述") : m_details.selectedDescription());
    m_deadline->setText(m_details.selectedDeadlineText().isEmpty()
        ? tr("未设置") : m_details.selectedDeadlineText());
    m_estimate->setText(m_details.selectedEstimatedMinutes() > 0
        ? tr("%1 分钟").arg(m_details.selectedEstimatedMinutes())
        : tr("未设置"));
    m_created->setText(m_details.selectedCreatedAtText());
    m_updated->setText(m_details.selectedUpdatedAtText());
    m_predecessorCount->setText(
        QString::number(m_details.selectedPredecessorCount()));
    m_unlockCount->setText(QString::number(m_details.selectedUnlockCount()));

    const bool blocked = !m_details.selectedBlockingReasonText().isEmpty();
    m_blocking->setText(m_details.selectedBlockingReasonText());
    m_blockingBlock->setVisible(blocked);
    m_recommendation->setText(m_details.selectedReasonText().isEmpty()
        ? tr("暂无推荐信息") : m_details.selectedReasonText());
    m_recommendationBlock->setVisible(!blocked);
    m_edit->setVisible(m_actionsVisible && m_details.selectedCanEditTask());
    m_editDependencies->setVisible(
        m_actionsVisible && m_details.selectedCanEditDependencies());
    applyPresentationStyle();
}

void TaskDetailsDialog::applyPresentationStyle()
{
    // 动态徽标只解释强类型展示值；重入保护避免 QSS 更新形成事件递归。
    if (m_applyingStyle) return;
    m_applyingStyle = true;
    const WidgetTheme theme = WidgetTheme::fromPalette(palette());
    styleBadge(m_statusBadge, theme.statusColor(m_details.selectedStatusVisual()));
    styleBadge(m_priorityBadge,
               theme.priorityColor(m_details.selectedPriorityVisual()));
    QColor categoryColor(m_details.selectedCategoryAccent());
    if (!categoryColor.isValid()) categoryColor = theme.primary;
    styleBadge(m_categoryBadge, categoryColor);
    styleBadge(m_overdueBadge, theme.danger);

    m_recommendationBlock->setStyleSheet(QStringLiteral(
        "QFrame#taskDetailsRecommendationBlock { background: %1; border: 1px solid %2; "
        "border-radius: 8px; }"
        "QFrame#taskDetailsRecommendationBlock QLabel { color: %2; border: none; "
        "background: transparent; }")
        .arg(rgba(theme.primary, 18), theme.primary.name()));
    m_blockingBlock->setStyleSheet(QStringLiteral(
        "QFrame#taskDetailsBlockingBlock { background: %1; border: 1px solid %2; "
        "border-radius: 8px; }"
        "QFrame#taskDetailsBlockingBlock QLabel { color: %2; border: none; "
        "background: transparent; }")
        .arg(rgba(theme.warning, 18), theme.warning.name()));
    m_applyingStyle = false;
}

} // namespace smartmate::view::widgets
