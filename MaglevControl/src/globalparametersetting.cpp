#include "globalparametersetting.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QApplication>
#include <QDir>

// 1. 定义16位整数类型（使用Qt的quint16，无符号16位，范围0~65535，符合业务无负值需求）
typedef quint16 ParamUInt16;

GlobalParameterSetting::GlobalParameterSetting(QWidget *parent)
    : QWidget(parent)
{
    // 2. 验证器改为16位整数范围（0 ~ 65535）
    uint16Validator = new QIntValidator(0, 65535, this); // 16位无符号整数最大值为65535

    setupUI();
    loadParameters();  // 启动时加载参数
    setWindowTitle("全局参数设定");
    setMinimumSize(800, 600);
}

GlobalParameterSetting::~GlobalParameterSetting()
{
    delete uint16Validator; // 释放16位整数验证器
}

void GlobalParameterSetting::setupUI()
{
    // 主布局
    auto *mainLayout = new QHBoxLayout(this);

    // 创建左右面板
    auto *leftWidget = new QWidget(this);
    auto *rightWidget = new QWidget(this);

    createLeftPanel(leftWidget);
    createRightPanel(rightWidget);

    // 设置左右面板比例
    mainLayout->addWidget(leftWidget, 1);
    mainLayout->addWidget(rightWidget, 1);

    // 添加保存按钮到底部
    auto *bottomLayout = new QVBoxLayout();
    saveButton = new QPushButton("保存参数", this);
    saveButton->setMinimumHeight(40);
    saveButton->setStyleSheet("QPushButton { font-size: 14px; font-weight: bold; }");
    connect(saveButton, &QPushButton::clicked, this, &GlobalParameterSetting::saveParameters);

    // 使用网格布局来将按钮放置在右下角
    auto *buttonContainer = new QWidget(this);
    auto *buttonLayout = new QGridLayout(buttonContainer);
    buttonLayout->addWidget(saveButton, 0, 0, Qt::AlignRight | Qt::AlignBottom);
    buttonLayout->setRowStretch(0, 1);
    buttonLayout->setColumnStretch(0, 1);

    bottomLayout->addWidget(buttonContainer);
    mainLayout->addLayout(bottomLayout, 0);

    setLayout(mainLayout);
}

