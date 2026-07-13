#pragma once

#include "domain/Task.h"
#include "services/TaskResult.h"
#include "viewmodel/contracts/TaskDependencyContract.h"

#include <QHash>
#include <QSet>
#include <QtQmlIntegration/qqmlintegration.h>

namespace smartmate::model {
class TaskService;
class TaskCategoryService;
}

namespace smartmate::viewmodel {

/// 为一个待办任务维护独立的前置依赖草稿。
///
/// 候选行始终使用稳定 TaskId；勾选只修改本地草稿，save() 成功后才会通过
/// TaskService 整体替换关系，cancel() 不改变 Model。
class TaskDependencyViewModel final : public TaskDependencyContract {
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    QML_NAMED_ELEMENT(TaskDependencyViewModel)
    QML_UNCREATABLE("TaskDependencyViewModel is owned by AppViewModel")

public:
    explicit TaskDependencyViewModel(model::TaskService &taskService,
                                     QObject *parent = nullptr);
    TaskDependencyViewModel(model::TaskService &taskService,
                            model::TaskCategoryService &categoryService,
                            QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString taskId() const override;
    [[nodiscard]] QString taskTitle() const override;
    [[nodiscard]] int count() const noexcept override;
    [[nodiscard]] int selectedCount() const noexcept override;
    [[nodiscard]] bool dirty() const noexcept override;
    [[nodiscard]] bool canSave() const noexcept override;
    [[nodiscard]] QString errorMessage() const;

    /// 读取任务和依赖快照并开始新的本地草稿；任务不可编辑或读取失败时返回 false。
    bool beginEdit(const QString &taskId) override;
    /// 按稳定 TaskId 改变候选项选择，不接受列表行号。
    bool setPredecessorSelected(const QString &predecessorTaskId,
                                bool selected) override;
    /// 将完整前置 TaskId 集合交给 Service；环检测等规则仍由 Model 判定。
    bool save() override;
    /// 恢复打开时的选择并放弃草稿，不访问 Repository。
    void cancel() override;
    Q_INVOKABLE void clearError();

signals:
    void errorMessageChanged();

private:
    [[nodiscard]] static QString statusText(model::TaskStatus status);
    [[nodiscard]] static QString priorityText(model::TaskPriority priority);
    [[nodiscard]] int candidateRow(const model::TaskId &taskId) const;
    [[nodiscard]] QString taskDisplayName(const model::TaskId &taskId) const;
    [[nodiscard]] QString dependencyErrorMessage(
        model::TaskError error,
        const model::TaskErrorContext &context) const;
    void replaceDraft(model::TaskDependencyEditContext context);
    void notifySelectionChanged();
    void setErrorMessage(const QString &message);
    void reloadCategories();
    [[nodiscard]] const model::TaskCategory *categoryForTask(
        const model::Task &task) const;

    TaskDependencyViewModel(model::TaskService &taskService,
                            model::TaskCategoryService *categoryService,
                            QObject *parent);

    /// 非拥有引用；组合根保证 Service 生命周期长于本 ViewModel。
    model::TaskService &m_taskService;
    model::TaskCategoryService *m_categoryService{nullptr};
    QList<model::TaskCategory> m_categories;
    model::TaskId m_taskId;
    QString m_taskTitle;
    /// 候选列表排除当前任务；未归档任务可新增，原有归档前置仍保留以便移除。
    QList<model::Task> m_candidates;
    /// 保存本次读取的全量标题，确保环路径经过隐藏或归档任务时仍可完整解释。
    QHash<model::TaskId, QString> m_taskTitles;
    QSet<model::TaskId> m_selectedPredecessors;
    QSet<model::TaskId> m_originalPredecessors;
    /// 候选资格完全来自 Model 上下文，ViewModel 不根据任务状态重新推导。
    QSet<model::TaskId> m_selectablePredecessors;
    QString m_errorMessage;
};

} // namespace smartmate::viewmodel
