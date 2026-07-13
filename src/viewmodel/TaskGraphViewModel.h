#pragma once

#include "domain/TaskGraph.h"
#include "TaskGraphLayout.h"
#include "viewmodel/contracts/TaskGraphContract.h"

#include <QHash>
#include <QVariantList>

namespace smartmate::model {
class TaskService;
class TaskCategoryService;
}

namespace smartmate::viewmodel {

class TaskGraphEdgeListModel;
class TaskGraphRelationListModel;

/// 将领域依赖图投影为纵向分层节点、正交边路径和只读详情数据。
/// 拓扑层级与上下游闭包来自 Model；像素布局、筛选和交互强调属于 ViewModel。
class TaskGraphViewModel final : public TaskGraphContract {
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
public:
    explicit TaskGraphViewModel(model::TaskService &taskService,
                                QObject *parent = nullptr);
    TaskGraphViewModel(model::TaskService &taskService,
                       model::TaskCategoryService &categoryService,
                       QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QAbstractItemModel *edges() noexcept override;
    [[nodiscard]] QAbstractItemModel *selectedPredecessors() noexcept override;
    [[nodiscard]] QAbstractItemModel *selectedSuccessors() noexcept override;
    [[nodiscard]] qreal contentWidth() const noexcept override;
    [[nodiscard]] qreal contentHeight() const noexcept override;
    [[nodiscard]] QString searchText() const override;
    void setSearchText(const QString &searchText) override;
    [[nodiscard]] int statusFilterIndex() const noexcept override;
    void setStatusFilterIndex(int index) override;
    [[nodiscard]] QVariantList categoryFilterOptions() const override;
    [[nodiscard]] int categoryFilterMode() const noexcept override;
    [[nodiscard]] QString categoryFilterCategoryId() const override;
    [[nodiscard]] int taskCount() const noexcept override;
    [[nodiscard]] int blockedCount() const noexcept override;
    [[nodiscard]] QString currentTaskId() const override;
    [[nodiscard]] QString selectedTaskId() const override;
    [[nodiscard]] QString selectedTaskTitle() const override;
    [[nodiscard]] QString selectedDescription() const override;
    [[nodiscard]] QString selectedStatusText() const override;
    [[nodiscard]] QString selectedPriorityText() const override;
    [[nodiscard]] QString selectedDeadlineText() const override;
    [[nodiscard]] QString selectedEstimatedDurationText() const override;
    [[nodiscard]] QString selectedBlockingReason() const override;
    [[nodiscard]] int selectedUnlockCount() const noexcept override;
    [[nodiscard]] int selectedPredecessorCount() const noexcept override;
    [[nodiscard]] int selectedSuccessorCount() const noexcept override;
    [[nodiscard]] qreal selectedNodeCenterX() const noexcept override;
    [[nodiscard]] qreal selectedNodeCenterY() const noexcept override;
    [[nodiscard]] bool canEditSelectedDependencies() const noexcept override;
    [[nodiscard]] QString selectedCategoryName() const override;
    [[nodiscard]] QString selectedCategoryAccent() const override;
    [[nodiscard]] bool selectedHasCategory() const noexcept override;
    [[nodiscard]] bool selectedCoreNode() const noexcept override;
    [[nodiscard]] bool empty() const noexcept override;
    [[nodiscard]] QString errorMessage() const;

    void reload() override;
    bool selectTask(const QString &taskId) override;
    void clearSelection() override;
    bool locateFirstMatch() override;
    bool selectCurrentTask() override;
    /// 重新请求Model裁剪后的类别子图；QML不得自行删除节点或边。
    bool setCategoryFilter(int mode, const QString &categoryId = {}) override;
    void setHoveredTask(const QString &taskId) override;
    void clearHoveredTask() override;

signals:
    void errorMessageChanged();

private:
    using NodeProjection = TaskGraphNodeProjection;

    [[nodiscard]] int rowForTask(const model::TaskId &taskId) const;
    [[nodiscard]] const NodeProjection *selectedNode() const;
    [[nodiscard]] int emphasisFor(const model::TaskId &taskId) const;
    [[nodiscard]] bool filterMatches(const NodeProjection &projection) const;
    [[nodiscard]] static QString statusText(model::TaskStatus status);
    [[nodiscard]] static QString priorityText(model::TaskPriority priority);
    [[nodiscard]] static QString deadlineText(const model::Task &task);
    [[nodiscard]] static QString durationText(const model::Task &task);
    void replaceGraph(const model::TaskGraphSnapshot &snapshot);
    void notifyInteractionRoles();
    void rebuildRelationModels();
    void setErrorMessage(const QString &message);
    void reloadCategories();
    [[nodiscard]] const model::TaskCategory *categoryForTask(
        const model::Task &task) const;

    TaskGraphViewModel(model::TaskService &taskService,
                       model::TaskCategoryService *categoryService,
                       QObject *parent);

    model::TaskService &m_taskService;
    model::TaskCategoryService *m_categoryService{nullptr};
    TaskGraphEdgeListModel *m_edges;
    TaskGraphRelationListModel *m_selectedPredecessors;
    TaskGraphRelationListModel *m_selectedSuccessors;
    QList<NodeProjection> m_nodes;
    QList<model::TaskGraphEdge> m_snapshotEdges;
    QHash<model::TaskId, int> m_predecessorCounts;
    QHash<model::TaskId, int> m_successorCounts;
    model::TaskId m_selectedTaskId;
    model::TaskId m_hoveredTaskId;
    QString m_searchText;
    int m_statusFilterIndex{0};
    QList<model::TaskCategory> m_categories;
    int m_categoryFilterMode{0};
    model::TaskCategoryId m_categoryFilterCategoryId;
    qreal m_contentWidth{0.0};
    qreal m_contentHeight{0.0};
    QString m_errorMessage;
};

} // namespace smartmate::viewmodel