void GlobalParameterSetting::createLeftPanel(QWidget *parent)
{
    auto *layout = new QVBoxLayout(parent);

    // -------------------------- 工作流参数组 --------------------------
    auto *workflowGroup = new QGroupBox("动子参数", parent);
    auto *workflowLayout = new QFormLayout();
    workflowLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    workflowLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    workflowLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 工作流方向
    workflowDirection = new QComboBox(parent);
    workflowDirection->addItems({"正方向", "逆方向"});
    workflowLayout->addRow("工作流方向:", workflowDirection);

    // 动子总数量（原int→16位int，验证器改为uint16Validator）
    motorTotalCount = new QLineEdit(parent);
    motorTotalCount->setValidator(uint16Validator);
    motorTotalCount->setText("10"); // 默认值：移除小数，改为16位范围内整数
    workflowLayout->addRow("动子总数量:", motorTotalCount);

    // 动子安全距离（原double→16位int，单位保留mm，值改为整数）
    motorSafeDistance = new QLineEdit(parent);
    motorSafeDistance->setValidator(uint16Validator);
    motorSafeDistance->setText("5"); // 原5.0→5（无小数）
    workflowLayout->addRow("动子安全距离 (mm):", motorSafeDistance);

    workflowGroup->setLayout(workflowLayout);
    layout->addWidget(workflowGroup);

    // -------------------------- 位置参数组 --------------------------
    auto *positionGroup = new QGroupBox("位置参数", parent);
    auto *positionLayout = new QFormLayout();
    positionLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    positionLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    positionLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 摆渡1动子停留位置
    ferry1MotorPos = new QLineEdit(parent);
    ferry1MotorPos->setValidator(uint16Validator);
    ferry1MotorPos->setText("100"); // 原100.0→100
    positionLayout->addRow("摆渡1动子停留位置 (mm):", ferry1MotorPos);

    // 摆渡2动子停留位置
    ferry2MotorPos = new QLineEdit(parent);
    ferry2MotorPos->setValidator(uint16Validator);
    ferry2MotorPos->setText("200"); // 原200.0→200
    positionLayout->addRow("摆渡2动子停留位置 (mm):", ferry2MotorPos);

    // 摆渡1入口暂存位置
    ferry1EntryPos = new QLineEdit(parent);
    ferry1EntryPos->setValidator(uint16Validator);
    ferry1EntryPos->setText("150"); // 原150.0→150
    positionLayout->addRow("摆渡1入口暂存位置 (mm):", ferry1EntryPos);

    // 摆渡2入口暂存位置
    ferry2EntryPos = new QLineEdit(parent);
    ferry2EntryPos->setValidator(uint16Validator);
    ferry2EntryPos->setText("250"); // 原250.0→250
    positionLayout->addRow("摆渡2入口暂存位置 (mm):", ferry2EntryPos);

    // 磁浮进入皮带位置
    maglevToBeltPos = new QLineEdit(parent);
    maglevToBeltPos->setValidator(uint16Validator);
    maglevToBeltPos->setText("300"); // 原300.0→300
    positionLayout->addRow("磁浮进入皮带位置 (mm):", maglevToBeltPos);

    // 皮带进入磁浮位置
    beltToMaglevPos = new QLineEdit(parent);
    beltToMaglevPos->setValidator(uint16Validator);
    beltToMaglevPos->setText("350"); // 原350.0→350
    positionLayout->addRow("皮带进入磁浮位置 (mm):", beltToMaglevPos);

    // RFID位置
    rfidPos = new QLineEdit(parent);
    rfidPos->setValidator(uint16Validator);
    rfidPos->setText("400"); // 原400.0→400
    positionLayout->addRow("RFID位置 (mm):", rfidPos);

    positionGroup->setLayout(positionLayout);
    layout->addWidget(positionGroup);

    // 添加强制伸展的空间
    layout->addStretch(1);
    parent->setLayout(layout);
}

void GlobalParameterSetting::createRightPanel(QWidget *parent)
{
    auto *layout = new QVBoxLayout(parent);

    // -------------------------- 动子参数组 --------------------------
    auto *motorParamGroup = new QGroupBox("动子参数", parent);
    auto *motorParamLayout = new QFormLayout();
    motorParamLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    motorParamLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    motorParamLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Jog速度
    jogSpeed = new QLineEdit(parent);
    jogSpeed->setValidator(uint16Validator);
    jogSpeed->setText("10"); // 原10.0→10
    motorParamLayout->addRow("Jog速度 (mm/s):", jogSpeed);

    // 手动速度
    manualSpeed = new QLineEdit(parent);
    manualSpeed->setValidator(uint16Validator);
    manualSpeed->setText("20"); // 原20.0→20
    motorParamLayout->addRow("手动速度 (mm/s):", manualSpeed);

    // 自动速度
    autoSpeed = new QLineEdit(parent);
    autoSpeed->setValidator(uint16Validator);
    autoSpeed->setText("50"); // 原50.0→50
    motorParamLayout->addRow("自动速度 (mm/s):", autoSpeed);

    // 加速度
    acceleration = new QLineEdit(parent);
    acceleration->setValidator(uint16Validator);
    acceleration->setText("100"); // 原100.0→100
    motorParamLayout->addRow("加速度 (mm/s²):", acceleration);

    // 减速度
    deceleration = new QLineEdit(parent);
    deceleration->setValidator(uint16Validator);
    deceleration->setText("100"); // 原100.0→100
    motorParamLayout->addRow("减速度 (mm/s²):", deceleration);

    motorParamGroup->setLayout(motorParamLayout);
    layout->addWidget(motorParamGroup);

    // 添加强制伸展的空间
    layout->addStretch(1);
    parent->setLayout(layout);
}

