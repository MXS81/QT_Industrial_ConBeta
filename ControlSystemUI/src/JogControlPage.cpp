// JogControlPage.cpp
#include "JogControlPage.h"
#include "TrackWidget.h"
#include "MainWindow.h"
#include "LogWidget.h"
#include <QVBoxLayout>
#include <QSplitter>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QTimer>
#include <QSpinBox>
#include <QComboBox>
#include <QDateTime>
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QAbstractSpinBox>
#include <QDebug>
#include <QCheckBox>
#include <QGroupBox>

const double JogControlPage::TRACK_LENGTH = 7455.75;
const double JogControlPage::SAFETY_DISTANCE = 100.0;

JogControlPage::JogControlPage(QList<MoverData> *movers, const QString& currentUser, ModbusManager *modbusManager, QWidget *parent)
    : QWidget(parent)
    , m_movers(movers)
    , m_selectedMover(0)
    , m_currentUser(currentUser)
    , m_modbusManager(modbusManager)
    , m_longPressTimer(new QTimer(this))
    , m_continuousJogTimer(new QTimer(this))
    , m_realTimeUpdateTimer(new QTimer(this))
    , m_isRealTimeEnabled(false)
    , m_isLongPressing(false)
    , m_isContinuousJogging(false)
    , m_jogDirection(true)
    , m_autoRunTimer(new QTimer(this))
    , m_isAutoRunActive(false)
    , m_isAutoRunPaused(false)
    , m_autoRunStartBtn(nullptr)
    , m_autoRunPauseBtn(nullptr)
    , m_autoRunStopBtn(nullptr)
    , m_autoRunGroup(nullptr)
    , m_autoRunEnableCheckBox(nullptr)
    , m_mainWindow(qobject_cast<MainWindow*>(parent))
{
    // 设置定时器
    m_longPressTimer->setSingleShot(true);
    m_realTimeUpdateTimer->setSingleShot(false);
    m_continuousJogTimer->setSingleShot(false);
    m_autoRunTimer->setInterval(AUTO_RUN_INTERVAL);

    // 连接定时器信号
    connect(m_longPressTimer, &QTimer::timeout, this, &JogControlPage::startContinuousJog);
    connect(m_continuousJogTimer, &QTimer::timeout, this, &JogControlPage::performContinuousJog);
    connect(m_realTimeUpdateTimer, &QTimer::timeout, this, &JogControlPage::onRealTimeDataUpdate);
    connect(m_autoRunTimer, &QTimer::timeout, this, &JogControlPage::performAutoRunStep);

    setupUI();
    updateMoverInfo();

    addLogEntry(QString("Jog控制页面已加载 - 操作员: %1").arg(m_currentUser), "success");

    if (m_modbusManager && m_modbusManager->isConnected()) {
        startRealTimeUpdates();
    } else {
        setControlsEnabled(false);
        addLogEntry("PLC未连接，请连接PLC以启用控制功能。", "warning");
    }


    // 在构造函数的最后添加
    if (m_mainWindow) {
            connect(m_mainWindow, &MainWindow::emergencyStopTriggered, this, &JogControlPage::onEmergencyStopTriggered);
            connect(m_mainWindow, &MainWindow::emergencyStopReset, this, &JogControlPage::onEmergencyStopReset);
    }

}

JogControlPage::~JogControlPage()
{
    stopLongPressDetection();
}

void JogControlPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 顶部用户信息
    QLabel *userInfoLabel = new QLabel(QString("👤 操作员: %1").arg(m_currentUser));
    userInfoLabel->setStyleSheet("color: #00ff41; font-size: 13px; font-weight: bold;");
    mainLayout->addWidget(userInfoLabel);

    // 创建主要的水平布局
    QHBoxLayout *topLayout = new QHBoxLayout();

    // 提前创建自动运行UI，以便移动到左侧面板
    setupAutoRunUI();

    // 左侧面板：控制面板
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftPanel->setMaximumWidth(300);
    leftPanel->setMinimumWidth(280);

    // === 动子选择组 ===
    QGroupBox *selectionGroup = new QGroupBox("动子选择");
    selectionGroup->setStyleSheet(R"(
        QGroupBox {
            font-size: 12px;
            font-weight: bold;
            border: 2px solid #533483;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 8px;
            background-color: #16213e;
            color: white;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 8px 0 8px;
        }
    )");

    QVBoxLayout *selectionLayout = new QVBoxLayout(selectionGroup);
    m_moverSelector = new QComboBox();
    updateMoverSelector();
    selectionLayout->addWidget(m_moverSelector);

    // === 使能控制组 ===
    m_enableGroup = new QGroupBox("动子使能控制");
    m_enableGroup->setStyleSheet(R"(
        QGroupBox {
            font-size: 12px;
            font-weight: bold;
            border: 2px solid #533483;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 8px;
            background-color: #16213e;
            color: white;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 8px 0 8px;
        }
    )");

    QVBoxLayout *enableLayout = new QVBoxLayout(m_enableGroup);

    // 使能状态显示
    m_enableStatusLabel = new QLabel("状态: 未知");
    m_enableStatusLabel->setStyleSheet(R"(
        QLabel {
            color: #fbbf24;
            font-size: 11px;
            font-weight: bold;
            background-color: rgba(251, 191, 36, 0.1);
            border: 1px solid rgba(251, 191, 36, 0.3);
            border-radius: 4px;
            padding: 5px;
        }
    )");
    m_enableStatusLabel->setAlignment(Qt::AlignCenter);

    // 使能控制按钮
    QWidget *enableButtonWidget = new QWidget();
    QHBoxLayout *enableButtonLayout = new QHBoxLayout(enableButtonWidget);

    m_enableBtn = new QPushButton("使能");
    m_disableBtn = new QPushButton("禁用");

    QString enableBtnStyle = R"(
        QPushButton {
            background-color: #22c55e;
            color: white;
            border: none;
            padding: 6px 12px;
            font-size: 11px;
            font-weight: bold;
            border-radius: 4px;
        }
        QPushButton:hover { background-color: #16a34a; }
    )";

    QString disableBtnStyle = enableBtnStyle;
    disableBtnStyle.replace("#22c55e", "#ef4444").replace("#16a34a", "#dc2626");

    m_enableBtn->setStyleSheet(enableBtnStyle);
    m_disableBtn->setStyleSheet(disableBtnStyle);

    enableButtonLayout->addWidget(m_enableBtn);
    enableButtonLayout->addWidget(m_disableBtn);

    enableLayout->addWidget(m_enableStatusLabel);
    enableLayout->addWidget(enableButtonWidget);

    // === 当前状态组（创建但不显示） ===
    m_currentStatusGroup = new QGroupBox("当前状态");
    m_currentStatusGroup->setStyleSheet(R"(
        QGroupBox {
            font-size: 12px;
            font-weight: bold;
            border: 2px solid #533483;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 8px;
            background-color: #16213e;
            color: white;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 8px 0 8px;
        }
    )");

    QGridLayout *statusLayout = new QGridLayout(m_currentStatusGroup);
    statusLayout->setSpacing(5);

    QLabel *posLabel = new QLabel("位置:");
    posLabel->setStyleSheet("color: white; font-size: 11px;");
    m_positionLabel = new QLabel("0.0 mm");
    m_positionLabel->setStyleSheet("color: #00ff41; font-weight: bold; font-size: 11px;");

    QLabel *spdLabel = new QLabel("速度:");
    spdLabel->setStyleSheet("color: white; font-size: 11px;");
    m_speedLabel = new QLabel("0.0 mm/s");
    m_speedLabel->setStyleSheet("color: #00ff41; font-weight: bold; font-size: 11px;");

    QLabel *stLabel = new QLabel("状态:");
    stLabel->setStyleSheet("color: white; font-size: 11px;");
    m_statusLabel = new QLabel("未知");
    m_statusLabel->setStyleSheet("color: #fbbf24; font-weight: bold; font-size: 11px;");

    statusLayout->addWidget(posLabel, 0, 0);
    statusLayout->addWidget(m_positionLabel, 0, 1);
    statusLayout->addWidget(spdLabel, 1, 0);
    statusLayout->addWidget(m_speedLabel, 1, 1);
    statusLayout->addWidget(stLabel, 2, 0);
    statusLayout->addWidget(m_statusLabel, 2, 1);

    // === 位置控制组 (MOVED) ===
    QGroupBox *positionGroup = new QGroupBox("位置控制");
    positionGroup->setStyleSheet(R"(
        QGroupBox {
            font-size: 12px;
            font-weight: bold;
            border: 2px solid #533483;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 8px;
            background-color: #16213e;
            color: white;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 8px 0 8px;
        }
    )");

    QGridLayout *positionLayout = new QGridLayout(positionGroup);

    QLabel *targetLabel = new QLabel("目标位置:");
    targetLabel->setStyleSheet("color: white; font-size: 11px;");
    m_targetPosSpinBox = new QDoubleSpinBox();
    m_targetPosSpinBox->setRange(0, TRACK_LENGTH);
    m_targetPosSpinBox->setSuffix(" mm");
    m_targetPosSpinBox->setDecimals(1);
    m_targetPosSpinBox->setStyleSheet(R"(
        QDoubleSpinBox {
            background-color: #16213e;
            color: white;
            border: 1px solid #533483;
            padding: 3px;
            border-radius: 4px;
        }
    )");

    QLabel *speedLabel2 = new QLabel("移动速度:");
    speedLabel2->setStyleSheet("color: white; font-size: 11px;");
    m_speedSpinBox = new QSpinBox();
    m_speedSpinBox->setRange(10, 1000); // 设置速度范围
    m_speedSpinBox->setValue(100);      // 设置默认速度
    m_speedSpinBox->setSuffix(" mm/s");  // 添加单位后缀
    m_speedSpinBox->setStyleSheet(R"(
    QSpinBox {
        background-color: #16213e;
        color: white;
        border: 1px solid #533483;
        padding: 3px;
        border-radius: 4px;
    }
)");

    m_goToBtn = new QPushButton("移动到位置");
    m_stopBtn = new QPushButton("停止");

    QString controlBtnStyle = R"(
        QPushButton {
            background-color: #22c55e;
            color: white;
            border: none;
            padding: 8px 15px;
            font-size: 12px;
            font-weight: bold;
            border-radius: 5px;
        }
        QPushButton:hover { background-color: #16a34a; }
    )";
    m_goToBtn->setStyleSheet(controlBtnStyle);
    m_stopBtn->setStyleSheet(controlBtnStyle.replace("#22c55e", "#ef4444").replace("#16a34a", "#dc2626"));

    positionLayout->addWidget(targetLabel, 0, 0);
    positionLayout->addWidget(m_targetPosSpinBox, 0, 1, 1, 2);
    positionLayout->addWidget(speedLabel2, 1, 0);
    positionLayout->addWidget(m_speedSpinBox, 1, 1, 1, 2);
    positionLayout->addWidget(m_goToBtn, 2, 0, 1, 3);
    positionLayout->addWidget(m_stopBtn, 3, 0, 1, 3);

    // === JOG控制组 ===
    QGroupBox *jogGroup = new QGroupBox("Jog控制 (单击/长按)");
    jogGroup->setStyleSheet(R"(
        QGroupBox {
            font-size: 12px;
            font-weight: bold;
            border: 2px solid #533483;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 8px;
            background-color: #16213e;
            color: white;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 8px 0 8px;
        }
    )");

    QVBoxLayout *jogLayout = new QVBoxLayout(jogGroup);

    QLabel *stepLabel = new QLabel("步长:");
    stepLabel->setStyleSheet("color: white; font-size: 11px;");
    m_jogStepSpinBox = new QSpinBox();
    m_jogStepSpinBox->setRange(1, 1000);
    m_jogStepSpinBox->setValue(10);
    m_jogStepSpinBox->setSuffix(" mm");
    m_jogStepSpinBox->setStyleSheet(R"(
        QSpinBox {
            background-color: #16213e;
            color: white;
            border: 1px solid #533483;
            padding: 3px;
            border-radius: 4px;
        }
    )");

    QHBoxLayout *stepLayout = new QHBoxLayout();
    stepLayout->addWidget(stepLabel);
    stepLayout->addWidget(m_jogStepSpinBox);
    jogLayout->addLayout(stepLayout);

    QHBoxLayout *jogButtonLayout = new QHBoxLayout();
    m_jogForwardBtn = new QPushButton("◀ 后退");
    m_jogBackwardBtn = new QPushButton("前进 ▶");

    QString jogBtnStyle = R"(
        QPushButton {
            background-color: #3b82f6;
            color: white;
            border: none;
            padding: 10px 15px;
            font-size: 12px;
            font-weight: bold;
            border-radius: 6px;
        }
        QPushButton:hover { background-color: #2563eb; }
        QPushButton:pressed { background-color: #1d4ed8; }
    )";

    m_jogForwardBtn->setStyleSheet(jogBtnStyle);
    m_jogBackwardBtn->setStyleSheet(jogBtnStyle);

    jogButtonLayout->addWidget(m_jogForwardBtn);
    jogButtonLayout->addWidget(m_jogBackwardBtn);
    jogLayout->addLayout(jogButtonLayout);

    // 将左侧组件添加到左侧布局
    leftLayout->addWidget(selectionGroup);
    leftLayout->addWidget(m_enableGroup);
    leftLayout->addWidget(positionGroup);
    leftLayout->addWidget(jogGroup);
    if (m_autoRunGroup) {
        leftLayout->addWidget(m_autoRunGroup);
    }
    leftLayout->addStretch();

    // 右侧面板：位置控制和日志
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightPanel->setMaximumWidth(300);
    rightPanel->setMinimumWidth(280);

    // === 操作日志组 ===
    QGroupBox *logGroup = new QGroupBox("操作日志");
    logGroup->setStyleSheet(R"(
        QGroupBox {
            font-size: 12px;
            font-weight: bold;
            border: 2px solid #533483;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 8px;
            background-color: #16213e;
            color: white;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 8px 0 8px;
        }
    )");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);

    m_logWidget = new LogWidget();
    m_logWidget->setMinimumHeight(150);
    logLayout->addWidget(m_logWidget);

    rightLayout->addWidget(logGroup);
    rightLayout->addStretch();

    // 中间：轨道视图（给予最大空间）
    QWidget *centerPanel = new QWidget();
    QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);

    // 轨道视图组
    QGroupBox *trackGroup = new QGroupBox("轨道可视化");
    trackGroup->setStyleSheet(R"(
        QGroupBox {
            font-size: 13px;
            font-weight: bold;
            border: 2px solid #533483;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 8px;
            background-color: #16213e;
            color: white;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 8px 0 8px;
        }
    )");
    QVBoxLayout *trackLayout = new QVBoxLayout(trackGroup);

    QString trackBtnStyle = R"(
        QPushButton {
            background-color: #6b7280;
            color: white;
            border: none;
            padding: 6px 12px;
            font-size: 11px;
            border-radius: 4px;
        }
        QPushButton:hover { background-color: #4b5563; }
    )";

    // 轨道视图
    m_trackWidget = new TrackWidget(this);
    m_trackScrollArea = new QScrollArea();
    m_trackScrollArea->setWidget(m_trackWidget);
    m_trackScrollArea->setWidgetResizable(true);
    m_trackScrollArea->setMinimumHeight(300);

    trackLayout->addWidget(m_trackScrollArea);
    centerLayout->addWidget(trackGroup);

    // 将三个面板添加到顶部布局
    topLayout->addWidget(leftPanel);
    topLayout->addWidget(centerPanel, 3);
    topLayout->addWidget(rightPanel);

    // 将顶部布局添加到主布局
    mainLayout->addLayout(topLayout);

    // 连接信号
    connect(m_moverSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &JogControlPage::onMoverSelectionChanged);

    connect(m_jogForwardBtn, &QPushButton::pressed, this, &JogControlPage::onJogForwardPressed);
    connect(m_jogForwardBtn, &QPushButton::released, this, &JogControlPage::onJogForwardReleased);
    connect(m_jogBackwardBtn, &QPushButton::pressed, this, &JogControlPage::onJogBackwardPressed);
    connect(m_jogBackwardBtn, &QPushButton::released, this, &JogControlPage::onJogBackwardReleased);
    connect(m_goToBtn, &QPushButton::clicked, this, &JogControlPage::onGoToPosition);
    connect(m_stopBtn, &QPushButton::clicked, this, &JogControlPage::onStopMover);

    connect(m_enableBtn, &QPushButton::clicked, this, [this]() {
        onEnableStateChanged(true);
    });
    connect(m_disableBtn, &QPushButton::clicked, this, [this]() {
        onEnableStateChanged(false);
    });
}

