#ifndef STYLEUTILS_H
#define STYLEUTILS_H

#include <QPushButton>
#include <QWidget>
#include <QString>

class StyleUtils
{
public:
    // 按钮样式类型枚举
    enum ButtonType {
        Default,
        Success,
        Danger,
        Warning,
        Info,
        Emergency
    };

    // 为按钮设置样式类型
    static void setButtonType(QPushButton *button, ButtonType type);

    // 为控件设置状态样式
    static void setWidgetState(QWidget *widget, const QString &state);

    // 应用主题到特定控件
    static void applyTheme(QWidget *widget, const QString &theme = "dark");

    // 强制刷新控件样式
    static void refreshStyle(QWidget *widget);

private:
    StyleUtils() = default;
};

#endif // STYLEUTILS_H
