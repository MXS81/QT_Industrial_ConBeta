#include "MainWindow.h"
#include "OverviewPage.h"
#include "JogControlPage.h"
#include "RecipeManagerPage.h"
#include "ModbusManager.h"
#include "LoginDialog.h"
#include "LogWidget.h"
#include "ModbusConfigDialog.h"
#include "StyleUtils.h"
#include "AnimatedButton.h"
#include <QSettings>
#include <QTabWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QAction>
#include <QVBoxLayout>
#include <QApplication>
#include <QTabBar>
#include <QThread>
#include <QRandomGenerator>

struct ModbusConfig;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    // --- 设置默认登录状态为 admin ---
    , m_isLoggedIn(true)        // 1. 设置为“已登录”
    , m_currentUser("admin")      // 2. 默认用户设置为“admin”
    // --- 调试模式 ---
    , m_modbusManager(nullptr)
    , m_modbusConnected(false)
    , m_systemReady(false)
    , m_emergencyStopActive(false)
    , m_overviewPage(nullptr)
    , m_jogPage(nullptr)
    , m_recipePage(nullptr)
    , m_globalLogWidget(nullptr)
    , m_isEmergencyStopPressed(false)
{
    try {
        setWindowTitle("SKZR 轨道控制系统 V2.0");
        setMinimumSize(1400, 900);

        // // 显示登录对话框
        // if (!showLoginDialog()) {
        //     qDebug() << "用户取消登录，程序将退出";
        //     QTimer::singleShot(0, qApp, &QApplication::quit);
        //     return;
        // }

        // 初始化数据
        initializeMovers();
        // 验证初始化结果
        if (m_movers.isEmpty()) {
            qCritical() << "动子初始化失败！";
            QMessageBox::critical(this, "初始化错误", "动子数据初始化失败，程序将退出！");
            QTimer::singleShot(0, qApp, &QApplication::quit);
            return;
        }
        // 创建UI
        setupUI();
        createMenuBar();
        createToolBar();

        createStatusBar();

        // 初始化Modbus管理器
        initializeModbus();

        // 创建定时器
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateSystemStatus);
        m_updateTimer->start(100);

        // 延迟触发初始更新
        QTimer::singleShot(500, this, [this]() {
            updateSystemStatus();

            if (m_overviewPage) {
                m_overviewPage->updateMovers(m_movers);
            }
        });

        // 暂时不设置复杂样式
        setStyleSheet(R"(
            QMainWindow { background-color: #1a1a2e; }
            QDialog { background-color: #1a1a2e; color: white; }
            QPushButton {
                background-color: #533483;
                color: white;
                border: none;
                padding: 8px 15px;
                border-radius: 4px;
            }
            QPushButton:hover { background-color: #e94560; }
            QSpinBox, QDoubleSpinBox {
                background-color: #0f3460;
                color: white;
                border: 1px solid #3a3a5e;
                padding: 5px;
            }
            QLineEdit, QComboBox {
                background-color: #0f3460;
                color: white;
                border: 1px solid #3a3a5e;
                padding: 5px;
            }
            QGroupBox {
                color: #e94560;
                border: 2px solid #3a3a5e;
                border-radius: 5px;
                margin-top: 10px;
                padding-top: 10px;
            }
            QLabel { color: white; }
        )");
        // 更新权限
        updateUIPermissions();
        // 初始日志
        addLogEntry(QString("系统启动完成 - 用户: %1").arg(m_currentUser), "success");
    } catch (const std::exception& e) {
        qCritical() << "MainWindow构造异常:" << e.what();
        QMessageBox::critical(nullptr, "程序错误",
                              QString("主窗口创建失败: %1").arg(e.what()));
        throw;
    } catch (...) {
        qCritical() << "MainWindow构造未知异常";
        QMessageBox::critical(nullptr, "程序错误", "主窗口创建遇到未知异常");
        throw;
    }
}

// 实现addLogEntry方法
void MainWindow::addLogEntry(const QString &message, const QString &type)
{
    // 同时添加到全局日志和当前页面日志
    addGlobalLogEntry(message, type);
    // 如果OverviewPage存在，转发日志到那里
    if (m_overviewPage) {
        m_overviewPage->addLogEntry(message, type);
    } else {
        // 如果OverviewPage还未创建，暂时输出到控制台
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        qDebug() << QString("[%1] %2: %3").arg(timestamp,type.toUpper(),message);
    }
}

// 提供给其他页面调用的全局日志接口
void MainWindow::logToGlobal(const QString &message, const QString &type, const QString &source)
{
    QString formattedMessage = QString("[%1] %2").arg(source, message);
    addGlobalLogEntry(formattedMessage, type);
}

MainWindow::~MainWindow()
{
    if (m_modbusManager) {
        m_modbusManager->disconnectFromDevice();
    }
}

bool MainWindow::isSimulationMode() const
{
    return !m_modbusConnected;
}

// 初始化Modbus连接
void MainWindow::initializeModbus()
{
    m_modbusManager = new ModbusManager(this);
    m_modbusManager->setMainWindow(this);

    // 连接Modbus信号
    connect(m_modbusManager, &ModbusManager::connected,
            this, &MainWindow::onModbusConnected);
    connect(m_modbusManager, &ModbusManager::disconnected,
            this, &MainWindow::onModbusDisconnected);
    connect(m_modbusManager, &ModbusManager::connectionError,
            this, &MainWindow::onModbusError);
    connect(m_modbusManager, &ModbusManager::dataReceived,
            this, &MainWindow::onModbusDataReceived);

    addLogEntry("Modbus管理器已初始化", "info");

    // 延迟尝试连接（可选）
    /*QTimer::singleShot(2000, this, [this]() {
            m_modbusManager->connectToDevice("192.168.1.100", 502, 1);
        });*/
}

// 在MainWindow的构造函数中，确保初始化时加载配置

void MainWindow::applyModbusConfig(const ModbusConfig &config)
{
    addLogEntry("开始应用Modbus配置", "info");

    try {
        // 如果当前有连接，先断开
        if (m_modbusManager && m_modbusManager->isConnected()) {
            addLogEntry("断开现有Modbus连接", "info");
            m_modbusManager->disconnectFromDevice();
        }

        // 等待断开完成
        QThread::msleep(100);

        // 应用新配置并尝试连接
        bool connectResult = false;

        if (config.type == ModbusConfig::TCP) {
            if (m_modbusManager) { // 先判空
                connectResult = m_modbusManager->connectToDevice(config.host, config.port, config.deviceId);
            }
        } else {
            if (m_modbusManager) { // 先判空
                connectResult = m_modbusManager->connectToSerialDevice(config.serialPort, config.baudRate, config.deviceId);
            }
        }

        if (connectResult) {
            addLogEntry("Modbus配置应用成功", "success");
        } else {
            addLogEntry("Modbus连接失败", "error");
        }

    } catch (const std::exception& e) {
        addLogEntry(QString("应用Modbus配置异常: %1").arg(e.what()), "error");
    }
}

ModbusConfig MainWindow::loadModbusConfigFromSettings()
{
    ModbusConfig config; // 使用默认值

    try {
        QSettings settings;
        settings.beginGroup("ModbusConfig");

        if (settings.contains("type")) {
            config.type = static_cast<ModbusConfig::ConnectionType>(settings.value("type").toInt());
            config.host = settings.value("host", config.host).toString();
            config.port = settings.value("port", config.port).toInt();
            config.serialPort = settings.value("serialPort", config.serialPort).toString();
            config.baudRate = settings.value("baudRate", config.baudRate).toInt();
            config.deviceId = settings.value("deviceId", config.deviceId).toInt();
            config.timeout = settings.value("timeout", config.timeout).toInt();

            addLogEntry("Modbus配置已从注册表加载", "info");
        } else {
            addLogEntry("注册表中未找到Modbus配置，使用默认值", "warning");
        }

        settings.endGroup();

    } catch (const std::exception& e) {
        addLogEntry(QString("加载Modbus配置失败: %1").arg(e.what()), "error");
    }

    return config;
}

void MainWindow::initializeModbusFromSettings()
{
    addLogEntry("初始化Modbus配置", "info");

    // 加载保存的配置
    ModbusConfig config = loadModbusConfigFromSettings();

    // 如果用户选择自动连接，则应用配置
    QSettings settings;
    bool autoConnect = settings.value("Modbus/AutoConnect", false).toBool();

    if (autoConnect) {
        addLogEntry("检测到自动连接设置，开始建立Modbus连接", "info");

        // 延迟2秒后连接，给系统初始化时间
        QTimer::singleShot(2000, this, [this, config]() {
            applyModbusConfig(config);
        });
    } else {
        addLogEntry("自动连接已禁用，等待手动配置", "info");
    }
}

// Modbus错误处理
void MainWindow::onModbusError(const QString &error)
{

    // 如果是测试模式，不记录错误
    if (m_isTestingConnection) {
        return;
    }
    addLogEntry(QString("Modbus错误: %1").arg(error), "error");
}

// 简化版的数据接收处理（避免复杂的寄存器解析）
void MainWindow::onModbusDataReceived(int startAddress, const QVector<quint16> &data)
{
    QMutexLocker locker(&m_dataUpdateMutex);
    // 根据接收到的数据块的起始地址，判断其内容并分发处理
    // 使用新的寄存器地址表来判断数据类型
    if (startAddress == ModbusRegisters::MoverStatus::BASE_ADDRESS) {
        processMoversStatusData(startAddress, data);
    }
    // 使用正确的 SystemStatus 命名空间来判断是否是系统状态数据
    else if (startAddress == ModbusRegisters::SystemStatus::SYSTEM_READY) {
        processSystemStatusData(startAddress, data);
    }
}

// 线圈数据接收处理
void MainWindow::onModbusCoilsReceived(int startAddress, const QVector<bool> &data)
{
    qDebug() << QString("收到线圈数据 - 起始地址:%1, 数据量:%2").arg(startAddress).arg(data.size());

    for (int i = 0; i < data.size(); ++i) {
        int address = startAddress + i;
        bool value = data[i];

        switch (address) {
        case 0:  // SYSTEM_INIT
            if (value) {
                addLogEntry("PLC系统初始化激活", "info");
            }
            break;
        case 1:  // SELECT_MODE
            break;
        case 2:  // JOG_BACKWARD
            if (value) {
                addLogEntry("PLC确认JOG后退指令", "info");
            }
            break;
        case 3:  // JOG_FORWARD
            if (value) {
                addLogEntry("PLC确认JOG前进指令", "info");
            }
            break;
        case 10: // SYSTEM_ENABLE
            if (value) {
                addLogEntry("PLC系统已使能", "success");
            } else {
                addLogEntry("PLC系统已禁用", "warning");
            }
            break;
        }
    }
}

// 系统状态变化处理
void MainWindow::onSystemStatusChanged(bool initialized, bool enabled)
{
    QString status = "系统状态: ";
    if (initialized && enabled) {
        status += "运行中";
        m_statusLabel->setStyleSheet("color: #22c55e;");
        m_systemReady = true;
    } else if (initialized && !enabled) {
        status += "已初始化，未使能";
        m_statusLabel->setStyleSheet("color: #fbbf24;");
        m_systemReady = false;
    } else {
        status += "未初始化";
        m_statusLabel->setStyleSheet("color: #ef4444;");
        m_systemReady = false;
    }

    // 只在没有错误信息时更新状态
    if (!m_statusLabel->text().contains("错误") && !m_statusLabel->text().contains("紧急")) {
        m_statusLabel->setText(status);
    }
}

bool MainWindow::showLoginDialog()
{
    LoginDialog loginDialog(this);

    if (loginDialog.exec() == QDialog::Accepted) {
        m_currentUser = loginDialog.getUsername();
        m_isLoggedIn = true;

        // 判断用户权限级别
        if (m_currentUser == "admin") {
            // 管理员权限
            QMessageBox::information(this, "登录成功",
                                     QString("欢迎管理员 %1！\n您拥有完全控制权限。").arg(m_currentUser));
        } else {
            // 普通操作员
            QMessageBox::information(this, "登录成功",
                                     QString("欢迎操作员 %1！\n您拥有基本操作权限。").arg(m_currentUser));
        }

        return true;
    }

    return false;
}

void MainWindow::setupUI()
{
    // 创建中央标签页部件
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);

    //创建全局日志组件
    m_globalLogWidget = new LogWidget("全局日志", this);
    m_globalLogWidget->setMaxLines(1000);
    m_globalLogWidget->setShowUser(true);
    m_globalLogWidget->setAutoScroll(true);

    // 创建系统概览页面作为第一个标签页
    //m_overviewPage = new OverviewPage(m_currentUser, this);
    m_overviewPage = new OverviewPage(&m_movers, m_currentUser, this);
    int overviewIndex = m_tabWidget->addTab(m_overviewPage, "📊 系统概览");

    // 设置系统概览页面不可关闭
    m_tabWidget->tabBar()->setTabButton(overviewIndex, QTabBar::RightSide, nullptr);

    // 确保系统概览是当前显示的页面
    m_tabWidget->setCurrentIndex(overviewIndex);

    // 连接信号
    connect(this, &MainWindow::moversUpdated, m_overviewPage, &OverviewPage::updateMovers);
    connect(this, &MainWindow::userChanged, m_overviewPage, &OverviewPage::onUserChanged);
    connect(this, &MainWindow::currentRecipeChanged,
            m_overviewPage, [this](int id, const QString &name) {
                // 在这里可以更新概览页的配方信息
                // 例如：m_overviewPage->updateRecipeInfo(id, name);
            });
    onRecipeApplied(m_currentRecipeId, m_currentRecipeName);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        // 只允许关闭非系统概览页面（index != 0）
        if (index > 0) {
            QWidget *widget = m_tabWidget->widget(index);
            m_tabWidget->removeTab(index);

            // 如果是系统日志页面被关闭，清理引用
            if (widget == m_globalLogWidget) {
                // 不删除m_globalLogWidget，因为可能需要重新打开
                addGlobalLogEntry("系统日志页面已关闭", "info");
            }
            // 如果是JOG控制页面被关闭，清理引用
            else if (widget == m_jogPage) {
                m_jogPage = nullptr;
            }
            // 如果是配方管理页面被关闭，清理引用
            else if (widget == m_recipePage) {
                m_recipePage = nullptr;
            }
        }
    });
    setCentralWidget(m_tabWidget);
    // 立即发送一次当前的动子数据（如果有的话）
    if (!m_movers.isEmpty()) {
        QTimer::singleShot(100, this, [this]() {
            emit moversUpdated(m_movers);
        });
    }
}

