#include "singlemovercontrol.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>


SingleMoverControl::SingleMoverControl(QWidget *parent)
    : QWidget{parent}
{

    initUI();

}
void SingleMoverControl::initUI()
{
    QWidget *widget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(widget);
    mainLayout->setSpacing(15);

    QGroupBox *groupBox = new QGroupBox("单动子控制参数", this);
    QFormLayout *formLayout = new QFormLayout(groupBox);
    formLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setSpacing(15);
    formLayout->setContentsMargins(15, 15, 15, 15);

    // 1. 设定使能
    singleEnableButton = new QPushButton("使能");
    singleEnableButton->setStyleSheet(" font-size: 15px;min-width: 200px; max-width: 200px;");//background-color: #f44336;
    //connect(singleEnableButton, &QPushButton::clicked, this, &SingleMoverControl::onSingleEnableClicked);
    formLayout->addRow("1. 设定使能:", singleEnableButton);

    // 2. 手动控制权限
    allowManualCheckBox = new QCheckBox("允许手动控制");
    allowManualCheckBox->setStyleSheet("font-size: 15px;min-width: 200px; max-width: 200px;");
    connect(allowManualCheckBox, &QCheckBox::toggled, [=](bool checked) {

        onAllowManualControlClicked(checked);
        // modeComboBox->setEnabled(checked);
        // if (!checked)modeComboBox->setCurrentIndex(0);
        //logMessage(checked ? "已允许手动控制" : "已禁止手动控制");
    });
    formLayout->addRow("2. 控制权限:", allowManualCheckBox);

    // 3. 运行模式
    modeComboBox = new QComboBox();
    modeComboBox->addItem("点动模式");
    modeComboBox->addItem("自动模式");
    modeComboBox->setStyleSheet("min-width: 200px; max-width: 200px;");
    modeComboBox->setEnabled(true);
    connect(modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SingleMoverControl::onModeChanged);
    formLayout->addRow("3. 运行模式:", modeComboBox);

    // 4. 模式参数切换容器
    paramStackedWidget = new QStackedWidget();

    // 4.1 自动模式参数（Speed + OK）
    QWidget *autoWidget = new QWidget();
    QVBoxLayout *autoLayout = new QVBoxLayout(autoWidget);
    autoLayout->setSpacing(15);

    //Speed
    QHBoxLayout *autoSpeedLayout = new QHBoxLayout();

    autoSpeedSpin = new QSpinBox();// 将QLabel替换为QSpinBox（可输入控件）
    autoSpeedSpin->setRange(10000, 100000); // 与滑块范围保持一致
    autoSpeedSpin->setSingleStep(10000);    // 与滑块步长保持一致
    autoSpeedSpin->setSuffix(" mm/s");       // 显示单位
    autoSpeedSpin->setValue(80000);         // 初始值与滑块同步
    autoSpeedSpin->setMinimumWidth(200);
    connect(autoSpeedSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SingleMoverControl::onAutoSpeedChanged);
    autoSpeedLayout->addWidget(new QLabel("自动速度:"));
    QSpacerItem *labelSpacer = new QSpacerItem(20, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    autoSpeedLayout->addItem(labelSpacer);
    autoSpeedLayout->addWidget(autoSpeedSpin);
    QSpacerItem *autoSpeedrightSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum); //右侧空白占位符（拉伸因子1）
    autoSpeedLayout->addItem(autoSpeedrightSpacer);

    //自动模式下开始运行+停止

    QHBoxLayout *autoRunBtnLayout = new QHBoxLayout();


    QPushButton *SingleAutoRunBtn = new QPushButton("运行");
    SingleAutoRunBtn->setStyleSheet("background-color: green; font-size: 15px; width:200px;min-width: 200px; max-width: 200px;");
    connect(SingleAutoRunBtn, &QPushButton::clicked, this, [this]() {
        this->onSingleAutoRunClicked(true);
    });
    QPushButton *SingleAutoStopBtn = new QPushButton("停止");
    SingleAutoStopBtn->setStyleSheet("background-color: green; font-size: 15px; width:200px;min-width: 200px; max-width: 200px;");
    connect(SingleAutoStopBtn, &QPushButton::clicked, this, [this]() {
        this->onSingleAutoRunClicked(false);
    });
    // 右侧空白占位符（拉伸因子1）
    QSpacerItem *rightSpacer1 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    autoRunBtnLayout->addWidget(SingleAutoRunBtn);
    autoRunBtnLayout->addWidget(SingleAutoStopBtn);
    autoRunBtnLayout->addItem(rightSpacer1);

    autoLayout->addLayout(autoSpeedLayout);
    autoLayout->addLayout(autoRunBtnLayout);


    // 4.2 手动模式参数（JogSpeed + JogPosition + < + >）
    QWidget *manualWidget = new QWidget();
    QVBoxLayout *manualLayout = new QVBoxLayout(manualWidget);
    manualLayout->setSpacing(15);

    // JogSpeed
    QHBoxLayout *jogSpeedLayout = new QHBoxLayout();

    jogSpeedSpin = new QSpinBox();
    jogSpeedSpin->setRange(10000, 100000); // 与滑块范围保持一致
    jogSpeedSpin->setSingleStep(10000);    // 与滑块步长保持一致
    jogSpeedSpin->setSuffix(" mm/s");       // 显示单位
    jogSpeedSpin->setValue(80000);         // 初始值与滑块同步
    jogSpeedSpin->setMinimumWidth(200);
    connect(jogSpeedSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SingleMoverControl::onJogSpeedChanged);
    jogSpeedLayout->addWidget(new QLabel("Jog速度 JogSpeed:   "));
    jogSpeedLayout->addWidget(jogSpeedSpin);
    jogSpeedLayout->addStretch(1);

    // JogPosition
    QHBoxLayout *jogPosLayout = new QHBoxLayout();
    jogPositionSpin = new QSpinBox();
    jogPositionSpin->setRange(-1000, 1000);
    jogPositionSpin->setSuffix(" mm");
    jogPositionSpin->setValue(1);
    jogPositionSpin->setSingleStep(1);
    jogPositionSpin->setMinimumWidth(200);
    connect(jogPositionSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SingleMoverControl::onJogPositionChanged);
    jogPosLayout->addWidget(new QLabel("Jog位置 JogPosition:"));
    jogPosLayout->addWidget(jogPositionSpin);
    jogPosLayout->addStretch(1);

    //JogButton
    QHBoxLayout *jogBtnLayout = new QHBoxLayout();
    QPushButton *jogLeftBtn = new QPushButton("<");
    jogLeftBtn->setStyleSheet(" font-size: 15px; width:200px;");
    QPushButton *jogRightBtn = new QPushButton(">");
    jogRightBtn->setStyleSheet(" font-size: 15px; width:200px;");

    connect(jogLeftBtn, &QPushButton::pressed, this, &SingleMoverControl::onSingleJogLeftPressed);
    connect(jogLeftBtn, &QPushButton::released, this, &SingleMoverControl::onSingleJogLeftReleased);
    connect(jogRightBtn, &QPushButton::pressed, this, &SingleMoverControl::onSingleJogRightPressed);
    connect(jogRightBtn, &QPushButton::released, this, &SingleMoverControl::onSingleJogRightReleased);



    // 右侧空白占位符（拉伸因子1）
    QSpacerItem *rightSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

    jogBtnLayout->addWidget(jogLeftBtn);
    jogBtnLayout->addWidget(jogRightBtn);
    jogBtnLayout->addItem(rightSpacer);
    //单独设置每个元素的拉伸因子（索引从0开始）
    jogBtnLayout->setStretch(0, 1);  // 第0个元素（jogBtnLeft）拉伸因子=1
    jogBtnLayout->setStretch(1, 1);  // 第1个元素（jogBtnRight）拉伸因子=1
    jogBtnLayout->setStretch(2, 3);  // 第2个元素（rightSpacer）拉伸因子=2


    manualLayout->addLayout(jogSpeedLayout);
    manualLayout->addLayout(jogPosLayout);
    manualLayout->addLayout(jogBtnLayout);
    paramStackedWidget->addWidget(manualWidget); // 索引0：手动模式
    paramStackedWidget->addWidget(autoWidget); // 索引1：自动模式

    formLayout->addRow("4. 模式参数:", paramStackedWidget);
    mainLayout->addWidget(groupBox);

    // 设置主布局
    setLayout(mainLayout);

}

