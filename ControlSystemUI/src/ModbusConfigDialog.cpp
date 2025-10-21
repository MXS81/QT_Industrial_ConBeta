// ModbusConfigDialog.cpp
#include "ModbusConfigDialog.h"
#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QSettings>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QApplication>
#include <stdexcept> // 用于 std::runtime_error

/**
 * @brief 构造函数
 * @param config 初始配置
 * @param parent 父窗口指针
 */
ModbusConfigDialog::ModbusConfigDialog(const ModbusConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_config(config)
    , m_mainWindow(qobject_cast<MainWindow*>(parent))
{
    setWindowTitle("Modbus连接配置");
    setModal(true); // 设置为模态对话框，阻塞父窗口
    setMinimumSize(500, 450);

    // 如果parent不是MainWindow，尝试遍历查找
    if (!m_mainWindow) {
        m_mainWindow = findMainWindow();
    }

    try {
        setupUI();       // 初始化UI
        loadSettings();  // 从注册表加载上一次的配置
        setConfig(m_config); // 将最终配置应用到UI
    } catch (const std::runtime_error& e) {
        // 在UI创建失败时提供明确的错误提示
        QMessageBox::critical(this, "初始化错误", QString("无法创建配置对话框: %1").arg(e.what()));
        // 延迟关闭，避免构造函数中直接关闭导致问题
        QTimer::singleShot(0, this, &QDialog::reject);
        return;
    }

    if (m_mainWindow) {
        logToSystem("Modbus配置对话框已打开", "info");
    }
}

/**
 * @brief 初始化UI界面和布局
 */
void ModbusConfigDialog::setupUI()
{
    // --- 主布局 ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    if (!mainLayout) throw std::runtime_error("无法创建主布局");
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // --- 1. 连接类型选择 ---
    QGroupBox *typeGroup = new QGroupBox("连接类型");
    if (!typeGroup) throw std::runtime_error("无法创建类型组");
    QHBoxLayout *typeLayout = new QHBoxLayout(typeGroup);
    m_tcpRadio = new QRadioButton("TCP/IP");
    m_serialRadio = new QRadioButton("串口");
    if (!m_tcpRadio || !m_serialRadio) throw std::runtime_error("无法创建单选按钮");
    m_tcpRadio->setChecked(true);
    typeLayout->addWidget(m_tcpRadio);
    typeLayout->addWidget(m_serialRadio);
    typeLayout->addStretch();

    // --- 2. TCP/IP 设置 ---
    m_tcpGroup = new QGroupBox("TCP/IP设置");
    if (!m_tcpGroup) throw std::runtime_error("无法创建TCP组");
    QFormLayout *tcpLayout = new QFormLayout(m_tcpGroup);
    m_hostEdit = new QLineEdit();
    if (!m_hostEdit) throw std::runtime_error("无法创建主机编辑框");
    m_hostEdit->setPlaceholderText("例如: 192.168.1.100");
    m_portSpin = new QSpinBox();
    if (!m_portSpin) throw std::runtime_error("无法创建端口输入框");
    m_portSpin->setRange(1, 65535);
    tcpLayout->addRow("PLC IP地址:", m_hostEdit);
    tcpLayout->addRow("端口:", m_portSpin);

    // --- 3. 串口设置 ---
    m_serialGroup = new QGroupBox("串口(RTU)设置");
    if (!m_serialGroup) throw std::runtime_error("无法创建串口组");
    QFormLayout *serialLayout = new QFormLayout(m_serialGroup);
    m_serialPortCombo = new QComboBox();
    m_baudRateCombo = new QComboBox();
    m_dataBitsCombo = new QComboBox();
    m_stopBitsCombo = new QComboBox();
    m_parityCombo = new QComboBox();
    if (!m_serialPortCombo || !m_baudRateCombo || !m_dataBitsCombo || !m_stopBitsCombo || !m_parityCombo) {
        throw std::runtime_error("无法创建串口下拉框");
    }
    // 填充可用串口
    for (const QSerialPortInfo &port : QSerialPortInfo::availablePorts()) {
        m_serialPortCombo->addItem(port.portName());
    }
    m_baudRateCombo->addItems({"9600", "19200", "38400", "57600", "115200"});
    m_dataBitsCombo->addItems({"8", "7"});
    m_stopBitsCombo->addItems({"1", "2"});
    m_parityCombo->addItems({"无 (None)", "奇校验 (Odd)", "偶校验 (Even)"});
    serialLayout->addRow("串口:", m_serialPortCombo);
    serialLayout->addRow("波特率:", m_baudRateCombo);
    serialLayout->addRow("数据位:", m_dataBitsCombo);
    serialLayout->addRow("停止位:", m_stopBitsCombo);
    serialLayout->addRow("校验位:", m_parityCombo);

    // --- 4. 通用设置 ---
    QGroupBox *commonGroup = new QGroupBox("通用设置");
    if (!commonGroup) throw std::runtime_error("无法创建通用组");
    QFormLayout *commonLayout = new QFormLayout(commonGroup);
    m_deviceIdSpin = new QSpinBox();
    if (!m_deviceIdSpin) throw std::runtime_error("无法创建设备ID输入框");
    m_deviceIdSpin->setRange(1, 247); // Modbus RTU/TCP的有效范围
    m_timeoutSpin = new QSpinBox();
    if (!m_timeoutSpin) throw std::runtime_error("无法创建超时输入框");
    m_timeoutSpin->setRange(100, 30000);
    m_timeoutSpin->setSuffix(" ms");
    m_retriesSpin = new QSpinBox();
    if (!m_retriesSpin) throw std::runtime_error("无法创建重试输入框");
    m_retriesSpin->setRange(0, 5);
    commonLayout->addRow("设备ID (从站地址):", m_deviceIdSpin);
    commonLayout->addRow("响应超时:", m_timeoutSpin);
    commonLayout->addRow("失败重试次数:", m_retriesSpin);

    // --- 5. 操作按钮 ---
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    if (!buttonLayout) throw std::runtime_error("无法创建按钮布局");
    m_loadBtn = new QPushButton("加载配置");
    m_saveBtn = new QPushButton("保存配置");
    m_okBtn = new QPushButton("确定");
    m_cancelBtn = new QPushButton("取消");
    m_okBtn->setDefault(true); // 将“确定”设为默认按钮
    buttonLayout->addWidget(m_loadBtn);
    buttonLayout->addWidget(m_saveBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okBtn);
    buttonLayout->addWidget(m_cancelBtn);

    // --- 组装布局 ---
    mainLayout->addWidget(typeGroup);
    mainLayout->addWidget(m_tcpGroup);
    mainLayout->addWidget(m_serialGroup);
    mainLayout->addWidget(commonGroup);
    mainLayout->addLayout(buttonLayout);

    // --- 连接信号与槽 ---
    connect(m_tcpRadio, &QRadioButton::toggled, this, &ModbusConfigDialog::onConnectionTypeChanged);
    connect(m_loadBtn, &QPushButton::clicked, this, &ModbusConfigDialog::onLoadProfile);
    connect(m_saveBtn, &QPushButton::clicked, this, &ModbusConfigDialog::onSaveProfile);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    // 初始化UI可见性
    onConnectionTypeChanged();
}

/**
 * @brief 获取当前UI上的配置信息
 * @return ModbusConfig 结构体
 */
ModbusConfig ModbusConfigDialog::getConfig() const
{
    ModbusConfig config;
    config.type = m_tcpRadio->isChecked() ? ModbusConfig::TCP : ModbusConfig::Serial;
    config.host = m_hostEdit->text().trimmed();
    config.port = m_portSpin->value();
    config.serialPort = m_serialPortCombo->currentText();
    config.baudRate = m_baudRateCombo->currentText().toInt();
    config.dataBits = m_dataBitsCombo->currentText().toInt();
    config.stopBits = m_stopBitsCombo->currentText().toInt() == 1 ? 1 : 2; // 简化处理
    config.parity = m_parityCombo->currentIndex(); // 0:None, 1:Odd, 2:Even
    config.deviceId = m_deviceIdSpin->value();
    config.timeout = m_timeoutSpin->value();
    config.retries = m_retriesSpin->value();
    return config;
}

/**
 * @brief 将配置信息设置到UI控件上显示
 * @param config 要显示的配置
 */
void ModbusConfigDialog::setConfig(const ModbusConfig &config)
{
    m_config = config;
    m_tcpRadio->setChecked(config.type == ModbusConfig::TCP);
    m_serialRadio->setChecked(config.type == ModbusConfig::Serial);
    m_hostEdit->setText(config.host);
    m_portSpin->setValue(config.port);
    m_serialPortCombo->setCurrentText(config.serialPort);
    m_baudRateCombo->setCurrentText(QString::number(config.baudRate));
    m_dataBitsCombo->setCurrentText(QString::number(config.dataBits));
    m_stopBitsCombo->setCurrentText(QString::number(config.stopBits));
    m_parityCombo->setCurrentIndex(config.parity);
    m_deviceIdSpin->setValue(config.deviceId);
    m_timeoutSpin->setValue(config.timeout);
    m_retriesSpin->setValue(config.retries);
    onConnectionTypeChanged();
}

/**
 * @brief 从QSettings (通常是注册表) 中加载上一次保存的配置
 */
void ModbusConfigDialog::loadSettings()
{
    QSettings settings;
    settings.beginGroup("ModbusConfig");
    ModbusConfig config; // 使用默认值作为备用
    config.type = static_cast<ModbusConfig::ConnectionType>(settings.value("type", static_cast<int>(config.type)).toInt());
    config.host = settings.value("host", config.host).toString();
    config.port = settings.value("port", config.port).toInt();
    config.serialPort = settings.value("serialPort", config.serialPort).toString();
    config.baudRate = settings.value("baudRate", config.baudRate).toInt();
    config.deviceId = settings.value("deviceId", config.deviceId).toInt();
    settings.endGroup();
    setConfig(config); // 将加载的配置应用到UI
}

/**
 * @brief 将当前UI上的配置保存到QSettings
 */
void ModbusConfigDialog::saveSettings()
{
    QSettings settings;
    settings.beginGroup("ModbusConfig");
    ModbusConfig config = getConfig(); // 从UI获取当前配置
    settings.setValue("type", static_cast<int>(config.type));
    settings.setValue("host", config.host);
    settings.setValue("port", config.port);
    settings.setValue("serialPort", config.serialPort);
    settings.setValue("baudRate", config.baudRate);
    settings.setValue("deviceId", config.deviceId);
    settings.endGroup();
}

/**
 * @brief 当连接类型（TCP/串口）改变时，切换显示对应的配置组
 */
void ModbusConfigDialog::onConnectionTypeChanged()
{
    bool isTcp = m_tcpRadio->isChecked();
    m_tcpGroup->setVisible(isTcp);
    m_serialGroup->setVisible(!isTcp);
    // 动态调整对话框大小以适应内容变化
    adjustSize();
    logToSystem(QString("连接类型切换到: %1").arg(isTcp ? "TCP/IP" : "串口"), "info");
}

/**
 * @brief 记录日志到控制台和主窗口
 */
void ModbusConfigDialog::logToSystem(const QString &message, const QString &type)
{
    QString logMsg = QString("[MODBUS_CONFIG] %1: %2").arg(type.toUpper(), message);
    qDebug().noquote() << logMsg;
    // 如果找到了主窗口，就通过信号将日志发送过去
    if (m_mainWindow) {
        emit logMessage(QString("[Modbus配置] %1").arg(message), type);
    }
}

/**
 * @brief 查找主窗口实例
 * @return MainWindow指针，如果找不到则为nullptr
 */
MainWindow* ModbusConfigDialog::findMainWindow()
{
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        if (MainWindow *mainWin = qobject_cast<MainWindow*>(widget)) {
            return mainWin;
        }
    }
    qWarning() << "未能找到MainWindow实例";
    return nullptr;
}


