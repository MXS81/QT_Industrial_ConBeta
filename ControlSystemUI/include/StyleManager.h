#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>

class StyleManager
{
public:
    // 获取全局样式表
    static QString getGlobalStyleSheet();

    // 获取特定组件的样式
    static QString getMainWindowStyle();
    static QString getDialogStyle();
    static QString getGroupBoxStyle();
    static QString getSpinBoxStyle();
    static QString getComboBoxStyle();
    static QString getLineEditStyle();
    static QString getPushButtonStyle();
    static QString getRadioButtonStyle();
    static QString getLabelStyle();
    static QString getTabWidgetStyle();
    static QString getToolBarStyle();
    static QString getStatusBarStyle();
    static QString getTableWidgetStyle();
    static QString getTextEditStyle();
    static QString getScrollAreaStyle();

private:
    StyleManager() = default;
};

#endif // STYLEMANAGER_H
