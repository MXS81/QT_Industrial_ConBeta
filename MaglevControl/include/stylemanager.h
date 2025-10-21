#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QObject>
#include <QWidget>
#include <QString>

// 样式常量类
class StyleConstants
{
public:
    // 标准尺寸
    static const QString STANDARD_WIDGET_SIZE;
    static const QString BUTTON_SIZE;
    static const QString FONT_SIZE_15;
    
    // 按钮样式
    static const QString ENABLE_BUTTON_STYLE;
    static const QString AUTO_RUN_BUTTON_STYLE;
    static const QString STANDARD_BUTTON_STYLE;
    
    // 布局常量
    static const int STANDARD_SPACING = 15;
    static const int STANDARD_MARGIN = 15;
};

// 错误消息常量类
class ErrorMessages
{
public:
    static const QString DEVICE_NOT_CONNECTED;
    static const QString INVALID_STATION_INDEX;
    static const QString AXIS_NOT_FOUND;
    static const QString FILE_SAVE_ERROR;
    static const QString FILE_OPEN_ERROR;
    static const QString JSON_PARSE_ERROR;
    static const QString RECIPE_NAME_EMPTY;
    static const QString RECIPE_NOT_FOUND;
    static const QString RECIPE_NAME_EXISTS;
};

class StyleManager : public QObject
{
    Q_OBJECT

public:
    enum ThemeType {
        DarkIndustrial,
        LightModern,
        ClassicBlue,
        BlueGradient
    };

    explicit StyleManager(QObject *parent = nullptr);
    
    // 应用样式主题
    static void applyTheme(QWidget* widget, ThemeType theme = DarkIndustrial);
    
    // 从资源文件加载主题样式
    static QString loadThemeFromFile(const QString& themeFileName);

private:
    static QString getThemeStyleSheet(ThemeType theme);
};

#endif // STYLEMANAGER_H