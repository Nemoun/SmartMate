#pragma once

#include "common/presentation/UiNotification.h"

#include <QAbstractListModel>
#include <QStringList>

namespace smartmate::viewmodel {

/// 类别目录和单类别编辑会话的抽象展示契约。
///
/// QAbstractListModel Role 提供目录绑定，Q_PROPERTY 提供当前草稿绑定；Widget 只提交
/// 稳定 TaskCategoryId 命令。具体 ViewModel 负责调用 Service、刷新模型和发布通知。
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
    /// 类别列表的稳定 Role；Widget 不得依赖领域对象、SQL 列或列表行号作为身份。
    enum Role {
        CategoryIdRole = Qt::UserRole + 1,
        NameRole,
        ColorIndexRole,
        AccentRole,
        // 任务数量是聚合展示值，不持久化在类别实体中。
        TaskCountRole,
    };
    Q_ENUM(Role)

    ~TaskCategoryContract() override = default;

    // getter 形成列表统计与编辑草稿的当前快照；绑定建立时必须先同步读取。
    [[nodiscard]] virtual int count() const noexcept = 0;
    [[nodiscard]] virtual bool empty() const noexcept = 0;
    [[nodiscard]] virtual bool editMode() const noexcept = 0;
    [[nodiscard]] virtual QString editingCategoryId() const = 0;
    [[nodiscard]] virtual QString draftName() const = 0;
    /// 双向草稿绑定的用户编辑入口；程序性回填控件时应阻断控件信号以避免回写循环。
    virtual void setDraftName(const QString &name) = 0;
    [[nodiscard]] virtual int draftColorIndex() const noexcept = 0;
    virtual void setDraftColorIndex(int index) = 0;
    [[nodiscard]] virtual QStringList colorOptions() const = 0;
    [[nodiscard]] virtual QStringList colorAccents() const = 0;
    [[nodiscard]] virtual bool dirty() const noexcept = 0;
    [[nodiscard]] virtual bool canSave() const noexcept = 0;

public slots:
    /// 从 Service 重新加载类别目录；通常由 Model 失效通知触发，也可显式刷新。
    virtual void reload() = 0;
    /// 开启空白创建会话并重置草稿。
    virtual void beginCreate() = 0;
    /// 按稳定 ID 开启编辑会话；目标不存在返回 false。
    virtual bool beginEdit(const QString &categoryId) = 0;
    /// 一次提交创建或更新命令；业务失败返回 false 并通过通知报告。
    virtual bool save() = 0;
    /// 按稳定 ID 删除类别；具体实现负责确认后的原子解除任务归属。
    virtual bool deleteCategory(const QString &categoryId) = 0;
    /// 放弃当前草稿，不调用 Model 写命令。
    virtual void cancel() = 0;

signals:
    /// 列表数量变化通知；行内容变化仍使用 QAbstractItemModel 标准信号。
    void countChanged();
    /// 编辑模式、草稿值、dirty 或 canSave 任一相关状态变化后发送。
    void draftChanged();
    /// 保存成功的会话结果，携带稳定类别 ID，便于 View 关闭或定位。
    void saved(const QString &categoryId);
    /// 删除成功结果；数量用于 View 展示有多少任务被转为未分类。
    void deleted(const QString &categoryId, int unassignedTaskCount);
    /// 用户取消编辑会话的流程通知。
    void cancelled();
    /// 业务错误或确认结果的展示通知，与数据变更信号职责分离。
    void notificationRaised(const smartmate::common::UiNotification &notification);

protected:
    /// 仅供具体 ViewModel 构造，Widget 无权创建实现或管理其业务生命周期。
    explicit TaskCategoryContract(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
};

} // namespace smartmate::viewmodel
