#pragma once

#include "domain/Task.h"
#include "domain/TaskStateMachine.h"
#include "planner/TaskOrderingPolicy.h"
#include "viewmodel/contracts/TaskListContract.h"

#include <QHash>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

namespace smartmate::model {
class TaskService;
class TaskCategoryService;
}

namespace smartmate::viewmodel {

/// 将领域任务集合投影成 QML 可绑定的列表；它负责展示格式与命令转发，
/// 不拥有任务真相，也不直接访问 Repository。
class TaskListViewModel final : public TaskListContract {
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(FocusState focusState READ focusState NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusTaskId READ focusTaskId NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusTitle READ focusTitle NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusDescription READ focusDescription NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusStatusText READ focusStatusText NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusPriorityText READ focusPriorityText NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusDeadlineText READ focusDeadlineText NOTIFY focusTaskChanged)
    Q_PROPERTY(int focusEstimatedMinutes READ focusEstimatedMinutes NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusReasonText READ focusReasonText NOTIFY focusTaskChanged)
    Q_PROPERTY(bool focusOverdue READ focusOverdue NOTIFY focusTaskChanged)
    Q_PROPERTY(bool focusCanStart READ focusCanStart NOTIFY focusTaskChanged)
    Q_PROPERTY(bool focusCanComplete READ focusCanComplete NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusCategoryName READ focusCategoryName NOTIFY focusTaskChanged)
    Q_PROPERTY(QString focusCategoryAccent READ focusCategoryAccent NOTIFY focusTaskChanged)
    Q_PROPERTY(bool focusHasCategory READ focusHasCategory NOTIFY focusTaskChanged)
    Q_PROPERTY(QString selectedTaskId READ selectedTaskId NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDescription READ selectedDescription NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedStatusText READ selectedStatusText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedPriorityText READ selectedPriorityText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedDeadlineText READ selectedDeadlineText NOTIFY selectionChanged)
    Q_PROPERTY(int selectedEstimatedMinutes READ selectedEstimatedMinutes NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCreatedAtText READ selectedCreatedAtText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedUpdatedAtText READ selectedUpdatedAtText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedReasonText READ selectedReasonText NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedBlockingReasonText READ selectedBlockingReasonText
                   NOTIFY selectionChanged)
    Q_PROPERTY(int selectedPredecessorCount READ selectedPredecessorCount NOTIFY selectionChanged)
    Q_PROPERTY(int selectedUnlockCount READ selectedUnlockCount NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedCanEditTask READ selectedCanEditTask NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedCanEditDependencies READ selectedCanEditDependencies
                   NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCategoryName READ selectedCategoryName NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCategoryAccent READ selectedCategoryAccent NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedHasCategory READ selectedHasCategory NOTIFY selectionChanged)
    QML_NAMED_ELEMENT(TaskListViewModel)
    QML_UNCREATABLE("TaskListViewModel is owned by AppViewModel")

public:
    enum class FocusState {
        NoTasks = 0,
        Suggested = 1,
        InProgress = 2,
        AllBlocked = 3,
    };
    Q_ENUM(FocusState)