// 实现全局日志方法
void MainWindow::addGlobalLogEntry(const QString &message, const QString &type)
{
    if (m_globalLogWidget) {
        m_globalLogWidget->addLogEntry(message, type, m_currentUser);
    }
}

void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = this->menuBar();

    // 文件菜单
    QMenu *fileMenu = menuBar->addMenu("文件(&F)");
    QAction *logoutAction = fileMenu->addAction("注销(&L)");
    QAction *switchUserAction = fileMenu->addAction("切换用户(&S)");
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("退出(&X)");

    connect(logoutAction, &QAction::triggered, this, &MainWindow::onLoginLogout);
    connect(switchUserAction, &QAction::triggered,this, [this]() {
        if (showLoginDialog()) {
            emit userChanged(m_currentUser);
            updateUIPermissions();
        }
    });
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    // 控制菜单
    QMenu *controlMenu = menuBar->addMenu("控制(&C)");
    QAction *jogAction = controlMenu->addAction("Jog控制(&J)");
    QAction *recipeAction = controlMenu->addAction("配方管理(&R)");
    controlMenu->addSeparator();
    QAction *emergencyAction = controlMenu->addAction("紧急停止(&E)");
    emergencyAction->setShortcut(Qt::Key_F1);

    connect(jogAction, &QAction::triggered, this, &MainWindow::openJogControl);
    connect(recipeAction, &QAction::triggered, this, &MainWindow::openRecipeManager);
    connect(emergencyAction, &QAction::triggered, this, &MainWindow::onEmergencyStop);

    // 视图菜单
    QMenu *viewMenu = menuBar->addMenu("视图(&V)");
    QAction *systemLogAction = viewMenu->addAction("系统日志(&L)");
    QAction *fullScreenAction = viewMenu->addAction("全屏(&F)");
    connect(systemLogAction, &QAction::triggered, this, &MainWindow::openSystemLog);
    connect(fullScreenAction, &QAction::triggered, this,[this]() {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });

    // 帮助菜单
    QMenu *helpMenu = menuBar->addMenu("帮助(&H)");
    QAction *aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, this,[this]() {
        QMessageBox::about(this, "关于",
                           "SKZR 轨道控制系统 V2.0\n\n"
                           "当前用户: " + m_currentUser + "\n"
                                                 "系统版本: 2.0.0\n");
    });
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar("主工具栏");
    toolBar->setMovable(false);

    // 快速访问按钮
    //QPushButton *overviewBtn = new QPushButton("📊 概览");
    QPushButton *systemLogBtn = new QPushButton("📋 系统日志");
    QPushButton *jogBtn = new QPushButton("🕹 Jog控制");
    QPushButton *recipeBtn = new QPushButton("📝 配方管理");
    QPushButton *initBtn = new QPushButton("🔄 系统初始化");

    // 用户登录/注销按钮
    m_loginLogoutButton = new QPushButton("👤 " + m_currentUser);
    m_loginLogoutButton->setProperty("class", "info");

    m_emergencyButton = new AnimatedButton("急停");
    StyleUtils::setButtonType(m_emergencyButton, StyleUtils::Emergency);

    // 设置脉冲动画
    static_cast<AnimatedButton*>(m_emergencyButton)->setAnimationType(AnimatedButton::Pulse);
    static_cast<AnimatedButton*>(m_emergencyButton)->startAnimation();

    toolBar->addWidget(m_loginLogoutButton);
    toolBar->addSeparator();
    toolBar->addWidget(systemLogBtn);
    //toolBar->addWidget(overviewBtn);
    toolBar->addWidget(jogBtn);
    toolBar->addWidget(recipeBtn);
    toolBar->addSeparator();
    toolBar->addWidget(initBtn);

    // 添加一个弹簧，占据所有可用空间
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(spacer);
    toolBar->addWidget(m_emergencyButton);


    // 保存按钮引用以便后续根据权限启用/禁用
    jogBtn->setObjectName("jogBtn");
    recipeBtn->setObjectName("recipeBtn");
    initBtn->setObjectName("initBtn");

    // 连接信号
    // connect(overviewBtn, &QPushButton::clicked, this,[this]() {
    //     m_tabWidget->setCurrentIndex(0);
    // });
    connect(jogBtn, &QPushButton::clicked, this, &MainWindow::openJogControl);
    connect(recipeBtn, &QPushButton::clicked, this, &MainWindow::openRecipeManager);
    connect(initBtn, &QPushButton::clicked, this, &MainWindow::onSystemInit);
    connect(m_emergencyButton, &QPushButton::clicked, this, &MainWindow::onEmergencyStop);
    connect(m_loginLogoutButton, &QPushButton::clicked, this, &MainWindow::onLoginLogout);
    connect(systemLogBtn, &QPushButton::clicked, this, &MainWindow::openSystemLog);

    // 设置特殊按钮样式
    StyleUtils::setButtonType(m_emergencyButton, StyleUtils::Emergency);

    // 可以根据用户权限动态设置样式
    if (m_currentUser == "admin") {
        StyleUtils::setButtonType(m_loginLogoutButton, StyleUtils::Success);
    } else {
        StyleUtils::setButtonType(m_loginLogoutButton, StyleUtils::Info);
    }

}

