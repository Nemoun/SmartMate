#pragma once

#include "domain/TaskCategory.h"
#include "viewmodel/contracts/TaskCategoryContract.h"


namespace smartmate::model {
class TaskService;
class TaskCategoryService;
}

namespace smartmate::viewmodel {

/// 投影类别目录并维护单个类别的创建/编辑草稿。
///
/// 名称唯一性、颜色有效性和原子删除均由 TaskCategoryService 判定；本类型
/// 只把结构化结果映射为 QML 可观察状态。
class TaskCategoryViewModel final : public TaskCategoryContract {
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
public:
    /// categoryService 可为空以兼容不加载类别功能的隔离测试；生产组合根必须注入。
    explicit TaskCategoryViewModel(model::TaskService &taskService,
                                   model::TaskCategoryService *categoryService = nullptr,
                                   QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int count() const noexcept override;
    [[nodiscard]] bool empty() const noexcept override;
    [[nodiscard]] bool editMode() const noexcept override;
    [[nodiscard]] QString editingCategoryId() const override;
    [[nodiscard]] QString draftName() const override;
    void setDraftName(const QString &name) override;
    [[nodiscard]] int draftColorIndex() const noexcept override;
    void setDraftColorIndex(int index) override;
    [[nodiscard]] QStringList colorOptions() const override;
    [[nodiscard]] QStringList colorAccents() const override;
    [[nodiscard]] bool dirty() const noexcept override;
    [[nodiscard]] bool canSave() const noexcept override;
    [[nodiscard]] QString errorMessage() const;

    void reload() override;
    void beginCreate() override;
    bool beginEdit(const QString &categoryId) override;
    bool save() override;
    bool deleteCategory(const QString &categoryId) override;
    void cancel() override;
    Q_INVOKABLE void clearError();

signals:
    void errorMessageChanged();

private:
    [[nodiscard]] int rowForCategory(const model::TaskCategoryId &id) const;
    [[nodiscard]] QString categoryErrorText(int error) const;
    void setErrorMessage(const QString &message);

    model::TaskService &m_taskService;
    /// 非拥有指针；nullptr 仅用于旧的隔离测试构造路径。
    model::TaskCategoryService *m_categoryService{nullptr};
    QList<model::TaskCategory> m_categories;
    QHash<model::TaskCategoryId, int> m_taskCounts;
    model::TaskCategoryId m_editingCategoryId;
    QString m_draftName;
    model::TaskCategoryColor m_draftColor{model::TaskCategoryColor::Blue};
    QString m_originalName;
    model::TaskCategoryColor m_originalColor{model::TaskCategoryColor::Blue};
    bool m_editMode{false};
    QString m_errorMessage;
};

} // namespace smartmate::viewmodel
