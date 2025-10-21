#include "controlpanel.h"
#include "modbusmanager.h"
#include <QDebug>
#include <QTimer>
#include <QLabel>
#include <QMessageBox>

ControlPanel::ControlPanel(QWidget *parent)
    : QWidget(parent),
      m_register(0),
      m_resetState(false),
      m_startState(false),
      m_pauseState(false),
      m_eStopState(false),
      m_initialState(false),
      m_manualModeState(true),  // 开机默认手动模式
      m_autoModeState(false),
      m_modbusManager(nullptr),
      m_isConnected(false)
{
    // 设置初始寄存器状态 - 默认手动模式
    m_register |= (1 << 5);  // bit5: manual_mode

    initUI();
    updateButtonStates();
}

ControlPanel::~ControlPanel()
{
    // 自动释放Qt控件
}

void ControlPanel::setRegisterValue(uint16_t registerVal)
{
    m_register = registerVal;
}

uint16_t ControlPanel::getRegisterValue() const
{
    return m_register;
}


void ControlPanel::initUI()
{
    // 创建按钮
    m_btnReset = new QPushButton("复位");
    m_btnStart = new QPushButton("启动");
    m_btnPause = new QPushButton("停止");
    m_btnEStop = new QPushButton("紧急停止");
    m_btnInitial = new QPushButton("初始化");
    m_btnManualMode = new QPushButton("手动模式");
    m_btnAutoMode = new QPushButton("自动模式");

    // ===== 按钮样式与尺寸设置 =====
        QList<QPushButton*> buttons = {
        m_btnReset, m_btnStart, m_btnPause,
        m_btnEStop, m_btnInitial, m_btnManualMode, m_btnAutoMode
    };
    foreach (QPushButton* btn, buttons) {
        // 尺寸与交互形态由代码控制；颜色交由全局主题控制        btn->setCursor(Qt::PointingHandCursor);
    }

    // 特殊按钮样式
    // 为主题细化样式设置 objectName（用于 QSS 精细控制配色）
    m_btnEStop->setObjectName("danger");
    m_btnManualMode->setObjectName("modeManual");
    m_btnAutoMode->setObjectName("modeAuto");

    // 颜色由主题控制，无需本地样式

    // 连接信号槽（保持不变）
    connect(m_btnReset, &QPushButton::clicked, this, &ControlPanel::onResetClicked);
    connect(m_btnStart, &QPushButton::clicked, this, &ControlPanel::onStartClicked);
    connect(m_btnPause, &QPushButton::clicked, this, &ControlPanel::onPauseClicked);
    connect(m_btnEStop, &QPushButton::clicked, this, &ControlPanel::onEmergencyStopClicked);
    connect(m_btnInitial, &QPushButton::clicked, this, &ControlPanel::onInitialClicked);
    connect(m_btnManualMode, &QPushButton::clicked, this, &ControlPanel::onManualModeClicked);
    connect(m_btnAutoMode, &QPushButton::clicked, this, &ControlPanel::onAutoModeClicked);

    // ===== 按功能分组布局 =====

    // 1. 运行控制组（启动、暂停、复位）
    QLabel *lblRunControl = new QLabel("运行控制");
    lblRunControl->setStyleSheet("font-weight: bold; margin-top: 10px;");

    QHBoxLayout *runControlLayout = new QHBoxLayout();
    runControlLayout->setSpacing(8); // 组内按钮间距
    runControlLayout->addWidget(m_btnStart);
    runControlLayout->addWidget(m_btnPause);
    runControlLayout->addWidget(m_btnReset);

    // 2. 紧急控制组（紧急停止、初始化）
    QLabel *lblEmergencyControl = new QLabel("紧急控制");
    lblEmergencyControl->setStyleSheet("font-weight: bold;  margin-top: 10px;");

    QHBoxLayout *emergencyLayout = new QHBoxLayout();
    emergencyLayout->setSpacing(8);
    emergencyLayout->addWidget(m_btnEStop);
    emergencyLayout->addWidget(m_btnInitial);

    // 3. 模式切换组（手动模式、自动模式）
    QLabel *lblModeControl = new QLabel("模式切换");
    lblModeControl->setStyleSheet("font-weight: bold;  margin-top: 10px;");

    QHBoxLayout *modeLayout = new QHBoxLayout();
    modeLayout->setSpacing(8);
    modeLayout->addWidget(m_btnManualMode);
    modeLayout->addWidget(m_btnAutoMode);

    // 创建控制按钮区域的水平布局，将三个分组并排显示
    QHBoxLayout *controlButtonsLayout = new QHBoxLayout();
    controlButtonsLayout->setSpacing(30); // 分组之间的间距
    controlButtonsLayout->setContentsMargins(20, 10, 20, 10); // 控制区域内边距
    
    // 运行控制分组（垂直布局）
    QVBoxLayout *runControlGroup = new QVBoxLayout();
    runControlGroup->setSpacing(8);
    runControlGroup->addWidget(lblRunControl);
    runControlGroup->addLayout(runControlLayout);
    runControlGroup->addStretch(); // 底部拉伸
    
    // 紧急控制分组（垂直布局）
    QVBoxLayout *emergencyControlGroup = new QVBoxLayout();
    emergencyControlGroup->setSpacing(8);
    emergencyControlGroup->addWidget(lblEmergencyControl);
    emergencyControlGroup->addLayout(emergencyLayout);
    emergencyControlGroup->addStretch(); // 底部拉伸
    
    // 模式切换分组（垂直布局）
    QVBoxLayout *modeControlGroup = new QVBoxLayout();
    modeControlGroup->setSpacing(8);
    modeControlGroup->addWidget(lblModeControl);
    modeControlGroup->addLayout(modeLayout);
    modeControlGroup->addStretch(); // 底部拉伸
    
    // 将三个分组添加到水平布局中
    controlButtonsLayout->addLayout(runControlGroup);
    controlButtonsLayout->addLayout(emergencyControlGroup);
    controlButtonsLayout->addLayout(modeControlGroup);
    controlButtonsLayout->addStretch(); // 右侧拉伸项

    // 创建主布局（垂直布局）
    QVBoxLayout *mainlayout = new QVBoxLayout();
    m_trackWidget = new TrackWidget(this);
    
    // 轨道视图在上方，控制按钮在下方
    mainlayout->addWidget(m_trackWidget, 1); // 轨道视图占用大部分空间
    mainlayout->addLayout(controlButtonsLayout, 0); // 控制按钮固定高度
    mainlayout->setContentsMargins(10, 10, 10, 10);
    mainlayout->setSpacing(15); // 轨道视图和控制区域之间的间距

    // 设置主布局
    setLayout(mainlayout);
}



