#pragma once

#include "common/presentation/UiNotification.h"

#include <QAbstractListModel>

namespace smartmate::viewmodel {

/// 单任务前置依赖编辑会话的抽象展示契约。
///
/// 列表 Role 投影候选任务，selectedCount/dirty/canSave 投影会话状态；save() 一次提交
/// 完整前置集合，禁止 Widget 循环调用单边持久化命令。
class TaskDependencyContract : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString taskId READ taskId NOTIFY contextChanged)
    Q_PROPERTY(QString taskTitle READ taskTitle NOTIFY contextChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY formStateChanged)
    Q_PROPERTY(bool canSave READ canSave NOTIFY formStateChanged)

public:
    /// 前置候选列表的稳定 Role；Selected/Selectable 分离展示选中状态与命令资格。
    enum Role {
        TaskIdRole = Qt::UserRole + 1,
        ShortIdRole,
        TitleRole,
        StatusTextRole,
        PriorityTextRole,
        // Selected 表示会话草稿，Selectable 表示当前是否允许用户切换该候选。
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
    /// 为稳定 TaskId 加载完整依赖编辑上下文；任务不可编辑或不存在时返回 false。
    virtual bool beginEdit(const QString &taskId) = 0;
    /// 只修改 ViewModel 会话选择，不立即写 Repository；不可选候选返回 false。
    virtual bool setPredecessorSelected(const QString &predecessorTaskId,
                                        bool selected) = 0;
    /// 原子保存当前完整前置集合；成功后发 saved()，失败保持会话供用户修正。
    virtual bool save() = 0;
    /// 放弃本地选择草稿，不执行 Model 命令。
    virtual void cancel() = 0;

signals:
    /// 当前目标任务 ID 或标题变化后发送。
    void contextChanged();
    /// 候选行数变化；行数据变化使用 QAbstractItemModel 标准通知。
    void countChanged();
    /// 候选选中 Role 或 selectedCount 变化后发送。
    void selectionChanged();
    /// dirty/canSave 资格变化后发送。
    void formStateChanged();
    /// 原子保存成功的流程通知，携带稳定目标 TaskId。
    void saved(const QString &taskId);
    /// 会话取消通知。
    void cancelled();
    /// 请求 View 展示错误或结果，不承担数据同步职责。
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    /// 仅允许具体依赖 ViewModel 继承构造。
    explicit TaskDependencyContract(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
};

} // namespace smartmate::viewmodel
