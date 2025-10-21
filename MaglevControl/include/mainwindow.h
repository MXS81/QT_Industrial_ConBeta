#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QTextEdit>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QActionGroup>
#include <QSplitter>
#include <QToolButton>
#include <QTimer>

// 引入模块
#include "modbusmanager.h"
// #include "logmanager.h"  // LogManager暂时不存在，先注释掉
#include "stylemanager.h"
#include "thememanager.h"
#include "recipemanager.h"
#include "recipewidget.h"
#include "controlpanel.h"
#include "globalparametersetting.h"
#include "singlemovercontrol.h"
#include "logwindow.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 连接相关槽函数
    void onConnectButtonClicked();
    void onClearLogClicked();

private slots:
    // 模块状态处理
    void onModbusError(const QString& error);
    void onStatusChanged(const QString& status);

private:
    // 核心模块
    ModbusManager* m_modbusManager;
    // LogManager* m_logManager;  // 暂时移除LogManager
    RecipeManager* m_recipeManager;
    RecipeWidget* m_recipeWidget;
    ControlPanel* m_controlPanel;
    GlobalParameterSetting* m_globalParameterSetting;
    SingleMoverControl* m_singleMoverControl;

    // 主题管理
    ThemeManager* m_themeManager { nullptr };
    QMenu* m_themeMenu { nullptr };
    QActionGroup* m_themeActionGroup { nullptr };
    QAction* m_actThemeDark { nullptr };
    QAction* m_actThemeLight { nullptr };
    QAction* m_actThemeBlue { nullptr };
    QAction* m_actThemeBlueGradient { nullptr };

    // 连接设置区域控件
    QLineEdit *ipLineEdit;
    QSpinBox *portSpinBox;
    QPushButton *connectButton;
    QLabel *statusLabel;


    // 日志窗口
    LogWindow* m_logWindow { nullptr };
    QToolButton* m_openLogBtn { nullptr };
    
    // 日志按钮闪烁相关
    QTimer* m_blinkTimer { nullptr };
    bool m_isBlinking { false };
    bool m_blinkState { false };
    QString m_blinkColor;


    QModbusDevice::State previousState = QModbusDevice::UnconnectedState; // 记录上一状态
    void onModbusStateChanged(QModbusDevice::State newState);

    // 初始化方法
    void initModules();
    void initUI();
    void connectSignals();
    void initThemeMenu();
    // 创建连接设置区域
    QWidget* createConnectionArea();
    // 日志相关方法
    void appendLog(const QString& text);
    void openLogWindow();
    void onLogWindowClosed();
    
    // 闪烁功能方法
    void startBlinking(const QString& color);
    void stopBlinking();
    void toggleBlinkState();
    
    // 布局工具方法
    void setupStandardLayout(QLayout* layout, int spacing = StyleConstants::STANDARD_SPACING);
    QGroupBox* createStandardGroupBox(const QString& title, QLayout** outLayout = nullptr);
    QHBoxLayout* createStandardHBoxLayout();
    QVBoxLayout* createStandardVBoxLayout();
    
};

#endif // MAINWINDOW_H
