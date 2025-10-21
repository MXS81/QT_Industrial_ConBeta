#include "mainwindow.h"
#include "stylemanager.h"
#include <QMessageBox>
#include <QModbusDataUnit>
#include <QModbusReply>
#include <QDebug>
#include <QHeaderView>
#include <QTimer>
#include <QToolButton>
#include <QIcon>
#include <QStyle>
#include <QSettings>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_modbusManager(nullptr)
    // 日志由面板管理
{
    initModules();
    initUI();
    connectSignals();
    
    // 主题管理与菜单
    if (!m_themeManager) {
        m_themeManager = new ThemeManager(this);
    }
    m_themeManager->captureOriginal();
    initThemeMenu();
    if (!m_themeManager->restoreFromSettings()) {
        m_themeManager->applyStyleManagerTheme(StyleManager::DarkIndustrial);
    }
    
    // 应用样式
    // 清理旧版局部样式，确保全局主题生效
    this->setStyleSheet("");
    if (m_themeManager) {
    m_themeManager->applyStyleManagerTheme(m_themeManager->currentTheme());
}

    setGeometry(100, 100, 1500, 700);
}

MainWindow::~MainWindow()
{
    if (m_modbusManager) {
        m_modbusManager->disconnectDevice();
    }
}

void MainWindow::initModules()
{
    // 创建核心模块
    m_modbusManager = new ModbusManager(this);
    // 日志由面板管理
    m_recipeManager = new RecipeManager(m_modbusManager, this);
    m_recipeWidget = new RecipeWidget(m_recipeManager, this);
    
    // 创建新移植的模块
    m_controlPanel = new ControlPanel(this);
    m_globalParameterSetting = new GlobalParameterSetting(this);
    m_singleMoverControl = new SingleMoverControl(this);
    
    // 设置ControlPanel的ModbusManager引用
    m_controlPanel->setModbusManager(m_modbusManager);
}

void MainWindow::connectSignals()
{
    // 连接Modbus状态变化
    connect(m_modbusManager, &ModbusManager::stateChanged,
            this, &MainWindow::onModbusStateChanged);
    
    // 连接Modbus管理器错误信号
    connect(m_modbusManager, &ModbusManager::errorOccurred,
            this, &MainWindow::onModbusError);
    
    // 连接配方管理器错误信号（状态信息通过RecipeWidget统一发送）
    connect(m_recipeManager, &RecipeManager::errorOccurred,
            this, &MainWindow::onModbusError);
    
    // 连接控制面板寄存器写入信号（不显示日志，只执行Modbus写入）
    connect(m_controlPanel, &ControlPanel::sendMessageToMainWindow,
            this, [this](const uint16_t &registerVal) {
                // 调试信息
                qDebug() << "=== MainWindow Modbus写入调试 ===";
                qDebug() << "准备写入寄存器[0]，值:" << QString("0x%1 (%2)").arg(registerVal, 4, 16, QChar('0')).arg(registerVal);
                qDebug() << "连接状态:" << (m_modbusManager ? (m_modbusManager->connectionState() == QModbusDevice::ConnectedState ? "已连接" : "未连接") : "无ModbusManager");
                
                // 实际写入Modbus寄存器[0]
                if (m_modbusManager && m_modbusManager->connectionState() == QModbusDevice::ConnectedState) {
                    bool writeResult = m_modbusManager->writeRegister(0, registerVal);
                    qDebug() << "写入结果:" << (writeResult ? "成功" : "失败");
                } else {
                    qDebug() << "跳过写入：设备未连接";
                }
                qDebug() << "==============================";
            });
    
    // 连接操作描述信号
    connect(m_controlPanel, &ControlPanel::sendOperationMessage,
            this, [this](const QString &operationMsg) {
                appendLog("[CONTROL] " + operationMsg);
            });
    
    connect(m_globalParameterSetting, &GlobalParameterSetting::sendMessageToMainWindow,
            this, [this](const QString &msg) {
                appendLog("[PARAM] " + msg);
            });
    
    connect(m_singleMoverControl, &SingleMoverControl::readModbusRegisterToMainWindow,
            this, [this](int address) {
                appendLog(QString("[MODBUS] 读取寄存器地址: 0x%1").arg(address, 4, 16, QChar('0')));
                // 实际的Modbus读取操作（如果连接状态良好）
                if (m_modbusManager && m_modbusManager->connectionState() == QModbusDevice::ConnectedState) {
                    // 执行异步读取操作
                    m_modbusManager->readRegister(address);
                } else {
                    appendLog("[ERROR] Modbus设备未连接，无法读取寄存器");
                }
            });
    
    connect(m_singleMoverControl, &SingleMoverControl::writeModbusRegisterToMainWindow,
            this, [this](int address, const uint16_t &registerVal) {
                appendLog(QString("[MODBUS] 写入寄存器地址: 0x%1, 值: 0x%2")
                                   .arg(address, 4, 16, QChar('0'))
                                   .arg(registerVal, 4, 16, QChar('0')));
                // 实际的Modbus写入操作（如果连接状态良好）
                if (m_modbusManager && m_modbusManager->connectionState() == QModbusDevice::ConnectedState) {
                    // 这里可以添加实际的写入逻辑
                    // m_modbusManager->writeHoldingRegister(address, registerVal);
                }
            });
    
    // 连接配方管理器的日志信号
    connect(m_recipeWidget, &RecipeWidget::logMessage,
            this, [this](const QString &message) {
                appendLog(message);
            });
    
    // 连接Modbus读取结果信号
    connect(m_modbusManager, &ModbusManager::registerDataRead,
            this, [this](quint16 value, int address) {
                appendLog(QString("[MODBUS] 寄存器 0x%1 读取成功，值: 0x%2 (%3)")
                         .arg(address, 4, 16, QChar('0'))
                         .arg(value, 4, 16, QChar('0'))
                         .arg(value));
            });
}