    explicit TaskListViewModel(model::TaskService &taskService, QObject *parent = nullptr);
    TaskListViewModel(model::TaskService &taskService,
                      model::TaskCategoryService &categoryService,
                      QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] int count() const noexcept override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] bool showArchived() const noexcept override;
    [[nodiscard]] QString searchText() const override;
    [[nodiscard]] int priorityFilterIndex() const noexcept override;
    [[nodiscard]] QStringList priorityFilterOptions() const override;
    [[nodiscard]] QVariantList categoryFilterOptions() const override;
    [[nodiscard]] int categoryFilterMode() const noexcept override;
    [[nodiscard]] QString categoryFilterCategoryId() const override;
    [[nodiscard]] bool hasActiveFilters() const override;
    [[nodiscard]] bool bulkSelectionMode() const noexcept override;
    [[nodiscard]] int bulkSelectedCount() const noexcept override;
    [[nodiscard]] int bulkSelectableVisibleCount() const override;
    [[nodiscard]] bool allVisibleSelected() const override;
    [[nodiscard]] bool canBulkArchive() const noexcept override;
    [[nodiscard]] bool canBulkRestore() const override;
    [[nodiscard]] bool canBulkDelete() const noexcept override;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] FocusState focusState() const noexcept;
    [[nodiscard]] QString focusTaskId() const;
    [[nodiscard]] QString focusTitle() const;
    [[nodiscard]] QString focusDescription() const;
    [[nodiscard]] QString focusStatusText() const;
    [[nodiscard]] QString focusPriorityText() const;
    [[nodiscard]] QString focusDeadlineText() const;
    [[nodiscard]] int focusEstimatedMinutes() const noexcept;
    [[nodiscard]] QString focusReasonText() const;
    [[nodiscard]] bool focusOverdue() const noexcept;
    [[nodiscard]] bool focusCanStart() const noexcept;
    [[nodiscard]] bool focusCanComplete() const noexcept;
    [[nodiscard]] QString focusCategoryName() const;
    [[nodiscard]] QString focusCategoryAccent() const;
    [[nodiscard]] bool focusHasCategory() const noexcept;
    [[nodiscard]] QString selectedTaskId() const;
    [[nodiscard]] QString selectedTitle() const;
    [[nodiscard]] QString selectedDescription() const;
    [[nodiscard]] QString selectedStatusText() const;
    [[nodiscard]] QString selectedPriorityText() const;
    [[nodiscard]] QString selectedDeadlineText() const;
    [[nodiscard]] int selectedEstimatedMinutes() const noexcept;
    [[nodiscard]] QString selectedCreatedAtText() const;
    [[nodiscard]] QString selectedUpdatedAtText() const;
    [[nodiscard]] QString selectedReasonText() const;
    [[nodiscard]] QString selectedBlockingReasonText() const;
    [[nodiscard]] int selectedPredecessorCount() const noexcept;
    [[nodiscard]] int selectedUnlockCount() const noexcept;
    [[nodiscard]] bool selectedCanEditTask() const noexcept;
    [[nodiscard]] bool selectedCanEditDependencies() const noexcept;
    [[nodiscard]] QString selectedCategoryName() const;
    [[nodiscard]] QString selectedCategoryAccent() const;
    [[nodiscard]] bool selectedHasCategory() const noexcept;
    void setShowArchived(bool showArchived) override;
    void setSearchText(const QString &searchText) override;
    void setPriorityFilterIndex(int priorityFilterIndex) override;
    /// mode: 0=全部，1=未分类，2=指定类别；指定类别始终使用稳定CategoryId。
    bool setCategoryFilter(int mode, const QString &categoryId = {}) override;

    /// 从Service重新获取快照并重建当前展示投影。
    void reload() override;
    /// 只清除关键字和优先级条件，活动/归档视图保持不变。
    void clearFilters() override;
    /// 请求 Todo → InProgress；阻塞与单进行中约束由 Model 最终判定。
    bool startTask(const QString &taskId) override;
    /// 请求 Todo/InProgress → Cancelled；取消确认由 View 负责。
    bool cancelTask(const QString &taskId) override;
    /// 请求 InProgress → Done，禁止绕过进行中状态。
    bool completeTask(const QString &taskId) override;
    /// 请求 Done/Cancelled → Todo，并由 Model 重新计算依赖有效性。
    bool redoTask(const QString &taskId) override;
    /// 按稳定TaskId请求软归档，并把领域错误映射为展示状态。
    bool archiveTask(const QString &taskId) override;
    /// 按稳定TaskId请求恢复，并保留Service给出的业务约束。
    bool restoreTask(const QString &taskId) override;
    /// 永久删除已归档任务；关联依赖由 Model 的原子删除端口统一清理。
    bool deleteArchivedTask(const QString &taskId) override;
    /// 进入会话级批量选择模式；此时不会自动选择任何任务。
    void beginBulkSelection() override;
    /// 按稳定 TaskId 切换选择；不具备当前批量操作资格的任务会被忽略。
    bool toggleBulkSelection(const QString &taskId) override;
    /// 选择或取消选择当前筛选结果中所有具备批量资格的任务。
    void toggleSelectAllVisible() override;
    /// 清空批量选择但保持批量模式。
    void clearBulkSelection() override;
    /// 清空选择并退出批量模式。
    void cancelBulkSelection() override;
    /// 将选中的已完成/已取消任务作为一个原子批次归档。
    bool archiveSelectedTasks() override;
    /// 将选中的归档任务作为一个原子批次恢复。
    bool restoreSelectedTasks() override;
    /// 将选中的归档任务及其关联依赖作为一个原子批次永久删除。
    bool deleteSelectedArchivedTasks() override;
    Q_INVOKABLE void clearError();
    Q_INVOKABLE bool selectTask(const QString &taskId);
    Q_INVOKABLE void clearSelection();