void JogControlPage::setupAutoRunUI()
{
    try {
        m_autoRunGroup = new QGroupBox("动子自动运行");
        if (!m_autoRunGroup) {
            qCritical() << "Failed to create autorun group box";
            return;
        }

        m_autoRunGroup->setStyleSheet(R"(
            QGroupBox {
                font-size: 13px;
                font-weight: bold;
                border: 2px solid #533483;
                border-radius: 8px;
                margin-top: 12px;
                padding-top: 8px;
                background-color: #16213e;
                color: white;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 8px;
                padding: 0 8px 0 8px;
                color: #f8fafc;
            }
        )");

        QVBoxLayout *autoLayout = new QVBoxLayout(m_autoRunGroup);

        // 控制区域
        QHBoxLayout *controlLayout = new QHBoxLayout();

        m_autoRunEnableCheckBox = new QCheckBox("启用");
        m_autoRunEnableCheckBox->setStyleSheet(R"(
            QCheckBox {
                font-size: 12px;
                color: white;
                padding: 5px;
            }
            QCheckBox::indicator {
                width: 16px;
                height: 16px;
                border: 2px solid #533483;
                border-radius: 3px;
                background-color: #16213e;
            }
            QCheckBox::indicator:checked {
                background-color: #22c55e;
                border-color: #22c55e;
            }
        )");

        // 控制按钮
        m_autoRunStartBtn = new QPushButton("开始");
        m_autoRunPauseBtn = new QPushButton("暂停");
        m_autoRunStopBtn = new QPushButton("停止");

        QString btnStyle = R"(
            QPushButton {
                background-color: #533483;
                color: white;
                border: none;
                padding: 6px 12px;
                font-size: 11px;
                font-weight: bold;
                border-radius: 4px;
            }
            QPushButton:hover { background-color: #e94560; }
            QPushButton:disabled { background-color: #555555; color: #888888; }
        )";

        m_autoRunStartBtn->setStyleSheet(btnStyle);
        m_autoRunPauseBtn->setStyleSheet(btnStyle);
        m_autoRunStopBtn->setStyleSheet(btnStyle);

        // 初始状态：禁用控制按钮
        m_autoRunStartBtn->setEnabled(false);
        m_autoRunPauseBtn->setEnabled(false);
        m_autoRunStopBtn->setEnabled(false);

        controlLayout->addWidget(m_autoRunEnableCheckBox);
        controlLayout->addStretch();
        controlLayout->addWidget(m_autoRunStartBtn);
        controlLayout->addWidget(m_autoRunPauseBtn);
        controlLayout->addWidget(m_autoRunStopBtn);

        autoLayout->addLayout(controlLayout);

        // 连接信号
        connectAutoRunSignals();

    } catch (const std::exception& e) {
        qDebug() << "创建自动运行UI异常:" << e.what();
    } catch (...) {
        qDebug() << "创建自动运行UI发生未知异常";
    }
}

/**
 * @brief 辅助函数，用于统一设置所有控制组件的使能状态
 * @param enabled true为启用，false为禁用
 */
void JogControlPage::setControlsEnabled(bool enabled)
{
    // 使能/禁用所有与PLC交互的按钮和输入框
    if (m_enableBtn) m_enableBtn->setEnabled(enabled);
    if (m_disableBtn) m_disableBtn->setEnabled(enabled);
    if (m_goToBtn) m_goToBtn->setEnabled(enabled);
    if (m_stopBtn) m_stopBtn->setEnabled(enabled);
    if (m_jogForwardBtn) m_jogForwardBtn->setEnabled(enabled);
    if (m_jogBackwardBtn) m_jogBackwardBtn->setEnabled(enabled);
    if (m_autoRunGroup) m_autoRunGroup->setEnabled(enabled);
    if (m_targetPosSpinBox) m_targetPosSpinBox->setEnabled(enabled);
    if (m_speedSpinBox) m_speedSpinBox->setEnabled(enabled);
    if (m_jogStepSpinBox) m_jogStepSpinBox->setEnabled(enabled);

    if (!enabled) {
        addLogEntry("所有控件已禁用，请连接PLC。", "warning");
    } else {
        addLogEntry("PLC已连接，控件已启用。", "success");
    }
}

// 更新动子选择器
void JogControlPage::updateMoverSelector()
{
    if (!m_moverSelector || !m_movers) return;

    m_moverSelector->clear();
    for (int i = 0; i < m_movers->size(); ++i) {
        m_moverSelector->addItem(QString("动子 %1").arg(i));
    }

    // 如果只有一个动子，隐藏选择器或禁用
    if (m_movers->size() == 1) {
        m_moverSelector->setEnabled(false);
        m_moverSelector->setToolTip("当前系统只配置了一个动子");
    } else {
        m_moverSelector->setEnabled(true);
        m_moverSelector->setToolTip("选择要控制的动子");
    }
}

// 自动运行控制方法实现
/**
 * @brief 启动/停止自动运行模式
 */
void JogControlPage::onAutoRunModeToggled(bool enabled) {
    if (enabled) {
        m_autoRunStartBtn->setEnabled(true);
        addLogEntry("自动运行模式已启用", "info");
    } else {
        if (m_isAutoRunActive) {
            onAutoRunStop();
        }
        m_autoRunStartBtn->setEnabled(false);
        m_autoRunPauseBtn->setEnabled(false);
        m_autoRunStopBtn->setEnabled(false);
        addLogEntry("自动运行模式已关闭", "info");
    }
}

// 安全连接信号
void JogControlPage::connectAutoRunSignals()
{
    try {
        if (m_autoRunEnableCheckBox) {
            connect(m_autoRunEnableCheckBox, &QCheckBox::toggled,
                    this, &JogControlPage::onAutoRunModeToggled);
        }

        if (m_autoRunStartBtn) {
            connect(m_autoRunStartBtn, &QPushButton::clicked,
                    this, &JogControlPage::onAutoRunStart);
        }

        if (m_autoRunPauseBtn) {
            connect(m_autoRunPauseBtn, &QPushButton::clicked,
                    this, &JogControlPage::onAutoRunPause);
        }

        if (m_autoRunStopBtn) {
            connect(m_autoRunStopBtn, &QPushButton::clicked,
                    this, &JogControlPage::onAutoRunStop);
        }

        qDebug() << "自动运行信号连接完成";

    } catch (const std::exception& e) {
        qCritical() << "连接自动运行信号异常:" << e.what();
    }
}

/**
 * @brief 开始自动运行
 */
void JogControlPage::onAutoRunStart() {
    if (!m_modbusManager || !m_modbusManager->isConnected()) {
        QMessageBox::warning(this, "PLC未连接", "请先连接PLC设备！");
        return;
    }
    m_isAutoRunActive = true;
    m_isAutoRunPaused = false;
    m_autoRunTimer->start();
    m_autoRunStartBtn->setEnabled(false);
    m_autoRunPauseBtn->setEnabled(true);
    m_autoRunStopBtn->setEnabled(true);
    m_autoRunEnableCheckBox->setEnabled(false);
    addLogEntry("自动运行已启动", "success");
}

/**
 * @brief 暂停/恢复自动运行
 */
void JogControlPage::onAutoRunPause() {
    if (m_isAutoRunPaused) {
        m_isAutoRunPaused = false;
        m_autoRunTimer->start();
        m_autoRunPauseBtn->setText("暂停");
        addLogEntry("自动运行已恢复", "info");
    } else {
        m_isAutoRunPaused = true;
        m_autoRunTimer->stop();
        m_autoRunPauseBtn->setText("继续");
        addLogEntry("自动运行已暂停", "warning");
    }
}

/**
 * @brief 停止自动运行
 */
void JogControlPage::onAutoRunStop() {
    m_isAutoRunActive = false;
    m_isAutoRunPaused = false;
    m_autoRunTimer->stop();

    // 发送停止命令，确保动子停下
    if (m_modbusManager && m_modbusManager->isConnected()) {
        m_modbusManager->setSingleAxisJog(0);
    }

    m_autoRunStartBtn->setEnabled(true);
    m_autoRunPauseBtn->setEnabled(false);
    m_autoRunPauseBtn->setText("暂停");
    m_autoRunStopBtn->setEnabled(false);
    m_autoRunEnableCheckBox->setEnabled(true);
    addLogEntry("自动运行已停止", "info");
}


/**
 * @brief 自动运行的定时器执行函数
 *
 * 此函数现在使用新的ModbusManager接口来发送周期性的JOG命令。
 */
void JogControlPage::performAutoRunStep()
{
    if (!m_isAutoRunActive || m_isAutoRunPaused || !m_modbusManager || !m_modbusManager->isConnected()) {
        // 如果中途断开连接，则停止自动运行
        if (m_isAutoRunActive) {
            onAutoRunStop();
            addLogEntry("PLC连接断开，自动运行已停止", "error");
        }
        return;
    }

    // 这是一个非常简单的自动运行逻辑：持续向前JOG
    // 可以在此基础上扩展更复杂的逻辑，例如到达边界后反向
    bool isForward = true; // 可根据需要修改为更复杂的逻辑
    int direction = isForward ? 2 : 1; // 2=前进, 1=后退

    // 使用新的Modbus接口
    m_modbusManager->setSingleAxisRunMode(false); // 确保是手动/点动模式
    m_modbusManager->setSingleAxisJogSpeed(m_speedSpinBox->value());
    m_modbusManager->setSingleAxisJog(direction);
}

/**
 * @brief 按下JOG按钮（前进/后退）
 *
 * 如果PLC未连接，则不执行任何操作。
 */
void JogControlPage::onJogForwardPressed()
{
    if (!m_modbusManager || !m_modbusManager->isConnected()) return;

    m_modbusManager->setSingleAxisRunMode(false); // 设置为手动模式
    m_modbusManager->setSingleAxisJogSpeed(m_speedSpinBox->value());
    m_modbusManager->setSingleAxisJogPosition(m_jogStepSpinBox->value());
    m_modbusManager->setSingleAxisJog(2); // 2 = 向右/前进
    addLogEntry("JOG前进按下", "info");
}

/**
 * @brief 松开JOG按钮
 *
 * 如果PLC未连接，则不执行任何操作。
 */
void JogControlPage::onJogForwardReleased()
{
    if (!m_modbusManager || !m_modbusManager->isConnected()) return;
    m_modbusManager->setSingleAxisJog(0); // 0 = 停止
    addLogEntry("JOG前进松开", "info");
}

void JogControlPage::onJogBackwardPressed()
{
    if (!m_modbusManager || !m_modbusManager->isConnected()) return;

    m_modbusManager->setSingleAxisRunMode(false);
    m_modbusManager->setSingleAxisJogSpeed(m_speedSpinBox->value());
    m_modbusManager->setSingleAxisJogPosition(m_jogStepSpinBox->value());
    m_modbusManager->setSingleAxisJog(1); // 1 = 向左/后退
    addLogEntry("JOG后退按下", "info");
}

void JogControlPage::onJogBackwardReleased()
{
    if (!m_modbusManager || !m_modbusManager->isConnected()) return;
    m_modbusManager->setSingleAxisJog(0); // 0 = 停止
    addLogEntry("JOG后退松开", "info");
}

void JogControlPage::startLongPressDetection(bool isForward)
{

    if (m_isContinuousJogging) {
        return;  // 如果已经在连续JOG中，忽略新的按下事件
    }

    m_isLongPressing = true;
    m_jogDirection = isForward;

    // 启动长按检测定时器
    m_longPressTimer->start(LONG_PRESS_THRESHOLD);

    //addLogEntry(QString("检测到%1按钮按下").arg(isForward ? "前进" : "后退"), "info"); 这个日志输出太频繁了，建议不要开启
}

void JogControlPage::stopLongPressDetection()
{
    bool wasLongPressing = m_isLongPressing;
    bool wasContinuous = m_isContinuousJogging;

    m_isLongPressing = false;
    m_longPressTimer->stop();

    if (m_isContinuousJogging) {
        // 停止连续JOG
        m_isContinuousJogging = false;
        m_continuousJogTimer->stop();
        addLogEntry(QString("停止连续%1").arg(m_jogDirection ? "前进" : "后退"), "info");

        // 确保在PLC模式下发送停止命令
        if (m_modbusManager && m_modbusManager->isConnected()) {
            m_modbusManager->setSingleAxisJog(0); // 0 = 停止
        }

        // 立即更新UI显示
        updateMoverInfo();

    } else if (wasLongPressing && !wasContinuous) {
        addLogEntry("完成一次单击JOG操作", "info");
    }
}

void JogControlPage::startContinuousJog()
{
    if (!m_isLongPressing) {
        return;  // 如果不在长按状态，不启动连续JOG
    }

    m_isContinuousJogging = true;
    addLogEntry(QString("开始连续%1").arg(m_jogDirection ? "前进" : "后退"), "success");

    // 立即执行一次JOG
    performContinuousJog();

    // 启动连续JOG定时器
    m_continuousJogTimer->start(CONTINUOUS_JOG_INTERVAL);
}

void JogControlPage::performContinuousJog()
{
    if (!m_isContinuousJogging || !m_modbusManager || !m_modbusManager->isConnected()) return;
    m_modbusManager->setSingleAxisJog(m_jogDirection ? 2 : 1);
}

void JogControlPage::onGoToPosition()
{
    if (!m_modbusManager || !m_modbusManager->isConnected()) {
        QMessageBox::warning(this, "PLC未连接", "请先连接PLC设备！");
        return;
    }
    double targetPos = m_targetPosSpinBox->value();
    qint32 targetSpeed = m_speedSpinBox->value();
    addLogEntry(QString("发送绝对定位命令: 位置=%1 mm, 速度=%2 mm/s").arg(targetPos).arg(targetSpeed), "info");

    // 使用新接口执行绝对定位
    m_modbusManager->setSingleAxisRunMode(true);
    m_modbusManager->setSingleAxisAutoSpeed(targetSpeed);
    m_modbusManager->setSingleAxisJogPosition(static_cast<qint16>(targetPos)); // 假设位置用16位寄存器
    m_modbusManager->setSingleAxisAutoRun(true);
}

/**
 * @brief 停止动子运动
 *
 * 如果PLC未连接，则弹出警告并返回。
 */
void JogControlPage::onStopMover()
{
    if (!m_modbusManager || !m_modbusManager->isConnected()) {
        QMessageBox::warning(this, "PLC未连接", "请先连接PLC设备！");
        return;
    }
    addLogEntry("发送停止命令", "warning");

    // 使用新接口停止所有运动
    m_modbusManager->setSingleAxisJog(0);
    m_modbusManager->setSingleAxisAutoRun(false);
}

void JogControlPage::updateMoverInfo()
{
    if (!m_movers || m_movers->isEmpty()) {
        qWarning() << "updateMoverInfo: 动子列表为空";
        return;
    }

    if (m_selectedMover >= 0 && m_selectedMover < m_movers->size()) {
        try {
            const MoverData &mover = (*m_movers)[m_selectedMover];

            if (m_positionLabel) {
                m_positionLabel->setText(QString("%1 mm").arg(mover.position, 0, 'f', 1));
            }
            if (m_speedLabel) {
                m_speedLabel->setText(QString("%1 mm/s").arg(mover.speed, 0, 'f', 1));
            }
        } catch (const std::exception& e) {
            qCritical() << "updateMoverInfo 异常：" << e.what();
        } catch (...) {
            qCritical() << "updateMoverInfo 发生未知异常";
        }
    }
}

bool JogControlPage::checkCollision(int moverId, double targetPosition)
{
    if (!m_movers || moverId < 0 || moverId >= m_movers->size()) {
        return false;
    }

    // 单动子系统不需要碰撞检测
    if (m_movers->size() == 1) {
        return true;
    }

    try {
        // 多动子系统的碰撞检测
        for (int i = 0; i < m_movers->size(); ++i) {
            if (i == moverId) continue;

            double otherPosition = (*m_movers)[i].position;
            double distance = calculateShortestDistance(targetPosition, otherPosition);

            if (distance < SAFETY_DISTANCE) {
                qDebug() << QString("碰撞检测：动子%1目标位置%2mm与动子%3当前位置%4mm距离过近(%5mm < %6mm)")
                                .arg(moverId).arg(targetPosition, 0, 'f', 1)
                                .arg(i).arg(otherPosition, 0, 'f', 1)
                                .arg(distance, 0, 'f', 1).arg(SAFETY_DISTANCE);
                return false;
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

double JogControlPage::calculateShortestDistance(double pos1, double pos2)
{
    double directDistance = qAbs(pos1 - pos2);
    double wrapDistance = TRACK_LENGTH - directDistance;
    return qMin(directDistance, wrapDistance);
}

double JogControlPage::normalizePosition(double position)
{
    while (position < 0) position += TRACK_LENGTH;
    while (position >= TRACK_LENGTH) position -= TRACK_LENGTH;
    return position;
}

/**
 * @brief 更新所有UI组件以反映最新的动子数据
 */
void JogControlPage::updateMovers(const QList<MoverData> &movers)
{
    if (!m_movers) return;
    *m_movers = movers;
    if (m_trackWidget) {
        m_trackWidget->updateMovers(*m_movers);
    }
    updateMoverInfo();
    if (m_selectedMover >= 0 && m_selectedMover < m_movers->size()) {
        updateEnableStatusDisplay((*m_movers)[m_selectedMover].isEnabled);
    }
}

void JogControlPage::onMoverSelectionChanged(int index)
{
    if (!m_movers || index < 0 || index >= m_movers->size()) {
        qWarning() << "onMoverSelectionChanged: 无效的动子索引：" << index;
        return;
    }

    try {
        // 切换动子时停止当前的JOG操作
        stopLongPressDetection();

        m_selectedMover = index;
        updateMoverInfo();

        if (m_targetPosSpinBox) {
            m_targetPosSpinBox->setValue((*m_movers)[index].position);
        }

        addLogEntry(QString("已选择动子 %1").arg(index), "info");
        // 在这里添加以下两行代码：
        const MoverData &mover = (*m_movers)[index];
        updateEnableStatusDisplay(mover.isEnabled);
    } catch (const std::exception& e) {
        qCritical() << "onMoverSelectionChanged 异常：" << e.what();
    } catch (...) {
        qCritical() << "onMoverSelectionChanged 发生未知异常";
    }
}

/**
 * @brief 使能/禁用动子
 *
 * 如果PLC未连接，则弹出警告并返回。
 * 调用ModbusManager的高级接口向PLC发送命令。
 */
void JogControlPage::onEnableStateChanged(bool enabled)
{
    if (!m_modbusManager || !m_modbusManager->isConnected()) {
        QMessageBox::warning(this, "PLC未连接", "请先连接PLC设备！");
        return;
    }
    // 调用新接口
    bool success = m_modbusManager->setSingleAxisEnable(enabled);
    if (success) {
        addLogEntry(QString("动子%1 %2 命令已发送").arg(m_selectedMover).arg(enabled ? "使能" : "禁用"), "success");
    } else {
        addLogEntry(QString("动子%1 %2 命令发送失败").arg(m_selectedMover).arg(enabled ? "使能" : "禁用"), "error");
    }
}

// 更新使能状态显示
void JogControlPage::updateEnableStatusDisplay(bool enabled)
{
    if (m_enableStatusLabel) {
        if (enabled) {
            m_enableStatusLabel->setText("状态: 已使能");
            m_enableStatusLabel->setStyleSheet(R"(
                QLabel {
                    color: #22c55e;
                    font-size: 13px;
                    font-weight: bold;
                    background-color: rgba(34, 197, 94, 0.1);
                    border: 1px solid rgba(34, 197, 94, 0.3);
                    border-radius: 4px;
                    padding: 5px 10px;
                }
            )");
        } else {
            m_enableStatusLabel->setText("状态: 已禁用");
            m_enableStatusLabel->setStyleSheet(R"(
                QLabel {
                    color: #ef4444;
                    font-size: 13px;
                    font-weight: bold;
                    background-color: rgba(239, 68, 68, 0.1);
                    border: 1px solid rgba(239, 68, 68, 0.3);
                    border-radius: 4px;
                    padding: 5px 10px;
                }
            )");
        }
    }

    // 根据使能状态控制操作按钮
    if (m_enableBtn && m_disableBtn) {
        m_enableBtn->setEnabled(!enabled);
        m_disableBtn->setEnabled(enabled);
    }

    // 控制Jog按钮状态
    bool canJog = enabled && (m_movers && m_selectedMover >= 0 && m_selectedMover < m_movers->size());
    if (m_jogForwardBtn) m_jogForwardBtn->setEnabled(canJog);
    if (m_jogBackwardBtn) m_jogBackwardBtn->setEnabled(canJog);
    if (m_goToBtn) m_goToBtn->setEnabled(canJog);
}

void JogControlPage::onEmergencyStopTriggered()
{
    m_emergencyActive = true;

    // 立即停止所有JOG操作
    stopLongPressDetection();

    // 禁用所有控制按钮
    // 停止自动运行
    if (m_isAutoRunActive) {onAutoRunStop();}
    if (m_jogForwardBtn) m_jogForwardBtn->setEnabled(false);
    if (m_jogBackwardBtn) m_jogBackwardBtn->setEnabled(false);
    if (m_goToBtn) m_goToBtn->setEnabled(false);
    if (m_enableBtn) m_enableBtn->setEnabled(false);
    if (m_disableBtn) m_disableBtn->setEnabled(false);
    // 禁用自动运行控件
    if (m_autoRunGroup) m_autoRunGroup->setEnabled(false);

    // 改变页面背景色提示急停状态
    setStyleSheet("QWidget { background-color: rgba(239, 68, 68, 0.1); }");

    addLogEntry("⚠️ 响应急停信号 - 所有操作已禁用", "error");
}

void JogControlPage::onEmergencyStopReset()
{
    m_emergencyActive = false;

    // 重新启用控制按钮
    if (m_jogForwardBtn) m_jogForwardBtn->setEnabled(true);
    if (m_jogBackwardBtn) m_jogBackwardBtn->setEnabled(true);
    if (m_goToBtn) m_goToBtn->setEnabled(true);
    if (m_enableBtn) m_enableBtn->setEnabled(true);
    if (m_disableBtn) m_disableBtn->setEnabled(true);
    // 重新启用自动运行控件
    if (m_autoRunGroup) m_autoRunGroup->setEnabled(true);

    // 恢复正常背景
    setStyleSheet("");
    addLogEntry("✅ 急停已重置 - 操作权限已恢复", "success");
}

// 启动实时数据更新
/**
 * @brief 启动实时数据更新定时器
 */
void JogControlPage::startRealTimeUpdates()
{
    if (!m_isRealTimeEnabled && m_modbusManager && m_modbusManager->isConnected()) {
        m_isRealTimeEnabled = true;
        m_realTimeUpdateTimer->start();
        setControlsEnabled(true); // 启用控件
        addLogEntry("启动实时数据更新", "info");
    }
}

// 停止实时数据更新
/**
 * @brief 停止实时数据更新
 */
void JogControlPage::stopRealTimeUpdates()
{
    if (m_isRealTimeEnabled) {
        m_isRealTimeEnabled = false;
        m_realTimeUpdateTimer->stop();
        setControlsEnabled(false); // 禁用控件
        addLogEntry("停止实时数据更新", "info");
    }
}

// 实时数据更新处理
/**
 * @brief 定时器触发的实时数据更新函数
 *
 * 完全依赖ModbusManager从PLC批量读取所有动子的最新数据。
 */
void JogControlPage::onRealTimeDataUpdate()
{
    if (!m_modbusManager || !m_modbusManager->isConnected() || !m_movers) {
        stopRealTimeUpdates();
        return;
    }
    m_modbusManager->readAllMoverData(*m_movers);
    updateMovers(*m_movers);
}

void JogControlPage::addLogEntry(const QString &message, const QString &type)
{
    // 1. 添加到本页面的日志
    m_logWidget->addLogEntry(message, type, m_currentUser);

    // 2. 转发到全局日志
    if (m_mainWindow) {
        m_mainWindow->addGlobalLogEntry(QString("[Jog控制] %1").arg(message), type);
    }
}
