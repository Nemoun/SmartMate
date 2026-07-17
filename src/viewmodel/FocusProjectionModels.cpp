#include "FocusProjectionModels.h"

#include <QVariant>

namespace smartmate::viewmodel {

FocusHistoryListModel::FocusHistoryListModel(QObject *parent)
    : FocusHistoryContract(parent)
{
}

int FocusHistoryListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant FocusHistoryListModel::data(const QModelIndex &index,
                                     const int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const FocusHistoryRow &row = m_rows.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case TaskTitleRole: return row.taskTitle;
    case SessionIdRole: return row.sessionId;
    case TaskIdRole: return row.taskId;
    case DurationMillisecondsRole: return row.durationMilliseconds;
    case DurationTextRole: return row.durationText;
    case StartedAtTextRole: return row.startedAtText;
    case CompletedAtTextRole: return row.completedAtText;
    case CategoryNameRole: return row.categoryName;
    case CategoryColorRole: return static_cast<int>(row.categoryColor);
    case TooltipRole: return row.tooltip;
    case AccessibleTextRole: return row.accessibleText;
    default: return {};
    }
}

QHash<int, QByteArray> FocusHistoryListModel::roleNames() const
{
    return {
        {SessionIdRole, QByteArrayLiteral("sessionId")},
        {TaskIdRole, QByteArrayLiteral("taskId")},
        {TaskTitleRole, QByteArrayLiteral("taskTitle")},
        {DurationMillisecondsRole, QByteArrayLiteral("durationMilliseconds")},
        {DurationTextRole, QByteArrayLiteral("durationText")},
        {StartedAtTextRole, QByteArrayLiteral("startedAtText")},
        {CompletedAtTextRole, QByteArrayLiteral("completedAtText")},
        {CategoryNameRole, QByteArrayLiteral("categoryName")},
        {CategoryColorRole, QByteArrayLiteral("categoryColor")},
        {TooltipRole, QByteArrayLiteral("tooltip")},
        {AccessibleTextRole, QByteArrayLiteral("accessibleText")},
    };
}

bool FocusHistoryListModel::replaceRows(QList<FocusHistoryRow> rows)
{
    if (rows == m_rows) return false;
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
    return true;
}

} // namespace smartmate::viewmodel