void ControlPanel::handleRisingEdge(int bitPosition, bool &currentState)
{
    bool previousState = currentState;
    currentState = true;  // 按钮按下

    // 上升沿检测：如果之前是false，现在是true
    if (!previousState && currentState) {
        uint16_t oldRegister = m_register;
        m_register |= (1 << bitPosition);  // 设置相应位
        
        // 详细调试信息
        qDebug() << "=== 控制按钮调试信息 ===";
        qDebug() << "按钮位置:" << bitPosition;
        qDebug() << "操作前寄存器值:" << QString("0x%1 (%2)").arg(oldRegister, 4, 16, QChar('0')).arg(oldRegister);
        qDebug() << "操作后寄存器值:" << QString("0x%1 (%2)").arg(m_register, 4, 16, QChar('0')).arg(m_register);
        qDebug() << "二进制表示:" << QString::number(m_register, 2).rightJustified(16, '0');
        qDebug() << "设置的bit位:" << QString("bit%1 = %2").arg(bitPosition).arg((m_register >> bitPosition) & 1);
        qDebug() << "========================";
    }
    emit sendMessageToMainWindow(m_register);
}

void ControlPanel::updateButtonStates()
{
    // 紧急停止按钮状态特殊处理（高电平有效）
    m_btnEStop->setChecked(m_eStopState);

    // 手动模式和自动模式是互斥的
    m_btnManualMode->setChecked(m_manualModeState);
    m_btnAutoMode->setChecked(m_autoModeState);

    // 初始化按钮仅在手动模式下可用
    m_btnInitial->setEnabled(m_manualModeState && m_isConnected);
}

// 设置Modbus管理器引用
void ControlPanel::setModbusManager(ModbusManager* modbusManager)
{
    m_modbusManager = modbusManager;
}

// 更新连接状态
void ControlPanel::updateConnectionState(bool connected)
{
    m_isConnected = connected;
    updateButtonStates();
    
    // 根据连接状态启用/禁用所有控制按钮
    QList<QPushButton*> controlButtons = {
        m_btnReset, m_btnStart, m_btnPause, m_btnEStop,
        m_btnInitial, m_btnManualMode, m_btnAutoMode
    };
    
    foreach (QPushButton* btn, controlButtons) {
        btn->setEnabled(connected);
        // 未连接时设置视觉提示
        // no-op
    }
}

