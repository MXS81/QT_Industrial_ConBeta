#include "stylemanager.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

// 样式常量定义
const QString StyleConstants::STANDARD_WIDGET_SIZE = "min-width: 200px; max-width: 200px;";
const QString StyleConstants::BUTTON_SIZE = "min-width: 200px; max-width: 200px; height: 35px;";
const QString StyleConstants::FONT_SIZE_15 = "font-size: 15px;";

const QString StyleConstants::ENABLE_BUTTON_STYLE = 
    "background-color: #f44336; font-size: 15px; min-width: 200px; max-width: 200px;";
const QString StyleConstants::AUTO_RUN_BUTTON_STYLE = 
    "background-color: green; font-size: 15px; width: 200px; min-width: 200px; max-width: 200px;";
const QString StyleConstants::STANDARD_BUTTON_STYLE = 
    "font-size: 15px; min-width: 200px; max-width: 200px;";

// 错误消息常量定义
const QString ErrorMessages::DEVICE_NOT_CONNECTED = "设备未连接";
const QString ErrorMessages::INVALID_STATION_INDEX = "工位索引超出范围";
const QString ErrorMessages::AXIS_NOT_FOUND = "轴不存在";
const QString ErrorMessages::FILE_SAVE_ERROR = "无法保存文件";
const QString ErrorMessages::FILE_OPEN_ERROR = "无法打开文件";
const QString ErrorMessages::JSON_PARSE_ERROR = "JSON解析错误";
const QString ErrorMessages::RECIPE_NAME_EMPTY = "配方名称不能为空";
const QString ErrorMessages::RECIPE_NOT_FOUND = "配方不存在";
const QString ErrorMessages::RECIPE_NAME_EXISTS = "配方名称已存在";

StyleManager::StyleManager(QObject *parent)
    : QObject(parent)
{
}

void StyleManager::applyTheme(QWidget* widget, ThemeType theme)
{
    if (!widget) return;
    
    QString styleSheet = getThemeStyleSheet(theme);
    widget->setStyleSheet(styleSheet);
}

QString StyleManager::getThemeStyleSheet(ThemeType theme)
{
    switch (theme) {
    case DarkIndustrial:
        return loadThemeFromFile(":/styles/dark_industrial_theme.css");
    case LightModern:
        return loadThemeFromFile(":/styles/light_modern_theme.css");
    case ClassicBlue:
        return loadThemeFromFile(":/styles/classic_blue_theme.css");
    case BlueGradient:
        return loadThemeFromFile(":/styles/lightblue_style.css");
    default:
        return loadThemeFromFile(":/styles/dark_industrial_theme.css");
    }
}

QString StyleManager::loadThemeFromFile(const QString& themeFileName)
{
    QFile file(themeFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开主题文件:" << themeFileName;
        return QString(); // 返回空字符串作为后备
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    return content;
}