void MainWindow::openSystemLog()
{
    // 检查是否已经打开系统日志标签页
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->tabText(i).contains("系统日志")) {
            m_tabWidget->setCurrentIndex(i);
            addGlobalLogEntry("切换到系统日志页面", "info");
            return;
        }
    }

    // 如果没有打开，则添加系统日志标签页
    if (m_globalLogWidget) {
        int index = m_tabWidget->addTab(m_globalLogWidget, "📋 系统日志");
        m_tabWidget->setCurrentIndex(index);

        // 添加日志记录
        addGlobalLogEntry(QString("用户 %1 打开了系统日志").arg(m_currentUser), "info");
    }else {
        QMessageBox::warning(this, "错误", "系统日志组件未初始化！");
    }
}

void MainWindow::createStatusBar()
{
    qDebug() << "createStatusBar 开始";

    try {
        QStatusBar *statusBar = this->statusBar();
        if (!statusBar) {
            qCritical() << "无法获取状态栏";
            return;
        }
        // 设置简化样式
        statusBar->setStyleSheet(R"(
            QStatusBar {
                background-color: #0f3460;
                color: white;
                border-top: 1px solid #3a3a5e;
            }
            QLabel {
                color: white;
                padding: 0 10px;
            }
        )");

        // 创建状态标签
        m_statusLabel = new QLabel("系统就绪");
        if (!m_statusLabel) {
            qCritical() << "无法创建状态标签";
            return;
        }

        m_timeLabel = new QLabel();
        if (!m_timeLabel) {
            qCritical() << "无法创建时间标签";
            return;
        }

        m_userLabel = new QLabel(QString("当前用户: %1 | 权限: %2")
                                     .arg(m_currentUser
                                     ,m_currentUser == "admin" ? "管理员" : "操作员"));
        if (!m_userLabel) {
            qCritical() << "无法创建用户标签";
            return;
        }

        // 添加 Modbus状态标签
        m_modbusStatusLabel = new QLabel("Modbus: 断开");
        if (!m_modbusStatusLabel) {
            qCritical() << "无法创建Modbus状态标签";
            return;
        }
        m_modbusStatusLabel->setStyleSheet("color: #ef4444; font-weight: bold;");

        // 添加 Modbus连接按钮
        m_modbusConnectBtn = new QPushButton("连接PLC");
        if (!m_modbusConnectBtn) {
            qCritical() << "无法创建Modbus连接按钮";
            return;
        }

        m_modbusConnectBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #22c55e;
                color: white;
                border: none;
                padding: 5px 10px;
                border-radius: 3px;
                font-size: 11px;
            }
            QPushButton:hover {
                background-color: #16a34a;
            }
        )");

        // 连接按钮信号 - 安全的连接方式
        connect(m_modbusConnectBtn, &QPushButton::clicked, this, [this]() {

            try {
                if (m_modbusConnected) {
                    if (m_modbusManager) {
                        m_modbusManager->disconnectFromDevice();
                    }
                } else {
                    // 弹出配置对话框
                    ModbusConfigDialog configDialog;
                    if (configDialog.exec() == QDialog::Accepted) {
                        ModbusConfig config = configDialog.getConfig();

                        if (m_modbusManager) {
                            if (config.type == ModbusConfig::TCP) {
                                addLogEntry(QString("正在尝试连接到 %1:%2...").arg(config.host).arg(config.port), "info");
                                m_modbusManager->connectToDevice(config.host, config.port, config.deviceId);
                            } else {
                                addLogEntry(QString("正在尝试连接到串口 %1...").arg(config.serialPort), "info");
                                m_modbusManager->connectToSerialDevice(config.serialPort, config.baudRate, config.deviceId);
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                qCritical() << "Modbus按钮点击处理异常:" << e.what();
                QMessageBox::warning(this, "错误", QString("连接操作失败: %1").arg(e.what()));
            } catch (...) {
                qCritical() << "Modbus按钮点击处理未知异常";
                QMessageBox::warning(this, "错误", "连接操作遇到未知错误");
            }
        });

        // 添加到状态栏
        statusBar->addWidget(m_statusLabel);
        statusBar->addPermanentWidget(m_modbusStatusLabel);
        statusBar->addPermanentWidget(m_modbusConnectBtn);
        statusBar->addPermanentWidget(m_userLabel);
        statusBar->addPermanentWidget(m_timeLabel);

        // 设置用户标签样式
        m_userLabel->setStyleSheet("color: #00ff41; font-weight: bold;");    
    } catch (const std::exception& e) {
        qCritical() << "createStatusBar异常:" << e.what();
        QMessageBox::critical(this, "错误", QString("创建状态栏失败: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "createStatusBar未知异常";
        QMessageBox::critical(this, "错误", "创建状态栏遇到未知错误");
    }
}

// Modbus事件处理函数
void MainWindow::onModbusConnected()
{
    m_modbusConnected = true;
    m_modbusStatusLabel->setText("Modbus: 已连接");
    m_modbusStatusLabel->setStyleSheet("color: #22c55e; font-weight: bold;");
    m_modbusConnectBtn->setText("断开PLC");

    m_statusLabel->setText("PLC已连接，系统就绪");

    // 启动循环读取
    m_modbusManager->startCyclicRead(200);  // 200ms间隔

    // 记录日志
    if (m_overviewPage) {
        m_overviewPage->addLogEntry("Modbus PLC连接成功", "success");
    }

    QMessageBox::information(this, "连接成功", "已成功连接到PLC设备！");
}

void MainWindow::onModbusDisconnected()
{
    m_modbusConnected = false;
    m_modbusStatusLabel->setText("Modbus: 断开");
    m_modbusStatusLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
    m_modbusConnectBtn->setText("连接PLC");

    m_statusLabel->setText("PLC连接断开，切换到模拟模式");
    m_statusLabel->setStyleSheet("color: #fbbf24;");

    addLogEntry("PLC连接断开，切换到模拟模式", "warning");
}

void MainWindow::processMoversStatusData(int startAddress, const QVector<quint16> &data)
{
    // 使用正确的寄存器命名空间
    int baseAddr = ModbusRegisters::MoverStatus::BASE_ADDRESS;
    // 注意：这里的解析逻辑需要根据PLC的真实数据结构来调整。
    int registersPerMover = ModbusRegisters::MoverStatus::REGISTERS_PER_MOVER;
    for (int i = 0; i < data.size(); i += 2) {
        int registerAddr = startAddress + i;
        int moverIndex = (registerAddr - baseAddr) / registersPerMover;
        int registerOffset = (registerAddr - baseAddr) % registersPerMover;

        if (moverIndex >= 0 && moverIndex < m_movers.size() && (i + 1) < data.size()) {
            MoverData &mover = m_movers[moverIndex];
            qint32 value = static_cast<qint32>((data[i+1] << 16) | data[i]);

            switch (registerOffset) {
            case ModbusRegisters::MoverStatus::POSITION_LOW:
                mover.position = value / 1000.0; // 假设PLC以微米为单位
                mover.lastUpdateTime = QDateTime::currentDateTime();
                mover.plcConnected = true;
                break;
            case ModbusRegisters::MoverStatus::VELOCITY_LOW:
                mover.speed = value / 1000.0; // 假设PLC以微米/秒为单位
                break;
            case ModbusRegisters::MoverStatus::TARGET_LOW:
                mover.target = value / 1000.0;
                break;
            }
        }
    }
}

void MainWindow::updateMoverStatus(MoverData &mover, quint16 statusWord)
{
    QString oldStatus = mover.status;
    mover.hasError = false;
    mover.errorMessage = "";

    // 解析状态字 (根据PLC实际定义调整)
    if (statusWord & 0x0001) {  // 位0: 使能
        if (statusWord & 0x0002) {  // 位1: 运行中
            mover.status = "运行中";
        } else if (statusWord & 0x0004) {  // 位2: 到位
            mover.status = "停止";
            mover.inPosition = true;
        } else {
            mover.status = "就绪";
        }
    } else {
        mover.status = "禁用";
    }

    if (statusWord & 0x0008) {  // 位3: 错误
        mover.hasError = true;
        mover.status = "错误";
    }

    if (statusWord & 0x0010) {  // 位4: 紧急停止
        mover.status = "紧急停止";
    }

    // 记录状态变化
    if (oldStatus != mover.status) {
        addLogEntry(QString("动子%1 状态变化: %2 -> %3")
                        .arg(mover.id).arg(oldStatus).arg(mover.status), "info");
    }
}

void MainWindow::updateMoverError(MoverData &mover, quint16 errorCode)
{
    if (errorCode != 0) {
        mover.hasError = true;

        // 根据错误码映射错误信息 (根据PLC实际定义调整)
        switch (errorCode) {
        case 1:
            mover.errorMessage = "位置超限";
            break;
        case 2:
            mover.errorMessage = "速度超限";
            break;
        case 3:
            mover.errorMessage = "通信超时";
            break;
        case 4:
            mover.errorMessage = "碰撞检测";
            break;
        default:
            mover.errorMessage = QString("未知错误 (代码:%1)").arg(errorCode);
            break;
        }

        addLogEntry(QString("动子%1 错误: %2").arg(mover.id).arg(mover.errorMessage), "error");
    } else {
        mover.hasError = false;
        mover.errorMessage = "";
    }
}

void MainWindow::processSystemStatusData(int startAddress, const QVector<quint16> &data)
{
    for (int i = 0; i < data.size(); ++i) {
        int address = startAddress + i;
        quint16 value = data[i];

        switch (address) {
        // 使用正确的寄存器命名空间
        case ModbusRegisters::SystemStatus::SYSTEM_READY:
            m_systemReady = (value != 0);
            break;
        case ModbusRegisters::SystemStatus::SYSTEM_ERROR:
            if (value != 0) addLogEntry(QString("系统错误: 错误码 %1").arg(value), "error");
            break;
        case ModbusRegisters::SystemStatus::EMERGENCY_STOP:
            if (value != 0 && !m_emergencyStopActive) {
                m_emergencyStopActive = true;
                onEmergencyStopFromPLC();
            } else if (value == 0) {
                m_emergencyStopActive = false;
            }
            break;
            // ... 其他系统状态处理 ...
        }
    }
}

void MainWindow::onEmergencyStopFromPLC()
{
    // 同步本地状态
    for (auto &mover : m_movers) {
        mover.speed = 0;
        mover.status = "紧急停止";
    }

    m_statusLabel->setText("⚠️ PLC紧急停止");
    m_statusLabel->setStyleSheet("color: #ef4444; font-weight: bold;");

    // 显示警告对话框
    QMessageBox::critical(this, "PLC紧急停止",
                          "检测到PLC设备触发紧急停止！\n所有动子已停止运动。");

    // 记录日志
    if (m_overviewPage) {
        m_overviewPage->addLogEntry("PLC触发紧急停止", "error");
    }
}

void MainWindow::updateUIPermissions()
{
    // 根据用户权限更新UI
    bool isAdmin = (m_currentUser == "admin");

    // 查找工具栏按钮并设置权限
    QToolBar *toolBar = findChild<QToolBar*>();
    if (toolBar) {
        // 配方管理和系统初始化仅管理员可用
        if (auto btn = toolBar->findChild<QPushButton*>("recipeBtn")) {
            btn->setEnabled(isAdmin);
            if (!isAdmin) {
                btn->setToolTip("需要管理员权限");
            }
        }
        if (auto btn = toolBar->findChild<QPushButton*>("initBtn")) {
            btn->setEnabled(isAdmin);
            if (!isAdmin) {
                btn->setToolTip("需要管理员权限");
            }
        }
    }

    // 更新用户按钮文本
    if (m_loginLogoutButton) {
        m_loginLogoutButton->setText("👤 " + m_currentUser);
    }

    // 更新状态栏
    if (m_userLabel) {
        m_userLabel->setText(QString("当前用户: %1 | 权限: %2")
                                 .arg(m_currentUser,isAdmin ? "管理员" : "操作员"));
    }
}

void MainWindow::initializeMovers()
{
    qDebug() << "MainWindow::initializeMovers 开始";
    // 清空现有数据
    m_movers.clear();
    // 修改为单动子系统，预留扩展接口
    const int MOVER_COUNT = 1; // 可配置的动子数量，将来可通过配置文件设置

    // 初始化动子，确保每个动子都有有效的数据
    for (int i = 0; i < MOVER_COUNT; ++i) {
        MoverData mover(i, 0.0);  // 单动子从位置0开始
        mover.id = i;
        mover.temperature = 25.0;
        mover.status = "停止";
        mover.speed = 0.0;
        mover.targetSpeed = 100.0;
        mover.acceleration = 500.0;
        mover.target = mover.position;
        mover.inPosition = true;
        mover.hasError = false;
        mover.plcConnected = false;
        mover.lastUpdateTime = QDateTime::currentDateTime();
        mover.lastCommandTime = QDateTime::currentDateTime();
        mover.lastCommand = "初始化";

        m_movers.append(mover);
        qDebug() << QString("初始化动子 %1，位置：%2mm").arg(i).arg(mover.position);
    }

    qDebug() << "动子初始化完成，总数：" << m_movers.size();
    emit moversUpdated(m_movers);
}

int MainWindow::getMoverCount() const
{
    QSettings settings;
    return settings.value("System/MoverCount", 1).toInt(); // 默认1个动子
}

void MainWindow::setMoverCount(int count)
{
    if (count < 1 || count > 8) { // 限制最大8个动子
        qWarning() << "动子数量超出范围，使用默认值1";
        count = 1;
    }

    QSettings settings;
    settings.setValue("System/MoverCount", count);

    // 重新初始化动子
    initializeMovers();
}

void MainWindow::updateSystemStatus()
{
    // 更新时间
    if (m_timeLabel) {
        m_timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    }

    // 确保动子列表不为空
    if (m_movers.isEmpty()) {
        qWarning() << "updateSystemStatus: 动子列表为空，重新初始化";
        initializeMovers();
        return;
    }

    try {
        if (m_modbusConnected) {
            // PLC连接模式：数据来自ModbusManager的信号
            // 这里不需要做额外处理，数据通过信号更新
            qDebug() << "PLC模式 - 数据通过Modbus信号更新";
        }
        // 发送更新信号（但要避免在JOG操作时产生冲突）
        emit moversUpdated(m_movers);

    } catch (const std::exception& e) {
        qCritical() << "系统状态更新异常：" << e.what();
    } catch (...) {
        qCritical() << "系统状态更新发生未知异常";
    }
}

void MainWindow::openJogControl()
{
    // 检查用户权限
    if (m_currentUser != "admin" && m_currentUser != "operator") {
        QMessageBox::warning(this, "权限不足", "您没有权限使用Jog控制功能！");
        return;
    }

    // 检查是否已经有Jog控制页面打开
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->tabText(i).contains("Jog控制")) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    // 确保动子数据已正确初始化
    if (m_movers.isEmpty()) {
        qWarning() << "动子列表为空，重新初始化";
        initializeMovers();
    }

    // 验证动子数据完整性
    bool needsValidation = false;
    for (int i = 0; i < m_movers.size(); ++i) {
        MoverData &mover = m_movers[i];
        if (mover.id != i) {
            qWarning() << "动子ID不匹配，修正中...";
            mover.id = i;
            needsValidation = true;
        }
        // 确保状态字符串不为空
        if (mover.status.isEmpty()) {
            mover.status = "停止";
            needsValidation = true;
        }
        // 确保目标位置有效
        if (mover.target < 0 || mover.target >= 7455.75) {
            mover.target = mover.position;
            needsValidation = true;
        }
    }

    if (needsValidation) {
        qDebug() << "动子数据已验证和修正";
    }

    try {
        // 创建JogControlPage并传递有效的动子列表指针
        m_jogPage = new JogControlPage(&m_movers, m_currentUser, m_modbusManager, this);

        if (!m_jogPage) {
            qCritical() << "无法创建JogControlPage";
            QMessageBox::critical(this, "错误", "无法创建Jog控制页面！");
            return;
        }

        int index = m_tabWidget->addTab(m_jogPage, "🕹 Jog控制");
        m_tabWidget->setCurrentIndex(index);

        // 连接信号，确保数据同步
        connect(this, &MainWindow::moversUpdated, m_jogPage, &JogControlPage::updateMovers);

        // 立即发送一次初始数据
        QTimer::singleShot(100, this, [this]() {
            if (m_jogPage) {
                m_jogPage->updateMovers(m_movers);
            }
        });

        addLogEntry(QString("Jog控制页面已创建 - 用户: %1 (模式: %2)")
                        .arg(m_currentUser).arg(m_modbusConnected ? "PLC" : "模拟"), "success");

    } catch (const std::exception& e) {
        qCritical() << "创建Jog控制页面异常：" << e.what();
        QMessageBox::critical(this, "错误", QString("创建Jog控制页面失败：%1").arg(e.what()));
    } catch (...) {
        qCritical() << "创建Jog控制页面发生未知异常";
        QMessageBox::critical(this, "错误", "创建Jog控制页面发生未知错误！");
    }
}

void MainWindow::openRecipeManager()
{
    if (m_currentUser != "admin") {
        QMessageBox::warning(this, "权限不足", "只有管理员才能访问配方管理！");
        return;
    }

    // 检查是否已经打开
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->tabText(i).contains("配方管理")) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    // 创建新的配方管理页面
    m_recipePage = new RecipeManagerPage(&m_movers, m_currentUser,m_modbusManager, this);
    connect(m_recipePage, &RecipeManagerPage::recipeApplied, this, &MainWindow::onRecipeApplied);
    int index = m_tabWidget->addTab(m_recipePage, "📝 配方管理");
    m_tabWidget->setCurrentIndex(index);
}

void MainWindow::onRecipeApplied(int id, const QString &name)
{
    m_currentRecipeId = id;
    m_currentRecipeName = name;
    // 收到后，立即广播出去
    emit currentRecipeChanged(id, name);
}

/**
 * @brief 响应急停按钮的点击事件
 *
 * 该函数在一个布尔标志 m_isEmergencyStopPressed 上切换状态。
 * - 当触发急停时：立即更新所有动子状态为“紧急停止”，并向PLC发送禁用命令。
 * - 当重置急停时：恢复本地动子状态，并向PLC发送使能命令。
 * 同时，函数会更新急停按钮的UI（文本、样式和动画）并发出相应的信号。
 */
void MainWindow::onEmergencyStop()
{
    qDebug() << "急停按钮被点击，当前状态：" << m_isEmergencyStopPressed;

    // 根据当前是否处于急停状态，执行相反的操作
    if (m_isEmergencyStopPressed) {
        // --- 重置急停 ---
        try {
            m_isEmergencyStopPressed = false;

            // 在重置时，向PLC发送“使能”命令以恢复系统
            if (m_modbusManager && m_modbusManager->isConnected()) {
                m_modbusManager->setSingleAxisEnable(true);
            }

            // 更新急停按钮的UI，恢复正常状态
            if (m_emergencyButton) {
                AnimatedButton* animatedBtn = static_cast<AnimatedButton*>(m_emergencyButton);
                if (animatedBtn) {
                    // 停止所有动画
                    animatedBtn->setAnimationType(AnimatedButton::None);
                }
                m_emergencyButton->setText("急停");
                StyleUtils::setButtonType(m_emergencyButton, StyleUtils::Emergency);
            }

            // 重置本地数据模型中动子的状态
            for (auto &mover : m_movers) {
                if (mover.status == "紧急停止") {
                    mover.status = "停止";
                    mover.hasError = false;
                }
            }

            // 发射信号，通知其他页面急停已重置
            emit emergencyStopReset();
            addLogEntry("急停状态已重置", "warning");

        } catch (const std::exception& e) {
            qCritical() << "急停重置异常：" << e.what();
            addLogEntry(QString("急停重置失败: %1").arg(e.what()), "error");
        }

    } else {
        // --- 触发急停 ---
        try {
            m_isEmergencyStopPressed = true;

            // 立即更新本地数据模型中所有动子的状态为“紧急停止”
            for (auto &mover : m_movers) {
                mover.speed = 0;
                mover.status = "紧急停止";
            }

            // 通过ModbusManager向PLC发送禁用命令，这是真正的急停操作
            if (m_modbusManager && m_modbusManager->isConnected()) {
                m_modbusManager->setSingleAxisEnable(false);
            }

            // 立即更新UI，让用户看到状态变化
            emit moversUpdated(m_movers);

            // 发射信号，通知其他页面急停已触发
            emit emergencyStopTriggered();
            addLogEntry("紧急停止已激活", "error");

            // 更新急停按钮的UI，显示为急停状态
            if (m_emergencyButton) {
                m_emergencyButton->setText("⚠️ 已急停");
                AnimatedButton* animatedBtn = static_cast<AnimatedButton*>(m_emergencyButton);
                if (animatedBtn) {
                    // 启动动画以警示用户
                    animatedBtn->setAnimationType(AnimatedButton::Pulse);
                    animatedBtn->startAnimation();
                }
            }
        } catch (const std::exception& e) {
            qCritical() << "急停触发异常：" << e.what();
            addLogEntry(QString("急停触发失败: %1").arg(e.what()), "error");
        }
    }
}

void MainWindow::onSystemInit()
{
    if (m_currentUser != "admin") {
        QMessageBox::warning(this, "权限不足", "只有管理员才能执行系统初始化！");
        return;
    }

    if (QMessageBox::question(this, "系统初始化", "确定要初始化系统吗？\n这将通过PLC使能动子控制器。") == QMessageBox::Yes) {
        if (m_modbusConnected && m_modbusManager) {
            // 系统初始化的核心是“使能”，调用新接口
            if (m_modbusManager->setSingleAxisEnable(true)) {
                addLogEntry("PLC系统使能命令已发送", "success");
            } else {
                QMessageBox::critical(this, "初始化失败", "无法发送使能命令到PLC！");
            }
        } else {
            QMessageBox::warning(this, "PLC未连接", "请先连接PLC设备后再执行初始化！");
        }
    }
}

void MainWindow::onMoverCountChanged(int count)
{
    if (count != m_movers.size()) {
        addLogEntry(QString("系统动子数量将从%1个变更为%2个")
                        .arg(m_movers.size()).arg(count), "warning");

        setMoverCount(count);
        emit moverCountChanged(count);

        // 更新所有相关页面
        if (m_jogPage) {
            m_jogPage->updateMoverSelector();
        }

        addLogEntry(QString("动子数量变更完成，当前系统：%1个动子").arg(count), "success");
    }
}

void MainWindow::loadMoverConfig()
{
    QSettings settings;
    settings.beginGroup("MoverSystem");

    m_moverConfig.count = settings.value("Count", 1).toInt();
    m_moverConfig.trackLength = settings.value("TrackLength", 7455.75).toDouble();
    m_moverConfig.safetyDistance = settings.value("SafetyDistance", 100.0).toDouble();
    m_moverConfig.enableCollisionDetection = settings.value("EnableCollisionDetection", true).toBool();

    settings.endGroup();
}

void MainWindow::saveMoverConfig()
{
    QSettings settings;
    settings.beginGroup("MoverSystem");

    settings.setValue("Count", m_moverConfig.count);
    settings.setValue("TrackLength", m_moverConfig.trackLength);
    settings.setValue("SafetyDistance", m_moverConfig.safetyDistance);
    settings.setValue("EnableCollisionDetection", m_moverConfig.enableCollisionDetection);

    settings.endGroup();
}

void MainWindow::onLoginLogout()
{
    // 判断当前用户是否为“访客”
    if (m_currentUser == "访客") {
        // 如果是访客，直接弹出登录对话框
        if (showLoginDialog()) {
            // 仅在登录成功后执行更新操作
            emit userChanged(m_currentUser);
            updateUIPermissions();
            addLogEntry(QString("用户 %1 登录成功").arg(m_currentUser), "success");
        }
        // 如果用户取消登录，则什么也不做，保持访客状态

    } else {
        // 如果是已登录用户，则执行注销流程
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle("注销确认");
        msgBox.setText(QString("用户 %1，确定要注销吗？").arg(m_currentUser));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);

        if (msgBox.exec() == QMessageBox::Yes) {
            // 用户确认注销
            addLogEntry(QString("用户 %1 已注销").arg(m_currentUser), "warning");

            // 重置为访客状态
            m_currentUser = "访客";
            m_isLoggedIn = false;

            // 更新UI权限和显示
            emit userChanged(m_currentUser);
            updateUIPermissions();

            // 关闭所有可关闭的标签页
            while (m_tabWidget->count() > 1) {
                m_tabWidget->removeTab(1);
            }
        }
        // 如果用户取消注销，则什么也不做
    }
}

void MainWindow::onUserLoginSuccess(const QString& username)
{
    m_currentUser = username;
    m_isLoggedIn = true;
    emit userChanged(username);
    updateUIPermissions();
}

void MainWindow::onTabChanged(int index)
{
    // 可以在这里处理标签页切换事件
}

void MainWindow::onModbusConnectButtonClicked()
{
    if (m_modbusConnected) {
        // 断开连接
        if (m_modbusManager) {
            m_modbusManager->disconnectFromDevice();
        }
    } else {
        // 弹出配置对话框
        ModbusConfigDialog *configDialog = new ModbusConfigDialog(ModbusConfig(), this);

        if (configDialog->exec() == QDialog::Accepted) {
            ModbusConfig config = configDialog->getConfig();

            // 设置非测试模式
            m_isTestingConnection = false;

            if (m_modbusManager) {
                if (config.type == ModbusConfig::TCP) {
                    addLogEntry(QString("正在尝试连接到 %1:%2...").arg(config.host).arg(config.port), "info");
                    m_modbusManager->connectToDevice(config.host, config.port, config.deviceId);
                } else {
                    addLogEntry(QString("正在尝试连接到串口 %1...").arg(config.serialPort), "info");
                    m_modbusManager->connectToSerialDevice(config.serialPort, config.baudRate, config.deviceId);
                }
            }
        }

        configDialog->deleteLater();
    }
}

void MainWindow::saveModbusConfigToSettings(const ModbusConfig &config)
{
    QSettings settings;
    settings.beginGroup("ModbusConfig");

    settings.setValue("type", static_cast<int>(config.type));
    settings.setValue("host", config.host);
    settings.setValue("port", config.port);
    settings.setValue("serialPort", config.serialPort);
    settings.setValue("baudRate", config.baudRate);
    settings.setValue("dataBits", config.dataBits);
    settings.setValue("stopBits", config.stopBits);
    settings.setValue("parity", config.parity);
    settings.setValue("deviceId", config.deviceId);
    settings.setValue("timeout", config.timeout);
    settings.setValue("retries", config.retries);

    settings.endGroup();

    addLogEntry("Modbus配置已保存到注册表", "info");
}

void MainWindow::openModbusConfig()
{
    try {
        // 加载当前配置
        ModbusConfig currentConfig = loadModbusConfigFromSettings();

        // 创建配置对话框，确保传递this指针
        ModbusConfigDialog *configDialog = new ModbusConfigDialog(currentConfig, this);

        // 连接日志信号
        connect(configDialog, &ModbusConfigDialog::logMessage,
                this, &MainWindow::addLogEntry);

        addLogEntry("打开Modbus配置对话框", "info");

        if (configDialog->exec() == QDialog::Accepted) {
            ModbusConfig newConfig = configDialog->getConfig();

            // 保存配置
            saveModbusConfigToSettings(newConfig);

            // 应用配置
            applyModbusConfig(newConfig);

            addLogEntry("Modbus配置已更新并应用", "success");
        } else {
            addLogEntry("用户取消了Modbus配置", "info");
        }

        // 清理对话框
        configDialog->deleteLater();

    } catch (const std::exception& e) {
        addLogEntry(QString("打开Modbus配置时发生异常: %1").arg(e.what()), "error");
    }
}