// 模式切换(Jog:0 ; 自动：1)
void SingleMoverControl::onModeChanged(int index)
{
    Q_UNUSED(index)
 //   paramStackedWidget->setCurrentIndex(index);

    // 读取当前控制寄存器状态
    emit readModbusRegisterToMainWindow(0x0000);
//    quint16 readVal = m_modbusDataReadData;
//    quint16 writeVal = 0;

//    const int16_t mask = 1 << 1;  //bit1

//    if (index == 1) {
//        // 设置第二位为1（使用或操作）
//        writeVal =  readVal | mask;
//    } else {
//        // 设置第二位为0（使用与操作和取反）
//        writeVal =  readVal & ~mask;
//    }


//    writeModbusRegister(0x0000, writeVal); //向保持寄存器[0]中写使能状态

//    logMessage(index == 0 ?  "切换到手动模式": "切换到自动模式");
}

// 单动子使能
void SingleMoverControl::onSingleEnableClicked()
{
//    if (modbusClient->state() != QModbusDevice::ConnectedState) {
//        QMessageBox::warning(this, "警告", "请先连接设备");
//        return;
//    }

//    bool isEnabled = (singleEnableButton->text() == "禁用");
//    //quint16 value = readModbusRegister(0x0000);  //bit0
//    readModbusRegister(0x0000);
//    quint16 value = m_modbusDataReadData;

//    // 根据isEnabled状态设置第0位：isEnabled为true则第0位设为0，否则设为1
//    if (isEnabled) {
//        value &= ~1; // 清除第0位（与~1按位与，其他位保持不变）

//    } else {

//        value |= 1;  // 设置第0位（与1按位或，其他位保持不变）
//    }

//    value = 10;
//    writeModbusRegister(0x0000, value); //向保持寄存器[0]中写使能状态
//    //writeModbusRegister(0x0000, isEnabled ? 0 : 1); //向保持寄存器[0]中写使能状态
//    // singleEnableButton->setText(isEnabled ? "使能" : "禁用");
//    // singleEnableButton->setStyleSheet(isEnabled ? "background-color: #f44336; font-size: 14px; min-width: 200px; max-width: 200px;" : "background-color: #4caf50; font-size: 14px; min-width: 200px; max-width: 200px;");
//    logMessage(isEnabled ? "单动子已禁用" : "单动子已使能");

}