// 检查连接状态
bool ControlPanel::checkConnection()
{
    if (!m_modbusManager || !m_isConnected) {
        emit sendOperationMessage("操作失败：设备未连接");
        return false;
    }
    return true;
}

void ControlPanel::onResetClicked()
{
    if (!checkConnection()) return;
    
    qDebug() << "用户点击：复位按钮 (应设置bit0)";
    handleRisingEdge(0, m_resetState);
    emit sendOperationMessage("系统已复位");
    // 复位后自动清除状态（上升沿有效）
    QTimer::singleShot(100, [this]() {
        m_resetState = false;
        m_register &= ~(1 << 0);  // 清除bit0
        emit sendMessageToMainWindow(m_register);
    });
}

void ControlPanel::onStartClicked()
{
    if (!checkConnection()) return;
    
    qDebug() << "用户点击：启动按钮 (应设置bit1)";
    handleRisingEdge(1, m_startState);
    emit sendOperationMessage("系统已启动");
    // 启动信号上升沿有效，之后清除
    QTimer::singleShot(100, [this]() {
        m_startState = false;
        m_register &= ~(1 << 1);  // 清除bit1
        emit sendMessageToMainWindow(m_register);
    });
}

void ControlPanel::onPauseClicked()
{
    if (!checkConnection()) return;
    
    qDebug() << "用户点击：停止按钮 (应设置bit2)";
    handleRisingEdge(2, m_pauseState);
    emit sendOperationMessage("系统已停止");
    // 暂停信号上升沿有效，之后清除
    QTimer::singleShot(100, [this]() {
        m_pauseState = false;
        m_register &= ~(1 << 2);  // 清除bit2
        emit sendMessageToMainWindow(m_register);
    });
}

void ControlPanel::onEmergencyStopClicked()
{
    if (!checkConnection()) return;
    
    qDebug() << "用户点击：紧急停止按钮 (应操作bit3)";
    // 紧急停止是高电平有效，切换状态
    m_eStopState = !m_eStopState;
    if (m_eStopState) {
        m_register |= (1 << 3);   // 设置bit3
        emit sendOperationMessage("紧急停止已激活");
    } else {
        m_register &= ~(1 << 3);  // 清除bit3
        emit sendOperationMessage("紧急停止已解除");
    }
    emit sendMessageToMainWindow(m_register);
    qDebug() << "Emergency stop" << (m_eStopState ? "activated" : "deactivated")
             << "Register value:" << QString::number(m_register, 2).rightJustified(16, '0');
    updateButtonStates();
}

void ControlPanel::onInitialClicked()
{
    if (!checkConnection()) return;
    
    qDebug() << "用户点击：初始化按钮 (应设置bit4)";
    // 仅在手动模式下有效
    if (m_manualModeState) {
        handleRisingEdge(4, m_initialState);
        emit sendOperationMessage("系统已初始化");
        // 初始化信号上升沿有效，之后清除
        QTimer::singleShot(100, [this]() {
            m_initialState = false;
            m_register &= ~(1 << 4);  // 清除bit4
            emit sendMessageToMainWindow(m_register);
        });
    }
}

void ControlPanel::onManualModeClicked()
{
    if (!checkConnection()) return;
    
    qDebug() << "用户点击：手动模式按钮 (应设置bit5)";
    // 手动模式和自动模式互斥
    if (!m_manualModeState) {
        handleRisingEdge(5, m_manualModeState);
        m_autoModeState = false;
        m_register &= ~(1 << 6);  // 清除自动模式位
        emit sendOperationMessage("切换到手动模式");
        emit sendMessageToMainWindow(m_register);
        updateButtonStates();
    }
}

void ControlPanel::onAutoModeClicked()
{
    if (!checkConnection()) return;
    
    qDebug() << "用户点击：自动模式按钮 (应设置bit6)";
    // 手动模式和自动模式互斥
    if (!m_autoModeState) {
        handleRisingEdge(6, m_autoModeState);
        m_manualModeState = false;
        m_register &= ~(1 << 5);  // 清除手动模式位
        emit sendOperationMessage("切换到自动模式");
        emit sendMessageToMainWindow(m_register);
        updateButtonStates();
    }
}