void MainWindow::initUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 主布局：左右分布 - 左侧控制区域 + 右侧日志区域
    QHBoxLayout *mainLayout = createStandardHBoxLayout();
    centralWidget->setLayout(mainLayout);

    // 左侧控制区域
    QWidget *controlWidget = new QWidget();
    QVBoxLayout *controlLayout = createStandardVBoxLayout();
    controlWidget->setLayout(controlLayout);

    // 连接设置
    controlLayout->addWidget(createConnectionArea());

    // 控制标签页
    QTabWidget *tabWidget = new QTabWidget();
    tabWidget->addTab(m_controlPanel, "全局控制");
    tabWidget->addTab(m_globalParameterSetting, "参数设置");
    tabWidget->addTab(m_singleMoverControl, "单动子控制");
    tabWidget->addTab(m_recipeWidget, "配方管理");
    controlLayout->addWidget(tabWidget);

    // 控制区域占用全部空间
    mainLayout->addWidget(controlWidget, 1);
    
    // 延迟初始化日志窗口（在主窗口显示后）
    m_logWindow = nullptr;
    
    // 初始化闪烁定时器
    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setInterval(500); // 500ms闪烁间隔
    connect(m_blinkTimer, &QTimer::timeout, this, &MainWindow::toggleBlinkState);
}