//是否允许手动控制
void SingleMoverControl::onAllowManualControlClicked(bool checked)
{
    Q_UNUSED(checked)
//    if (modbusClient->state() != QModbusDevice::ConnectedState) {
//        allowManualCheckBox->setCheckState(Qt::Unchecked);
//        QMessageBox::warning(this, "警告", "请先连接设备");
//        return;
//    }

//    //quint16 readVal = readModbusRegister(0x0000);
//    readModbusRegister(0x0000);
//    quint16 readVal = m_modbusDataReadData;

//    const int16_t mask = 1 << 2;  //bit2
//    if(checked)
//    {
//        writeModbusRegister(0x0000, (readVal | mask)); //向保持寄存器[0]中写使能状态 1
//        modeComboBox->setEnabled(false);
//        paramStackedWidget->setEnabled(false);
//    }
//    else
//    {
//        writeModbusRegister(0x0000, (readVal & ~mask)); //向保持寄存器[0]中写使能状态 0
//        modeComboBox->setEnabled(true);
//        paramStackedWidget->setEnabled(true);
//    }

//    logMessage(checked ? "已允许手动控制" : "已禁止手动控制");
}

// 自动模式下，设定好速度后开始运行
void SingleMoverControl::onSingleAutoRunClicked(bool isRun)
{
    Q_UNUSED(isRun)
//    if (modbusClient->state() != QModbusDevice::ConnectedState) {
//        QMessageBox::warning(this, "警告", "请先连接设备");
//        return;
//    }
//    int speedValue = autoSpeedSpin->value();
//    onAutoSpeedChanged(speedValue);

//    readModbusRegister(0x0000);
//    quint16 readVal = m_modbusDataReadData;

//    const int16_t mask = 1 << 5;  //bit5

//    if(isRun)
//    {
//        writeModbusRegister(0x0000, (readVal | mask)); //向保持寄存器[0]中写使能状态 1
//        logMessage(QString("自动模式开始运行！自动模式速度设置为 %1 mmm/s").arg(speedValue));
//    }
//    else
//    {
//        writeModbusRegister(0x0000, (readVal & ~mask)); //向保持寄存器[0]中写使能状态 0
//        logMessage(QString("自动模式停止"));
//    }

}