void GlobalParameterSetting::loadParameters()
{
    // 获取配置文件路径
    QString appDir = QCoreApplication::applicationDirPath();
    QString filePath = appDir + "/parameters.ini";

    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        return; // 文件不存在，使用UI默认值
    }

    // 读取配置文件
    QSettings settings(filePath, QSettings::IniFormat);

    // 读取工作流参数
    settings.beginGroup("Workflow");
    QString direction = settings.value("Direction", "正方向").toString();
    // 原int→16位int，用toUInt()转换（确保无符号），默认值10
    ParamUInt16 totalMotors = static_cast<ParamUInt16>(settings.value("TotalMotors", 10).toUInt());
    // 原double→16位int，先转double再强转（兼容旧配置文件的浮点值），默认值5
    ParamUInt16 safeDistance = static_cast<ParamUInt16>(settings.value("SafeDistance", 5).toDouble());
    settings.endGroup();

    // 读取位置参数
    settings.beginGroup("Positions");
    // 所有位置参数：原double→16位int，兼容旧配置文件的浮点值，默认值移除小数
    ParamUInt16 ferry1MotorPosVal = static_cast<ParamUInt16>(settings.value("Ferry1MotorPos", 100).toDouble());
    ParamUInt16 ferry2MotorPosVal = static_cast<ParamUInt16>(settings.value("Ferry2MotorPos", 200).toDouble());
    ParamUInt16 ferry1EntryPosVal = static_cast<ParamUInt16>(settings.value("Ferry1EntryPos", 150).toDouble());
    ParamUInt16 ferry2EntryPosVal = static_cast<ParamUInt16>(settings.value("Ferry2EntryPos", 250).toDouble());
    ParamUInt16 maglevToBeltPosVal = static_cast<ParamUInt16>(settings.value("MaglevToBeltPos", 300).toDouble());
    ParamUInt16 beltToMaglevPosVal = static_cast<ParamUInt16>(settings.value("BeltToMaglevPos", 350).toDouble());
    ParamUInt16 rfidPosVal = static_cast<ParamUInt16>(settings.value("RFIDPos", 400).toDouble());
    settings.endGroup();

    // 读取动子参数
    settings.beginGroup("MotorParameters");
    // 所有速度/加速度参数：原double→16位int，兼容旧配置文件的浮点值，默认值移除小数
    ParamUInt16 jogSpeedVal = static_cast<ParamUInt16>(settings.value("JogSpeed", 10).toDouble());
    ParamUInt16 manualSpeedVal = static_cast<ParamUInt16>(settings.value("ManualSpeed", 20).toDouble());
    ParamUInt16 autoSpeedVal = static_cast<ParamUInt16>(settings.value("AutoSpeed", 50).toDouble());
    ParamUInt16 accelerationVal = static_cast<ParamUInt16>(settings.value("Acceleration", 100).toDouble());
    ParamUInt16 decelerationVal = static_cast<ParamUInt16>(settings.value("Deceleration", 100).toDouble());
    settings.endGroup();

    // 设置UI控件值
    // 所有数值转字符串时直接用整数格式（无小数点）
    workflowDirection->setCurrentText(direction);
    motorTotalCount->setText(QString::number(totalMotors)); // 整数转字符串，无小数
    motorSafeDistance->setText(QString::number(safeDistance));

    ferry1MotorPos->setText(QString::number(ferry1MotorPosVal));
    ferry2MotorPos->setText(QString::number(ferry2MotorPosVal));
    ferry1EntryPos->setText(QString::number(ferry1EntryPosVal));
    ferry2EntryPos->setText(QString::number(ferry2EntryPosVal));
    maglevToBeltPos->setText(QString::number(maglevToBeltPosVal));
    beltToMaglevPos->setText(QString::number(beltToMaglevPosVal));
    rfidPos->setText(QString::number(rfidPosVal));

    jogSpeed->setText(QString::number(jogSpeedVal));
    manualSpeed->setText(QString::number(manualSpeedVal));
    autoSpeed->setText(QString::number(autoSpeedVal));
    acceleration->setText(QString::number(accelerationVal));
    deceleration->setText(QString::number(decelerationVal));
}

