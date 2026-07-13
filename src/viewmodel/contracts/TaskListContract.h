#pragma once

#include "common/presentation/UiNotification.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>

namespace smartmate::viewmodel {

/// 任务列表、筛选、批量选择和状态命令的抽象展示契约。
///
/// 焦点任务与详情选择分别由 TaskFocusContract、TaskDetailsContract 提供。
class TaskListContract : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool showArchived READ showArchived WRITE setShowArchived NOTIFY showArchivedChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(int priorityFilterIndex READ priorityFilterIndex WRITE setPriorityFilterIndex
                   NOTIFY priorityFilterIndexChanged)
    Q_PROPERTY(QStringList priorityFilterOptions READ priorityFilterOptions CONSTANT)
    Q_PROPERTY(QVariantList categoryFilterOptions READ categoryFilterOptions
                   NOTIFY categoryOptionsChanged)
    Q_PROPERTY(int categoryFilterMode READ categoryFilterMode NOTIFY categoryFilterChanged)
    Q_PROPERTY(QString categoryFilterCategoryId READ categoryFilterCategoryId
                   NOTIFY categoryFilterChanged)
    Q_PROPERTY(bool hasActiveFilters READ hasActiveFilters NOTIFY hasActiveFiltersChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool bulkSelectionMode READ bulkSelectionMode NOTIFY bulkSelectionChanged)
    Q_PROPERTY(int bulkSelectedCount READ bulkSelectedCount NOTIFY bulkSelectionChanged)
    Q_PROPERTY(int bulkSelectableVisibleCount READ bulkSelectableVisibleCount
                   NOTIFY bulkSelectionChanged)
    Q_PROPERTY(bool allVisibleSelected READ allVisibleSelected NOTIFY bulkSelectionChanged)
    Q_PROPERTY(bool canBulkArchive READ canBulkArchive NOTIFY bulkSelectionChanged)
    Q_PROPERTY(bool canBulkRestore READ canBulkRestore NOTIFY bulkSelectionChanged)
    Q_PROPERTY(bool canBulkDelete READ canBulkDelete NOTIFY bulkSelectionChanged)

public:
    enum Role {
        TaskIdRole = Qt::UserRole + 1,
        TitleRole,
        DescriptionRole,
        StatusRole,
        StatusTextRole,
        PriorityRole,
        PriorityTextRole,
        DeadlineTextRole,
        EstimatedMinutesRole,
        ArchivedRole,
        OverdueRole,
        OrderReasonTextRole,
        BlockedRole,
        BlockingReasonTextRole,
        PredecessorCountRole,
        UnlockCountRole,
        CanEditTaskRole,
        CanEditDependenciesRole,
        CanStartRole,
        CanCancelRole,
        CanCompleteRole,
        CanRedoRole,
        CanArchiveRole,
        CanRestoreRole,
        CanDeletePermanentlyRole,
        BulkSelectedRole,
        BulkSelectableRole,
        CategoryIdRole,
        CategoryNameRole,
        CategoryAccentRole,
        HasCategoryRole,
    };
    Q_ENUM(Role)

    ~TaskListContract() override = default;

    [[nodiscard]] virtual int count() const noexcept = 0;
    [[nodiscard]] virtual bool showArchived() const noexcept = 0;
    [[nodiscard]] virtual QString searchText() const = 0;
    [[nodiscard]] virtual int priorityFilterIndex() const noexcept = 0;
    [[nodiscard]] virtual QStringList priorityFilterOptions() const = 0;
    [[nodiscard]] virtual QVariantList categoryFilterOptions() const = 0;
    [[nodiscard]] virtual int categoryFilterMode() const noexcept = 0;
    [[nodiscard]] virtual QString categoryFilterCategoryId() const = 0;
    [[nodiscard]] virtual bool hasActiveFilters() const = 0;
    [[nodiscard]] virtual bool bulkSelectionMode() const noexcept = 0;
    [[nodiscard]] virtual int bulkSelectedCount() const noexcept = 0;
    [[nodiscard]] virtual int bulkSelectableVisibleCount() const = 0;
    [[nodiscard]] virtual bool allVisibleSelected() const = 0;
    [[nodiscard]] virtual bool canBulkArchive() const noexcept = 0;
    [[nodiscard]] virtual bool canBulkRestore() const = 0;
    [[nodiscard]] virtual bool canBulkDelete() const noexcept = 0;
    virtual void setShowArchived(bool showArchived) = 0;
    virtual void setSearchText(const QString &searchText) = 0;
    virtual void setPriorityFilterIndex(int priorityFilterIndex) = 0;

public slots:
    virtual bool setCategoryFilter(int mode, const QString &categoryId = {}) = 0;
    virtual void reload() = 0;
    virtual void clearFilters() = 0;
    virtual bool startTask(const QString &taskId) = 0;
    virtual bool cancelTask(const QString &taskId) = 0;
    virtual bool completeTask(const QString &taskId) = 0;
    virtual bool redoTask(const QString &taskId) = 0;
    virtual bool archiveTask(const QString &taskId) = 0;
    virtual bool restoreTask(const QString &taskId) = 0;
    virtual bool deleteArchivedTask(const QString &taskId) = 0;
    virtual void beginBulkSelection() = 0;
    virtual bool toggleBulkSelection(const QString &taskId) = 0;
    virtual void toggleSelectAllVisible() = 0;
    virtual void clearBulkSelection() = 0;
    virtual void cancelBulkSelection() = 0;
    virtual bool archiveSelectedTasks() = 0;
    virtual bool restoreSelectedTasks() = 0;
    virtual bool deleteSelectedArchivedTasks() = 0;

signals:
    void showArchivedChanged();
    void searchTextChanged();
    void priorityFilterIndexChanged();
    void categoryOptionsChanged();
    void categoryFilterChanged();
    void hasActiveFiltersChanged();
    void countChanged();
    void bulkSelectionChanged();
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    explicit TaskListContract(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
};

} // namespace smartmate::viewmodel