//JogLeft + JogRight
void SingleMoverControl::onSingleJogLeftPressed()
{
//    if (modbusClient->state() != QModbusDevice::ConnectedState) {
//        QMessageBox::warning(this, "警告", "请先连接设备");
//        return;
//    }

//    int jogSpeedVal = jogSpeedSpin->value();
//    int jogPositionVal = jogPositionSpin->value();
//    onJogSpeedChanged(jogSpeedVal);
//    onJogPositionChanged(jogPositionVal);

    // 读取当前控制寄存器状态用于JogLeft操作
    emit readModbusRegisterToMainWindow(0x0000);
//    quint16 readVal = m_modbusDataReadData;

//    const int16_t mask = 1 << 3;  //bit3
//    writeModbusRegister(0x0000, (readVal | mask)); //向保持寄存器[0]中写使能状态 1
//    logMessage(QString("JogLeft模式开始运行！Jog Left 模式速度设置为 %1 mmm/s,位置设定为 %2 mm").arg(jogSpeedVal).arg(jogPositionVal));
}
void SingleMoverControl::onSingleJogLeftReleased()
{
//    if (modbusClient->state() != QModbusDevice::ConnectedState) {
//        QMessageBox::warning(this, "警告", "请先连接设备");
//        return;
//    }

//    //quint16 readVal = readModbusRegister(0x0000);
//    readModbusRegister(0x0000);
//    quint16 readVal = m_modbusDataReadData;

//    const int16_t mask = 1 << 3;  //bit3
//    writeModbusRegister(0x0000, (readVal & ~mask)); //向保持寄存器[0]中写使能状态 0
//    logMessage(QString("JogLeft模式松开"));
}
void SingleMoverControl::onSingleJogRightPressed()
{
//    if (modbusClient->state() != QModbusDevice::ConnectedState) {
//        QMessageBox::warning(this, "警告", "请先连接设备");
//        return;
//    }
//    int jogSpeedVal = jogSpeedSpin->value();
//    int jogPositionVal = jogPositionSpin->value();
//    onJogSpeedChanged(jogSpeedVal);
//    onJogPositionChanged(jogPositionVal);

    // 读取当前控制寄存器状态用于JogRight操作
    emit readModbusRegisterToMainWindow(0x0000);
//    quint16 readVal = m_modbusDataReadData;

//    const int16_t mask = 1 << 4;  //bit4
//    writeModbusRegister(0x0000, (readVal | mask)); //向保持寄存器[0]中写使能状态 1
//    logMessage(QString("JogRight模式开始运行！Jog Right 模式速度设置为 %1 mm/s,位置设定为 %2 mm").arg(jogSpeedVal).arg(jogPositionVal));
}
void SingleMoverControl::onSingleJogRightReleased()
{
//    if (modbusClient->state() != QModbusDevice::ConnectedState) {
//        QMessageBox::warning(this, "警告", "请先连接设备");
//        return;
//    }

//    //quint16 readVal = readModbusRegister(0x0000);
//    readModbusRegister(0x0000);
//    quint16 readVal = m_modbusDataReadData;

//    const int16_t mask = 1 << 4;  //bit4
//    writeModbusRegister(0x0000, (readVal & ~mask)); //向保持寄存器[0]中写使能状态 0
//    logMessage(QString("JogRight模式松开"));
}


// 自动模式速度
void SingleMoverControl::onAutoSpeedChanged(int value)
{
    Q_UNUSED(value)
//    //autoSpeedLabel->setText(QString("%1 mm/s").arg(value));
//    if (modbusClient->state() == QModbusDevice::ConnectedState) {

//        //提取高16位和低16位
//        quint16 high16 = static_cast<quint16>((value >> 16) & 0xFFFF);  // 高16位
//        quint16 low16 = static_cast<quint16>(value & 0xFFFF);           // 低16位

//        //低16位写入保持寄存器[1],高16位写入保持寄存器[2]，
//        writeModbusRegister(0x0001, low16);
//        writeModbusRegister(0x0002, high16);

//        logMessage(QString("自动模式速度设置为 %1 mm/s").arg(value));
//    }
}

// 手动模式速度
void SingleMoverControl::onJogSpeedChanged(int value)
{
    Q_UNUSED(value)
//    //jogSpeedLabel->setText(QString("%1 mm/s").arg(value));
//    if (modbusClient->state() == QModbusDevice::ConnectedState) {

//        //提取高16位和低16位
//        quint16 high16 = static_cast<quint16>((value >> 16) & 0xFFFF);  // 高16位
//        quint16 low16 = static_cast<quint16>(value & 0xFFFF);           // 低16位

//        //低16位写入保持寄存器[4],高16位写入保持寄存器[5]，
//        writeModbusRegister(0x0004, low16);
//        writeModbusRegister(0x0005, high16);

//        logMessage(QString("手动模式速度设置为 %1 mm/s").arg(value));
//    }
}

// 手动位置
void SingleMoverControl::onJogPositionChanged(int value)
{
    Q_UNUSED(value)
//    if (modbusClient->state() == QModbusDevice::ConnectedState) {
//        writeModbusRegister(0x0003, value);    //向保持寄存器[3]写入JogPostion
//        logMessage(QString("手动位置设置为 %1 mm").arg(value));
//    }
}

