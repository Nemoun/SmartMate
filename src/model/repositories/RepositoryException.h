#pragma once

#include <QString>

#include <stdexcept>

namespace smartmate::model {

/// 持久化端口无法完成操作时抛出的统一异常，不向 Service 泄漏 QSqlError/QSettings 状态。
/// Service 捕获后映射为结构化错误；异常文本只用于诊断，不直接绑定为界面文案。
class RepositoryException final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;

    explicit RepositoryException(const QString &message)
        : std::runtime_error(message.toUtf8().constData())
    {
    }
};

} // namespace smartmate::model
