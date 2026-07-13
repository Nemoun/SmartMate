#pragma once

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QListView;
class QPushButton;

namespace smartmate::viewmodel { class TaskCategoryContract; }

namespace smartmate::view::widgets {

/// 类别目录与单类别草稿的纯 Widgets View；所有命令都使用 Contract 提供的稳定 ID。
class TaskCategoryDialog final : public QDialog {
    Q_OBJECT
public:
    explicit TaskCategoryDialog(viewmodel::TaskCategoryContract &categories,
                                QWidget *parent = nullptr);

    /// 刷新类别目录并显示管理窗口，不隐式清空当前草稿。
    void openManager();

private:
    void synchronizeDraft();
    void synchronizeActions();
    void selectEditingCategory();
    [[nodiscard]] QString selectedCategoryId() const;

    viewmodel::TaskCategoryContract &m_categories;
    QListView *m_list;
    QLabel *m_empty;
    QLineEdit *m_name;
    QComboBox *m_color;
    QLabel *m_notification;
    QPushButton *m_edit;
    QPushButton *m_delete;
    QPushButton *m_reset;
    QPushButton *m_save;
};

} // namespace smartmate::view::widgets