void GlobalParameterSetting::saveParameters()
{
    // 验证所有输入
    bool hasInvalidInput = false;
    int pos = 0;

    // -------------------------- 验证16位整数输入 --------------------------
    // 所有数值输入控件统一用uint16Validator验证（无整数/浮点数区分）
    QList<QLineEdit*> allUInt16Edits = {
        motorTotalCount, motorSafeDistance, ferry1MotorPos, ferry2MotorPos,
        ferry1EntryPos, ferry2EntryPos, maglevToBeltPos, beltToMaglevPos,
        rfidPos, jogSpeed, manualSpeed, autoSpeed, acceleration, deceleration
    };

    for (QLineEdit* edit : allUInt16Edits) {
        QString text = edit->text();
        pos = 0;
        if (uint16Validator->validate(text, pos) != QValidator::Acceptable) {
            edit->setStyleSheet("border: 1px solid red;");
            hasInvalidInput = true;
        } else {
            edit->setStyleSheet("");
        }
    }

    if (hasInvalidInput) {
        QMessageBox::warning(this, "输入错误", "请检查并修正红色边框的输入项（需为0~65535的整数）");
        return;
    }

    // -------------------------- 保存参数到配置文件 --------------------------
    QString appDir = QCoreApplication::applicationDirPath();
    QString filePath = appDir + "/parameters.ini";
    QSettings settings(filePath, QSettings::IniFormat);

    // 工作流参数：所有数值转16位整数
    settings.beginGroup("Workflow");
    settings.setValue("Direction", workflowDirection->currentText());
    settings.setValue("TotalMotors", static_cast<ParamUInt16>(motorTotalCount->text().toUInt()));
    settings.setValue("SafeDistance", static_cast<ParamUInt16>(motorSafeDistance->text().toUInt()));
    settings.endGroup();

    // 位置参数：所有数值转16位整数
    settings.beginGroup("Positions");
    settings.setValue("Ferry1MotorPos", static_cast<ParamUInt16>(ferry1MotorPos->text().toUInt()));
    settings.setValue("Ferry2MotorPos", static_cast<ParamUInt16>(ferry2MotorPos->text().toUInt()));
    settings.setValue("Ferry1EntryPos", static_cast<ParamUInt16>(ferry1EntryPos->text().toUInt()));
    settings.setValue("Ferry2EntryPos", static_cast<ParamUInt16>(ferry2EntryPos->text().toUInt()));
    settings.setValue("MaglevToBeltPos", static_cast<ParamUInt16>(maglevToBeltPos->text().toUInt()));
    settings.setValue("BeltToMaglevPos", static_cast<ParamUInt16>(beltToMaglevPos->text().toUInt()));
    settings.setValue("RFIDPos", static_cast<ParamUInt16>(rfidPos->text().toUInt()));
    settings.endGroup();

    // 保存动子参数
    settings.beginGroup("MotorParameters");
    settings.setValue("JogSpeed", jogSpeed->text().toDouble());
    settings.setValue("ManualSpeed", manualSpeed->text().toDouble());
    settings.setValue("AutoSpeed", autoSpeed->text().toDouble());
    settings.setValue("Acceleration", acceleration->text().toDouble());
    settings.setValue("Deceleration", deceleration->text().toDouble());
    settings.endGroup();

    QMessageBox::information(this, "保存成功", "参数已成功保存到配置文件");


    // 发射信号到主页面，传递消息
    emit sendMessageToMainWindow("全局参数设定数据保存成功");


}