void ModbusConfigDialog::onLoadProfile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "加载Modbus配置", "", "JSON配置文件 (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开配置文件！");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        ModbusConfig config;
        config.type = obj["type"].toInt(0) == 0 ? ModbusConfig::TCP : ModbusConfig::Serial;
        config.host = obj["host"].toString();
        config.port = obj["port"].toInt();
        config.serialPort = obj["serialPort"].toString();
        config.baudRate = obj["baudRate"].toInt();
        config.deviceId = obj["deviceId"].toInt();
        setConfig(config);
        QMessageBox::information(this, "成功", "配置文件加载成功！");
    } else {
        QMessageBox::warning(this, "错误", "配置文件格式无效！");
    }
}

void ModbusConfigDialog::onSaveProfile()
{
    QString fileName = QFileDialog::getSaveFileName(this, "保存Modbus配置", "modbus_config.json", "JSON配置文件 (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "错误", "无法保存配置文件！");
        return;
    }

    ModbusConfig config = getConfig();
    QJsonObject obj;
    obj["type"] = static_cast<int>(config.type);
    obj["host"] = config.host;
    obj["port"] = config.port;
    obj["serialPort"] = config.serialPort;
    obj["baudRate"] = config.baudRate;
    obj["deviceId"] = config.deviceId;

    file.write(QJsonDocument(obj).toJson());
    QMessageBox::information(this, "成功", "配置文件保存成功！");
}
