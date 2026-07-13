#pragma once

#include "common/presentation/UiNotification.h"

#include <QAbstractListModel>
#include <QStringList>

namespace smartmate::viewmodel {

/// 类别目录和单类别编辑会话的抽象展示契约。
class TaskCategoryContract : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY countChanged)
    Q_PROPERTY(bool editMode READ editMode NOTIFY draftChanged)
    Q_PROPERTY(QString editingCategoryId READ editingCategoryId NOTIFY draftChanged)
    Q_PROPERTY(QString draftName READ draftName WRITE setDraftName NOTIFY draftChanged)
    Q_PROPERTY(int draftColorIndex READ draftColorIndex WRITE setDraftColorIndex
                   NOTIFY draftChanged)
    Q_PROPERTY(QStringList colorOptions READ colorOptions CONSTANT)
    Q_PROPERTY(QStringList colorAccents READ colorAccents CONSTANT)
    Q_PROPERTY(bool dirty READ dirty NOTIFY draftChanged)
    Q_PROPERTY(bool canSave READ canSave NOTIFY draftChanged)

public:
    enum Role {
        CategoryIdRole = Qt::UserRole + 1,
        NameRole,
        ColorIndexRole,
        AccentRole,
        TaskCountRole,
    };
    Q_ENUM(Role)

    ~TaskCategoryContract() override = default;

    [[nodiscard]] virtual int count() const noexcept = 0;
    [[nodiscard]] virtual bool empty() const noexcept = 0;
    [[nodiscard]] virtual bool editMode() const noexcept = 0;
    [[nodiscard]] virtual QString editingCategoryId() const = 0;
    [[nodiscard]] virtual QString draftName() const = 0;
    virtual void setDraftName(const QString &name) = 0;
    [[nodiscard]] virtual int draftColorIndex() const noexcept = 0;
    virtual void setDraftColorIndex(int index) = 0;
    [[nodiscard]] virtual QStringList colorOptions() const = 0;
    [[nodiscard]] virtual QStringList colorAccents() const = 0;
    [[nodiscard]] virtual bool dirty() const noexcept = 0;
    [[nodiscard]] virtual bool canSave() const noexcept = 0;

public slots:
    virtual void reload() = 0;
    virtual void beginCreate() = 0;
    virtual bool beginEdit(const QString &categoryId) = 0;
    virtual bool save() = 0;
    virtual bool deleteCategory(const QString &categoryId) = 0;
    virtual void cancel() = 0;

signals:
    void countChanged();
    void draftChanged();
    void saved(const QString &categoryId);
    void deleted(const QString &categoryId, int unassignedTaskCount);
    void cancelled();
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    explicit TaskCategoryContract(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
};

} // namespace smartmate::viewmodel
