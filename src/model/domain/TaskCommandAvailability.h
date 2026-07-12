#pragma once

namespace smartmate::model {

/// 当前完整任务与依赖快照下的命令资格；ViewModel 只能投影，不能自行推导。
struct TaskCommandAvailability final {
    bool canEditTask{false};
    bool canEditDependencies{false};
    bool canStart{false};
    bool canCancel{false};
    bool canComplete{false};
    bool canRedo{false};
    bool canArchive{false};
    bool canRestore{false};

    friend bool operator==(const TaskCommandAvailability &,
                           const TaskCommandAvailability &) = default;
};

} // namespace smartmate::model
