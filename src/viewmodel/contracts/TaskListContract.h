#pragma once

#include "common/presentation/UiNotification.h"
#include "viewmodel/contracts/TaskPresentationTypes.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>

namespace smartmate::viewmodel {

/// 任务列表、筛选、批量选择和状态命令的抽象展示契约。
///
/// 焦点任务与详情选择分别由 TaskFocusContract、TaskDetailsContract 提供。
/// 主模型 Role 是 Widget 的唯一任务列表绑定面；搜索、筛选和批量选择是 ViewModel
/// 会话状态，不持久化。所有写命令使用稳定 TaskId，并由 Model 最终复核资格。
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
    /// 稳定列表 Role；身份、展示字段、派生状态、命令资格和批量选择均由 ViewModel 投影。
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
        // 以下资格 Role 由 Model 计算、ViewModel 投影，Widget 只能据此启用命令控件。
        CanEditTaskRole,
        CanEditDependenciesRole,
        CanStartRole,
        CanCancelRole,
        CanCompleteRole,
        CanRedoRole,
        CanArchiveRole,
        CanRestoreRole,
        CanDeletePermanentlyRole,
        // 批量选择是 ViewModel 会话状态，身份仍由 TaskIdRole 保持稳定。
        BulkSelectedRole,
        BulkSelectableRole,
        // 类别只投影稳定 ID 与展示信息，不把 TaskCategory 领域实体交给 View。
        CategoryIdRole,
        CategoryNameRole,
        CategoryAccentRole,
        HasCategoryRole,
    };
    Q_ENUM(Role)

    ~TaskListContract() override = default;

    // getter 提供过滤与批量会话的当前快照；绑定建立时先读取，再监听对应通知。
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
    /// 更新“显示归档”会话筛选，不写入 QSettings 或 Repository。
    virtual void setShowArchived(bool showArchived) = 0;
    /// 更新搜索词并重建可见投影；不改变 Model 已有推荐顺序。
    virtual void setSearchText(const QString &searchText) = 0;
    /// 更新优先级筛选索引；具体实现负责范围检查。
    virtual void setPriorityFilterIndex(int priorityFilterIndex) = 0;

public slots:
    /// 更新类别筛选模式与稳定类别 ID；非法组合返回 false。
    virtual bool setCategoryFilter(int mode, const QString &categoryId = {}) = 0;
    /// 从 Service 重建推荐计划和列表投影；通常由 Model 失效通知触发。
    virtual void reload() = 0;
    /// 一次清除全部搜索、优先级、类别及归档筛选会话状态。
    virtual void clearFilters() = 0;
    /// 以下状态命令使用稳定 TaskId；返回值仅表示命令是否成功，错误通过通知报告。
    virtual bool startTask(const QString &taskId) = 0;
    virtual bool cancelTask(const QString &taskId) = 0;
    virtual bool completeTask(const QString &taskId) = 0;
    virtual bool redoTask(const QString &taskId) = 0;
    virtual bool archiveTask(const QString &taskId) = 0;
    virtual bool restoreTask(const QString &taskId) = 0;
    virtual bool deleteArchivedTask(const QString &taskId) = 0;
    /// 进入批量选择会话；选择集合只保存在 ViewModel。
    virtual void beginBulkSelection() = 0;
    /// 切换单个稳定 TaskId 的批量选择；不可选或不可见时返回 false。
    virtual bool toggleBulkSelection(const QString &taskId) = 0;
    /// 按当前可见且可选集合全选或全不选，不使用列表行号保存身份。
    virtual void toggleSelectAllVisible() = 0;
    /// 清空选择但保留批量模式。
    virtual void clearBulkSelection() = 0;
    /// 退出批量模式并清空选择。
    virtual void cancelBulkSelection() = 0;
    /// 以下命令各自一次提交完整稳定 ID 集合，禁止 Widget 循环调用单项命令。
    virtual bool archiveSelectedTasks() = 0;
    virtual bool restoreSelectedTasks() = 0;
    virtual bool deleteSelectedArchivedTasks() = 0;

signals:
    /// 以下筛选通知只表示会话投影变化，Widget 收到后重读 getter。
    void showArchivedChanged();
    void searchTextChanged();
    void priorityFilterIndexChanged();
    void categoryOptionsChanged();
    void categoryFilterChanged();
    void hasActiveFiltersChanged();
    /// 可见任务数量变化；行增删和 Role 更新仍使用 QAbstractItemModel 标准信号。
    void countChanged();
    /// 批量模式、选择集合、计数或批量命令资格变化后发送。
    void bulkSelectionChanged();
    /// 请求 View 展示命令成功、失败或确认结果；不承担列表数据同步。
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    /// 仅供具体列表 ViewModel 构造，View 不得创建实现或接触 Service。
    explicit TaskListContract(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
};

} // namespace smartmate::viewmodel
