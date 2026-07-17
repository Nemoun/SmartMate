#pragma once

#include "viewmodel/contracts/FocusContract.h"

#include <QList>
#include <QString>

namespace smartmate::viewmodel {

/// 一条已完成专注的纯展示投影；不保存领域对象或 Repository 行。
struct FocusHistoryRow final {
    QString sessionId;
    QString taskId;
    QString taskTitle;
    qint64 durationMilliseconds{0};
    QString durationText;
    QString startedAtText;
    QString completedAtText;
    QString categoryName;
    FocusContract::CategoryColor categoryColor{FocusContract::Unclassified};
    QString tooltip;
    QString accessibleText;

    friend bool operator==(const FocusHistoryRow &,
                           const FocusHistoryRow &) = default;
};

/// FocusViewModel 拥有的历史列表实现；View 只能通过抽象 Contract 读取。
class FocusHistoryListModel final : public FocusHistoryContract {
    Q_OBJECT

public:
    explicit FocusHistoryListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    /// 内容相同不重置模型，保证秒级计时不会扰动历史列表。
    [[nodiscard]] bool replaceRows(QList<FocusHistoryRow> rows);

private:
    QList<FocusHistoryRow> m_rows;
};

} // namespace smartmate::viewmodel
