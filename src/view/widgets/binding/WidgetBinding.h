#pragma once

#include <QAbstractButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QMetaObject>
#include <QSignalBlocker>

#include <functional>
#include <utility>

namespace smartmate::view::widgets::binding {

/// 先读取当前 getter 完成初始同步，再以 context 管理通知连接的生命周期。
template<typename Source, typename Signal, typename Getter, typename Apply>
QMetaObject::Connection bindOneWay(Source &source,
                                   Signal signal,
                                   QObject &context,
                                   Getter getter,
                                   Apply apply)
{
    auto synchronize = [getter = std::move(getter),
                        apply = std::move(apply)]() mutable {
        std::invoke(apply, std::invoke(getter));
    };
    synchronize();
    return QObject::connect(&source, signal, &context, std::move(synchronize));
}

/// 组合框只把用户 activated 事件写回 Contract；通知回填使用 QSignalBlocker。
template<typename Source, typename Signal, typename Getter, typename Setter>
void bindComboBoxIndex(Source &source,
                       Signal signal,
                       QComboBox &comboBox,
                       Getter getter,
                       Setter setter)
{
    bindOneWay(source, signal, comboBox, getter,
               [&comboBox](const int index) {
                   const QSignalBlocker blocker{&comboBox};
                   comboBox.setCurrentIndex(index);
               });
    QObject::connect(&comboBox, &QComboBox::activated, &comboBox,
                     [setter = std::move(setter)](const int index) mutable {
                         std::invoke(setter, index);
                     });
}

/// 按钮组选中状态由 Contract 索引驱动；程序性更新不得被误认为用户命令。
inline void setCheckedButton(QButtonGroup &group, const int checkedId)
{
    for (QAbstractButton *button : group.buttons()) {
        const QSignalBlocker blocker{button};
        button->setChecked(group.id(button) == checkedId);
    }
}

} // namespace smartmate::view::widgets::binding
