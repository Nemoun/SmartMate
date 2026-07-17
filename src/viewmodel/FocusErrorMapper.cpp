#include "FocusErrorMapper.h"

namespace smartmate::viewmodel {

QString focusErrorMessage(const model::FocusError error)
{
    using enum model::FocusError;
    switch (error) {
    case None: return {};
    case NotInitialized:
        return QStringLiteral("专注服务尚未就绪，请重新启动应用。");
    case TaskNotFound:
        return QStringLiteral("任务不存在或已无法访问。");
    case TaskNotInProgress:
        return QStringLiteral("只有进行中的任务才能开始专注。");
    case ActiveSessionExists:
        return QStringLiteral("已有活动专注，请先处理当前专注。");
    case SessionNotFound:
        return QStringLiteral("该专注会话已不存在，页面已重新加载。");
    case StateConflict:
        return QStringLiteral("专注状态已经变化，页面已重新加载。");
    case MinimumDurationNotReached:
        return QStringLiteral("累计专注不足 1 秒，暂时不能完成。");
    case PersistenceFailure:
        return QStringLiteral("专注数据访问失败，请稍后重试。");
    }
    return QStringLiteral("未知专注错误。");
}

} // namespace smartmate::viewmodel
