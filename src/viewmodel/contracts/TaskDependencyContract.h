#pragma once

#include "common/presentation/UiNotification.h"

#include <QAbstractListModel>

namespace smartmate::viewmodel {

/// 单任务前置依赖编辑会话的抽象展示契约。
class TaskDependencyContract : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString taskId READ taskId NOTIFY contextChanged)
    Q_PROPERTY(QString taskTitle READ taskTitle NOTIFY contextChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY formStateChanged)
    Q_PROPERTY(bool canSave READ canSave NOTIFY formStateChanged)

public:
    enum Role {
        TaskIdRole = Qt::UserRole + 1,
        ShortIdRole,
        TitleRole,
        StatusTextRole,
        PriorityTextRole,
        SelectedRole,
        ArchivedRole,
        SelectableRole,
        CategoryNameRole,
        CategoryAccentRole,
        HasCategoryRole,
    };
    Q_ENUM(Role)

    ~TaskDependencyContract() override = default;

    [[nodiscard]] virtual QString taskId() const = 0;
    [[nodiscard]] virtual QString taskTitle() const = 0;
    [[nodiscard]] virtual int count() const noexcept = 0;
    [[nodiscard]] virtual int selectedCount() const noexcept = 0;
    [[nodiscard]] virtual bool dirty() const noexcept = 0;
    [[nodiscard]] virtual bool canSave() const noexcept = 0;

public slots:
    virtual bool beginEdit(const QString &taskId) = 0;
    virtual bool setPredecessorSelected(const QString &predecessorTaskId,
                                        bool selected) = 0;
    virtual bool save() = 0;
    virtual void cancel() = 0;

signals:
    void contextChanged();
    void countChanged();
    void selectionChanged();
    void formStateChanged();
    void saved(const QString &taskId);
    void cancelled();
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    explicit TaskDependencyContract(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
};

} // namespace smartmate::viewmodel
