// StyleUtils.cpp
#include "StyleUtils.h"
#include <QStyle>

void StyleUtils::setButtonType(QPushButton *button, ButtonType type)
{
    if (!button) return;

    QString className;
    switch (type) {
    case Success:
        className = "success";
        break;
    case Danger:
        className = "danger";
        break;
    case Warning:
        className = "warning";
        break;
    case Info:
        className = "info";
        break;
    case Emergency:
        className = "emergency";
        break;
    default:
        className = "default";
        break;
    }

    button->setProperty("class", className);
    refreshStyle(button);
}

void StyleUtils::setWidgetState(QWidget *widget, const QString &state)
{
    if (!widget) return;

    widget->setProperty("state", state);
    refreshStyle(widget);
}

void StyleUtils::applyTheme(QWidget *widget, const QString &theme)
{
    if (!widget) return;

    widget->setProperty("theme", theme);
    refreshStyle(widget);
}

void StyleUtils::refreshStyle(QWidget *widget)
{
    if (!widget) return;

    // 强制重新应用样式表
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}