QWidget* MainWindow::createConnectionArea()
{
    QGroupBox *groupBox = new QGroupBox("Modbus TCP 连接设置", this);
    QHBoxLayout *layout = createStandardHBoxLayout();
    groupBox->setLayout(layout);
    layout->setContentsMargins(10, 10, 10, 10);

    layout->addWidget(new QLabel("控制器IP:"));
    ipLineEdit = new QLineEdit("127.0.0.1");
    ipLineEdit->setMinimumWidth(120);
    layout->addWidget(ipLineEdit);

    layout->addWidget(new QLabel("端口:"));
    portSpinBox = new QSpinBox();
    portSpinBox->setRange(1, 65535);
    portSpinBox->setValue(502);
    portSpinBox->setMinimumWidth(80);
    layout->addWidget(portSpinBox);

    connectButton = new QPushButton("连接");
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
    layout->addSpacing(50);
    layout->addWidget(connectButton);

    statusLabel = new QLabel("状态：未连接");
    statusLabel->setStyleSheet("color: #f44336; font-weight: bold;");
    layout->addSpacing(50);
    layout->addWidget(statusLabel);

    
    layout->addStretch();
    
    // 添加日志按钮（在右边，透明背景，无图标）
    m_openLogBtn = new QToolButton(this);
    m_openLogBtn->setText(QStringLiteral("日志"));
    m_openLogBtn->setToolTip(QStringLiteral("打开日志窗口"));
    m_openLogBtn->setAutoRaise(true);
    m_openLogBtn->setStyleSheet("QToolButton { background: transparent; border: none; padding: 4px; }");
    connect(m_openLogBtn, &QToolButton::clicked, this, &MainWindow::openLogWindow);
    layout->addWidget(m_openLogBtn);
    return groupBox;
}


void MainWindow::onConnectButtonClicked()
{
    QModbusDevice::State currentState = m_modbusManager->connectionState();
    
    if (currentState == QModbusDevice::ConnectedState) {
        // 当前已连接，点击按钮执行断开操作
        m_modbusManager->disconnectDevice();
    } else if (currentState == QModbusDevice::ConnectingState) {
        // 当前正在连接中，点击按钮执行取消连接操作
        m_modbusManager->disconnectDevice();
    } else {
        // 当前未连接，点击按钮执行连接操作
        QString ip = ipLineEdit->text().trimmed();
        int port = portSpinBox->value();
        
        if (ip.isEmpty()) {
            QMessageBox::warning(this, "错误", "请输入有效的IP地址");
            return;
        }
        
        m_modbusManager->connectToDevice(ip, port);
    }
}

void MainWindow::onModbusStateChanged(QModbusDevice::State newState)
{
    if (previousState == newState) return;
    
    QString statusText;
    QString statusStyle;
    QString buttonText;
    
    switch (newState) {
        case QModbusDevice::UnconnectedState:
            statusText = "状态：未连接";
            statusStyle = "color: #f44336; font-weight: bold;";
            buttonText = "连接";
            appendLog("[INFO] Modbus连接已断开");
            m_controlPanel->updateConnectionState(false);
            break;
        case QModbusDevice::ConnectingState:
            statusText = "状态：连接中...";
            statusStyle = "color: #ff9800; font-weight: bold;";
            buttonText = "取消";
            appendLog(QString("[INFO] 正在连接到 %1:%2").arg(ipLineEdit->text()).arg(portSpinBox->value()));
            break;
        case QModbusDevice::ConnectedState:
            statusText = "状态：已连接";
            statusStyle = "color: #4caf50; font-weight: bold;";
            buttonText = "断开";
            appendLog(QString("[SUCCESS] 成功连接到 %1:%2").arg(ipLineEdit->text()).arg(portSpinBox->value()));
            m_controlPanel->updateConnectionState(true);
            break;
        case QModbusDevice::ClosingState:
            statusText = "状态：断开中...";
            statusStyle = "color: #ff9800; font-weight: bold;";
            buttonText = "连接";
            break;}
    
    statusLabel->setText(statusText);
    statusLabel->setStyleSheet(statusStyle);
    connectButton->setText(buttonText);
    
    previousState = newState;
}

void MainWindow::onModbusError(const QString& error)
{
    appendLog("[ERROR] " + error);
    QMessageBox::critical(this, "Modbus错误", error);
}

void MainWindow::onStatusChanged(const QString& status)
{
    appendLog(QString("[INFO] 状态变更: %1").arg(status));
}

void MainWindow::onClearLogClicked()
{
    if (m_logWindow) {
        m_logWindow->clearAllLogs();
        appendLog("[INFO] 日志已清空");
    }
}

