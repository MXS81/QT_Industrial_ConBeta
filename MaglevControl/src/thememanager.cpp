
#include "thememanager.h"
// Qt / 系统头文件
#include <QApplication>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>

/**
 * 构造函数
 */
ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
}


/**
 * 捕获应用启动时的原生样式与调色板
 * 注意：应在任何 setStyleSheet 或自定义样式应用前尽早调用
 */
void ThemeManager::captureOriginal()
{
    if (qApp->style()) {
        m_originalStyleName = qApp->style()->objectName();
    }
    m_originalPalette = qApp->palette();
}


/**
 * 应用 StyleManager 提供的主题
 */
bool ThemeManager::applyStyleManagerTheme(StyleManager::ThemeType theme, QString* /*error*/)
{
    // 通过 StyleManager 获取样式字符串并应用到应用级别
    const QString css = [theme]() {
        switch (theme) {
        case StyleManager::DarkIndustrial: return StyleManager::loadThemeFromFile(":/styles/dark_industrial_theme.css");
        case StyleManager::LightModern:    return StyleManager::loadThemeFromFile(":/styles/light_modern_theme.css");
        case StyleManager::ClassicBlue:    return StyleManager::loadThemeFromFile(":/styles/classic_blue_theme.css");
        case StyleManager::BlueGradient:   return StyleManager::loadThemeFromFile(":/styles/lightblue_style.css");
        }
        return StyleManager::loadThemeFromFile(":/styles/dark_industrial_theme.css");
    }();

    qApp->setStyleSheet(css);
    m_styleManagerTheme = theme;
    m_mode = ThemeMode::StyleManagerTheme;
    saveToSettings();
    emit themeChanged(m_mode);
    return true;
}


/**
 * 清空样式并回到原生
 */
void ThemeManager::clearToOriginal()
{
    qApp->setStyleSheet("");

    if (!m_originalStyleName.isEmpty()) {
        if (QStyle* style = QStyleFactory::create(m_originalStyleName)) {
            qApp->setStyle(style);
        }
    }
    qApp->setPalette(m_originalPalette);

    m_mode = ThemeMode::Original;
    saveToSettings();
    emit themeChanged(m_mode);
}


/**
 * 从 QSettings 恢复主题
 * 失败则保持原生（返回 false）
 */
bool ThemeManager::restoreFromSettings()
{
    QSettings settings;
    const QString mode = settings.value(kKeyMode).toString();
    if (mode == QLatin1String("StyleManagerTheme")) {
        int t = settings.value(kKeySMTheme, static_cast<int>(StyleManager::DarkIndustrial)).toInt();
        StyleManager::ThemeType theme = static_cast<StyleManager::ThemeType>(t);
        return applyStyleManagerTheme(theme);
    }

    // 默认/Original：不做任何操作，保留原生
    return false;
}


/**
 * 将当前主题状态写入 QSettings
 */
void ThemeManager::saveToSettings()
{
    QSettings settings;
    switch (m_mode) {
    case ThemeMode::Original:
        settings.setValue(kKeyMode, "Original");
        settings.remove(kKeySMTheme);
        break;
    case ThemeMode::StyleManagerTheme:
        settings.setValue(kKeyMode, "StyleManagerTheme");
        settings.setValue(kKeySMTheme, static_cast<int>(m_styleManagerTheme));
        break;
    case ThemeMode::CustomQss:
        settings.setValue(kKeyMode, "CustomQss");
        // 预留：QSS 路径等
        break;
    }
}

