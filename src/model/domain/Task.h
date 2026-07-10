#pragma once

#include "domain/TaskTypes.h"

#include <QString>

namespace smartmate::model {

class Task final {
public:
    Task();
    Task(TaskId id, QString title, TaskStatus status = TaskStatus::Todo);

    [[nodiscard]] const TaskId &id() const noexcept;
    [[nodiscard]] const QString &title() const noexcept;
    [[nodiscard]] TaskStatus status() const noexcept;

private:
    TaskId m_id;
    QString m_title;
    TaskStatus m_status{TaskStatus::Todo};
};

} // namespace smartmate::model
