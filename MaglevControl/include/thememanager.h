// ThemeManager: 运行时主题管理器
// 负责：记录原生样式与调色板、应用/清除 StyleManager 主题、持久化/恢复主题状态

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

// 本地头文件优先
#include "stylemanager.h"

// 再引入 Qt/系统头文件
#include <QObject>
#include <QString>
#include <QPalette>

class QWidget;

/**
 * ThemeManager
 * 说明：
 * - 捕获应用启动时的原生样式名与调色板（用于回退）。
 * - 应用 StyleManager 提供的主题（DarkIndustrial/LightModern/ClassicBlue）。
 * - 清空样式回到原生；支持通过 QSettings 持久化与恢复主题。
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum class ThemeMode {
        Original,            // 启动时的原生样式 + 调色板
        StyleManagerTheme,   // 使用 StyleManager 的主题
        CustomQss            // 预留：自定义 QSS（本迭代不实现）
    };

    explicit ThemeManager(QObject* parent = nullptr);

    /** 捕获应用启动时的原生样式名与调色板（应在任何 setStyleSheet 之前尽早调用）。 */
    void captureOriginal();

    /** 应用 StyleManager 主题并记录状态。 */
    bool applyStyleManagerTheme(StyleManager::ThemeType theme, QString* error = nullptr);

    /** 清除样式并回到应用启动时的原生样式与调色板。 */
    void clearToOriginal();

    /** 从 QSettings 恢复主题；失败返回 false（保持原生）。 */
    bool restoreFromSettings();

    /** 将当前主题状态写入 QSettings。 */
    void saveToSettings();

    ThemeMode currentMode() const { return m_mode; }
    StyleManager::ThemeType currentTheme() const { return m_styleManagerTheme; }

Q_SIGNALS:
    void themeChanged(ThemeMode mode);

private:
    // QSettings 键名
    static constexpr const char* kKeyMode = "ui/theme/mode";
    static constexpr const char* kKeySMTheme = "ui/theme/styleManagerTheme";

    // 原生样式与调色板
    QString m_originalStyleName;
    QPalette m_originalPalette;

    // 当前状态
    ThemeMode m_mode { ThemeMode::Original };
    StyleManager::ThemeType m_styleManagerTheme { StyleManager::DarkIndustrial };
};

#endif // THEMEMANAGER_H

