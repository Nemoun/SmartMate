#pragma once

#include <QString>

#include <stdexcept>

namespace smartmate::model {

/// 持久化端口无法完成操作时抛出的统一异常，不向 Model 泄漏具体存储类型。
class RepositoryException final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;

    explicit RepositoryException(const QString &message)
        : std::runtime_error(message.toUtf8().constData())
    {
    }
};

} // namespace smartmate::model
