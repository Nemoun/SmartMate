#include "services/TaskCategoryService.h"

#include "domain/TaskCategoryConstraints.h"
#include "domain/UnicodeText.h"
#include "repositories/RepositoryException.h"

#include <QDateTime>

#include <algorithm>
#include <exception>
#include <utility>

namespace smartmate::model {
namespace {

[[nodiscard]] QString stableCategoryId(const TaskCategoryId &id)
{
    return id.toString(QUuid::WithoutBraces);
}

[[nodiscard]] std::optional<TaskCategoryError> validateDraft(
    const TaskCategoryDraft &draft)
{
    const QString trimmedName = draft.name.trimmed();
    if (trimmedName.isEmpty()) {
        return TaskCategoryError::EmptyName;
    }
    // 领域“字符”边界按 Unicode code point 计算，并与 SQLite length() 保持一致。
    if (unicodeCodePointCount(trimmedName)
        > TaskCategoryConstraints::maximumNameLength) {
        return TaskCategoryError::NameTooLong;
    }
    if (!isValidTaskCategoryColor(draft.color)) {
        return TaskCategoryError::InvalidColor;
    }
    return std::nullopt;
}

[[nodiscard]] QString validationDetail(const TaskCategoryError error)
{
    switch (error) {
    case TaskCategoryError::EmptyName:
        return QStringLiteral("Task category name must not be empty.");
    case TaskCategoryError::NameTooLong:
        return QStringLiteral("Task category name exceeds %1 characters.")
            .arg(TaskCategoryConstraints::maximumNameLength);
    case TaskCategoryError::InvalidColor:
        return QStringLiteral("Task category color is invalid.");
    default:
        return QStringLiteral("Task category validation failed.");
    }
}

[[nodiscard]] bool hasDuplicateName(const QList<TaskCategory> &categories,
                                    const QString &nameKey,
                                    const std::optional<TaskCategoryId> &excludedId)
{
    // 更新时排除自身；名称比较统一使用领域规范化键，保证大小写规则一致。
    return std::any_of(categories.cbegin(), categories.cend(),
                       [&nameKey, &excludedId](const TaskCategory &category) {
        return (!excludedId.has_value() || category.id != *excludedId)
            && taskCategoryNameKey(category.name) == nameKey;
    });
}

void sortCategories(QList<TaskCategory> &categories)
{
    // 稳定 ID 作为同名键的次级排序，避免 Repository 返回顺序影响界面。
    std::sort(categories.begin(), categories.end(),
              [](const TaskCategory &left, const TaskCategory &right) {
        const QString leftKey = taskCategoryNameKey(left.name);
        const QString rightKey = taskCategoryNameKey(right.name);
        if (leftKey != rightKey) {
            return leftKey < rightKey;
        }
        return stableCategoryId(left.id) < stableCategoryId(right.id);
    });
}

template<typename Result>
[[nodiscard]] Result persistenceFailure(const RepositoryException &exception)
{
    return Result::failure(TaskCategoryError::PersistenceFailure,
                           QString::fromUtf8(exception.what()));
}

template<typename Result>
[[nodiscard]] Result unexpectedPersistenceFailure()
{
    return Result::failure(TaskCategoryError::PersistenceFailure,
                           QStringLiteral("Unexpected task category repository failure."));
}

} // namespace

TaskCategoryService::TaskCategoryService(ITaskCategoryRepository &repository,
                                         QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
}

TaskCategoryListResult TaskCategoryService::listCategories() const
{
    try {
        QList<TaskCategory> categories = m_repository.findAllCategories();
        sortCategories(categories);
        return TaskCategoryListResult::success(std::move(categories));
    } catch (const RepositoryException &exception) {
        return persistenceFailure<TaskCategoryListResult>(exception);
    } catch (...) {
        return unexpectedPersistenceFailure<TaskCategoryListResult>();
    }
}

TaskCategoryResult TaskCategoryService::createCategory(
    const TaskCategoryDraft &draft)
{
    if (const auto error = validateDraft(draft)) {
        return TaskCategoryResult::failure(*error, validationDetail(*error));
    }

    try {
        const QList<TaskCategory> categories = m_repository.findAllCategories();
        const QString name = draft.name.trimmed();
        if (hasDuplicateName(categories, taskCategoryNameKey(name), std::nullopt)) {
            return TaskCategoryResult::failure(
                TaskCategoryError::DuplicateName,
                QStringLiteral("Task category name already exists."));
        }

        TaskCategoryId id;
        // UUID 理论上极少冲突，但 Service 仍以当前快照复核，确保领域 ID 唯一。
        do {
            id = QUuid::createUuid();
        } while (std::any_of(categories.cbegin(), categories.cend(),
                             [&id](const TaskCategory &category) {
            return category.id == id;
        }));
        const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
        TaskCategory category{id, name, draft.color, nowUtc, nowUtc};
        m_repository.insertCategory(category);
        // 命令先完成持久化，再广播失效通知；订阅 ViewModel 会各自重建 Contract 投影。
        emit categoriesChanged();
        return TaskCategoryResult::success(std::move(category));
    } catch (const RepositoryException &exception) {
        return persistenceFailure<TaskCategoryResult>(exception);
    } catch (...) {
        return unexpectedPersistenceFailure<TaskCategoryResult>();
    }
}

TaskCategoryResult TaskCategoryService::updateCategory(
    const TaskCategoryId &id,
    const TaskCategoryDraft &draft)
{
    if (const auto error = validateDraft(draft)) {
        return TaskCategoryResult::failure(*error, validationDetail(*error));
    }

    try {
        const std::optional<TaskCategory> current =
            m_repository.findCategoryById(id);
        if (!current.has_value()) {
            return TaskCategoryResult::failure(
                TaskCategoryError::NotFound,
                QStringLiteral("Task category was not found."));
        }

        const QList<TaskCategory> categories = m_repository.findAllCategories();
        const QString name = draft.name.trimmed();
        if (hasDuplicateName(categories, taskCategoryNameKey(name), id)) {
            return TaskCategoryResult::failure(
                TaskCategoryError::DuplicateName,
                QStringLiteral("Task category name already exists."));
        }
        if (current->name == name && current->color == draft.color) {
            // 幂等保存不写 Repository，也不触发无意义的投影刷新。
            return TaskCategoryResult::success(*current);
        }

        TaskCategory updated{id,
                             name,
                             draft.color,
                             current->createdAtUtc,
                             QDateTime::currentDateTimeUtc()};
        if (!m_repository.updateCategory(updated)) {
            return TaskCategoryResult::failure(
                TaskCategoryError::NotFound,
                QStringLiteral("Task category disappeared during update."));
        }
        // 只广播“类别目录已变化”，不携带领域实体，避免把 Service 信号当成界面数据绑定。
        emit categoriesChanged();
        return TaskCategoryResult::success(std::move(updated));
    } catch (const RepositoryException &exception) {
        return persistenceFailure<TaskCategoryResult>(exception);
    } catch (...) {
        return unexpectedPersistenceFailure<TaskCategoryResult>();
    }
}

TaskCategoryDeletionResult TaskCategoryService::deleteCategory(
    const TaskCategoryId &id)
{
    try {
        const std::optional<TaskCategory> current =
            m_repository.findCategoryById(id);
        if (!current.has_value()) {
            return TaskCategoryDeletionResult::failure(
                TaskCategoryError::NotFound,
                QStringLiteral("Task category was not found."));
        }

        const CategoryDeletionWriteResult writeResult =
            // “删除类别 + 清空任务归属”必须由 Repository 端口在同一事务中完成。
            m_repository.deleteCategoryAndUnassignTasks(
                id, QDateTime::currentDateTimeUtc());
        if (!writeResult.categoryDeleted) {
            return TaskCategoryDeletionResult::failure(
                TaskCategoryError::NotFound,
                QStringLiteral("Task category disappeared during deletion."));
        }
        // 类别目录必然变化；各 ViewModel 通过 listCategories() 拉取一致的新快照。
        emit categoriesChanged();
        if (writeResult.unassignedTaskCount > 0) {
            // 只有任务归属实际变化才通知任务投影，避免无关联类别删除引起无效刷新。
            emit taskCategoryAssignmentsChanged();
        }
        return TaskCategoryDeletionResult::success(
            {*current, writeResult.unassignedTaskCount});
    } catch (const RepositoryException &exception) {
        return persistenceFailure<TaskCategoryDeletionResult>(exception);
    } catch (...) {
        return unexpectedPersistenceFailure<TaskCategoryDeletionResult>();
    }
}

} // namespace smartmate::model