signals:
    void errorMessageChanged();
    void errorOccurred(const QString &message);
    void focusTaskChanged();
    void selectionChanged();

private:
    /// Model 依赖状态的界面投影；中文原因在 reload() 时一次生成，QML 不拼接图数据。
    struct DependencyProjection {
        bool blocked{false};
        QString blockingReasonText;
        int predecessorCount{0};
        int unlockCount{0};

        bool operator==(const DependencyProjection &) const = default;
    };

    [[nodiscard]] static QString statusText(model::TaskStatus status);
    [[nodiscard]] static QString priorityText(model::TaskPriority priority);
    [[nodiscard]] static model::TaskId parseTaskId(const QString &taskId);
    [[nodiscard]] const model::Task *taskForId(const model::TaskId &taskId) const;
    [[nodiscard]] const model::Task *focusTask() const;
    [[nodiscard]] const model::Task *selectedTask() const;
    [[nodiscard]] const model::TaskCommandAvailability &availabilityFor(
        const model::TaskId &taskId) const;
    [[nodiscard]] bool isBulkSelectable(const model::TaskId &taskId) const;
    [[nodiscard]] QList<model::TaskId> sortedBulkSelection() const;
    [[nodiscard]] QString taskIdsContext(const QList<model::TaskId> &taskIds) const;
    void pruneBulkSelection();
    void setBulkSelection(QSet<model::TaskId> selection);
    void rebuildFocusTask();
    bool performTransition(const QString &taskId, model::TaskTransition transition);
    void rebuildVisibleTasks();
    void setError(const QString &message);
    void reloadCategories();
    [[nodiscard]] const model::TaskCategory *categoryForTask(
        const model::Task *task) const;

    TaskListViewModel(model::TaskService &taskService,
                      model::TaskCategoryService *categoryService,
                      QObject *parent);

    // Service 由组合根拥有；列表只保留非拥有引用并监听其变化通知。
    model::TaskService &m_taskService;
    model::TaskCategoryService *m_categoryService{nullptr};
    // 每分钟重新请求Model计划，使“已逾期”等随时间变化的推荐理由及时刷新。
    QTimer m_reloadTimer;
    // 全量计划顺序与当前可见投影分离，搜索和筛选不会修改领域数据。
    QList<model::Task> m_allTasks;
    QList<model::Task> m_visibleTasks;
    // 当前搜索、筛选与活动/归档范围内的稳定ID集合，用于O(1)批量资格检查。
    QSet<model::TaskId> m_visibleTaskIds;
    QHash<model::TaskId, QString> m_orderReasonTexts;
    // 逾期随当前时间变化，是 Model 计算后交给 ViewModel 的会话级投影。
    QHash<model::TaskId, bool> m_overdueStates;
    QHash<model::TaskId, DependencyProjection> m_dependencyProjections;
    QHash<model::TaskId, model::TaskCommandAvailability> m_availabilities;
    model::TaskId m_focusTaskId;
    FocusState m_focusState{FocusState::NoTasks};
    model::TaskId m_selectedTaskId;
    // 批量选择与详情选择相互独立，只保存稳定 TaskId，且绝不写入持久化层。
    QSet<model::TaskId> m_bulkSelectedTaskIds;
    bool m_bulkSelectionMode{false};
    bool m_showArchived{false};
    QString m_searchText;
    // 0表示全部，1～4分别映射Low～Urgent；非法索引不会替换当前条件。
    int m_priorityFilterIndex{0};
    QList<model::TaskCategory> m_categories;
    /// 0=全部、1=未分类、2=指定类别；筛选状态只存在于当前会话。
    int m_categoryFilterMode{0};
    model::TaskCategoryId m_categoryFilterCategoryId;
    QString m_errorMessage;
};

} // namespace smartmate::viewmodel