// 布局工具方法
void MainWindow::setupStandardLayout(QLayout* layout, int spacing)
{
    if (!layout) return;
    layout->setSpacing(spacing);
    layout->setContentsMargins(15, 15, 15, 15);
}

QGroupBox* MainWindow::createStandardGroupBox(const QString& title, QLayout** outLayout)
{
    QGroupBox* groupBox = new QGroupBox(title, this);
    QVBoxLayout* layout = createStandardVBoxLayout();
    groupBox->setLayout(layout);
    
    if (outLayout) {
        *outLayout = layout;
    }
    
    return groupBox;
}

QHBoxLayout* MainWindow::createStandardHBoxLayout()
{
    QHBoxLayout* layout = new QHBoxLayout();
    setupStandardLayout(layout);
    return layout;
}

QVBoxLayout* MainWindow::createStandardVBoxLayout()
{
    QVBoxLayout* layout = new QVBoxLayout();
    setupStandardLayout(layout);
    return layout;
}


/**
 * 追加日志到底部面板，并按级别路由到对应页签。
 * 识别规则：包含 [ERROR]→错误；包含 [WARN]/[WARNING]→警告；否则→消息。
 * 同时总是写入“全部”。会在最前添加本地时间戳 HH:mm:ss.zzz。
 */
void MainWindow::appendLog(const QString& text)
{
    // 如果日志窗口还未创建，先创建它
    if (!m_logWindow) {
        m_logWindow = new LogWindow(this);
        connect(m_logWindow, &LogWindow::windowClosed, this, &MainWindow::onLogWindowClosed);
    }
    
    const QString ts = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    const QString line = QString("[%1] %2").arg(ts, text);

    // 输出到所有日志页面
    if (m_logWindow->getLogAllWidget()) {
        m_logWindow->getLogAllWidget()->append(line);
    }

    // 检测消息级别并触发相应的闪烁
    const QString upper = text.toUpper();
    if (upper.contains("[ERROR]")) {
        // 错误消息：红色闪烁
        if (m_logWindow->getLogErrorWidget()) {
            m_logWindow->getLogErrorWidget()->append(line);
        }
        startBlinking("red");
    } else if (upper.contains("[WARN]") || upper.contains("[WARNING]")) {
        // 警告消息：黄色闪烁
        if (m_logWindow->getLogWarnWidget()) {
            m_logWindow->getLogWarnWidget()->append(line);
        }
        startBlinking("yellow");
    } else {
        // 普通消息：不闪烁
        if (m_logWindow->getLogInfoWidget()) {
            m_logWindow->getLogInfoWidget()->append(line);
        }
    }
}


void MainWindow::openLogWindow()
{
    // 停止闪烁（用户已查看日志）
    stopBlinking();
    
    // 首次打开时创建日志窗口
    if (!m_logWindow) {
        m_logWindow = new LogWindow(this);
        connect(m_logWindow, &LogWindow::windowClosed, this, &MainWindow::onLogWindowClosed);
        
        // 确保窗口在主窗口附近显示
        QRect mainRect = this->geometry();
        int x = mainRect.x() + 50;  // 右移50像素
        int y = mainRect.y() + 50;  // 下移50像素
        m_logWindow->move(x, y);
    }
    
    if (m_logWindow->isVisible()) {
        m_logWindow->activateWindow();
        m_logWindow->raise();
    } else {
        m_logWindow->show();
    }
}


void MainWindow::onLogWindowClosed()
{
    // 日志窗口关闭时的处理逻辑（如果需要）
}


void MainWindow::startBlinking(const QString& color)
{
    if (!m_openLogBtn) return;
    
    // 如果已经在闪烁相同颜色，不重复启动
    if (m_isBlinking && m_blinkColor == color) return;
    
    m_blinkColor = color;
    m_isBlinking = true;
    m_blinkState = false;
    
    m_blinkTimer->start();
}


void MainWindow::stopBlinking()
{
    if (!m_openLogBtn) return;
    
    m_isBlinking = false;
    m_blinkTimer->stop();
    
    // 恢复透明背景
    m_openLogBtn->setStyleSheet("QToolButton { background: transparent; border: none; padding: 4px; }");
}


void MainWindow::toggleBlinkState()
{
    if (!m_openLogBtn || !m_isBlinking) return;
    
    m_blinkState = !m_blinkState;
    
    QString style;
    if (m_blinkState) {
        // 显示闪烁颜色
        if (m_blinkColor == "yellow") {
            style = "QToolButton { background: #ffeb3b; border: 1px solid #fbc02d; padding: 4px; border-radius: 3px; }";
        } else if (m_blinkColor == "red") {
            style = "QToolButton { background: #f44336; border: 1px solid #d32f2f; padding: 4px; border-radius: 3px; color: white; }";
        }
    } else {
        // 恢复透明背景
        style = "QToolButton { background: transparent; border: none; padding: 4px; }";
    }
    
    m_openLogBtn->setStyleSheet(style);
}


void MainWindow::initThemeMenu()
{
    if (!menuBar()) return;
    if (!m_themeMenu) {
        m_themeMenu = menuBar()->addMenu(QStringLiteral("主题"));
    }

    if (!m_themeActionGroup) {
        m_themeActionGroup = new QActionGroup(this);
        m_themeActionGroup->setExclusive(true);
    }

    // 工业暗色
    m_actThemeDark = new QAction(QStringLiteral("工业暗色"), this);
    m_actThemeDark->setCheckable(true);
    m_themeActionGroup->addAction(m_actThemeDark);
    m_themeMenu->addAction(m_actThemeDark);
    connect(m_actThemeDark, &QAction::triggered, this, [this]() {
        if (m_themeManager) m_themeManager->applyStyleManagerTheme(StyleManager::DarkIndustrial);
    });

    // 现代浅色
    m_actThemeLight = new QAction(QStringLiteral("现代浅色"), this);
    m_actThemeLight->setCheckable(true);
    m_themeActionGroup->addAction(m_actThemeLight);
    m_themeMenu->addAction(m_actThemeLight);
    connect(m_actThemeLight, &QAction::triggered, this, [this]() {
        if (m_themeManager) m_themeManager->applyStyleManagerTheme(StyleManager::LightModern);
    });

    // 经典蓝
    m_actThemeBlue = new QAction(QStringLiteral("经典蓝"), this);
    m_actThemeBlue->setCheckable(true);
    m_themeActionGroup->addAction(m_actThemeBlue);
    m_themeMenu->addAction(m_actThemeBlue);
    connect(m_actThemeBlue, &QAction::triggered, this, [this]() {
        if (m_themeManager) m_themeManager->applyStyleManagerTheme(StyleManager::ClassicBlue);
    });

    // 蓝色渐变主题（CSS文件）
    m_actThemeBlueGradient = new QAction(QStringLiteral("蓝色渐变"), this);
    m_actThemeBlueGradient->setCheckable(true);
    m_themeActionGroup->addAction(m_actThemeBlueGradient);
    m_themeMenu->addAction(m_actThemeBlueGradient);
    connect(m_actThemeBlueGradient, &QAction::triggered, this, [this]() {
        if (m_themeManager) m_themeManager->applyStyleManagerTheme(StyleManager::BlueGradient);
    });

    // 根据当前状态设置勾选
    if (!m_themeManager) return;
    switch (m_themeManager->currentTheme()) {
    case StyleManager::DarkIndustrial: m_actThemeDark->setChecked(true); break;
    case StyleManager::LightModern:    m_actThemeLight->setChecked(true); break;
    case StyleManager::ClassicBlue:    m_actThemeBlue->setChecked(true); break;
    case StyleManager::BlueGradient:   m_actThemeBlueGradient->setChecked(true); break;
    }
}


















