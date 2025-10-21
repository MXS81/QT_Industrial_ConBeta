#include "ModbusManager.h"
#include "MainWindow.h"
#include "ModbusConfigDialog.h"
#include <QModbusTcpClient>
#include <QModbusRtuSerialClient>
#include <QEventLoop>
#include <QThread>
#include <QSerialPort>
#include <QDebug>

// --- 构造函数与析构函数 ---

/**
 * @brief ModbusManager类的构造函数
 * @param parent 父对象指针
 */
ModbusManager::ModbusManager(QObject *parent)
    : QObject(parent)
    , m_modbusClient(nullptr)
    , m_cyclicTimer(new QTimer(this))
    , m_mainWindow(nullptr)
    , m_port(502)
    , m_deviceId(1)
    , m_isConnected(false)
    , m_successfulOperations(0)
    , m_failedOperations(0)
{
    // 设置周期性读取定时器为非单次触发
    m_cyclicTimer->setSingleShot(false);
}

/**
 * @brief ModbusManager类的析构函数
 */
ModbusManager::~ModbusManager()
{
    cleanup(); // 清理资源
}



// --- 日志与信息获取 ---
/**
 * @brief 记录一次Modbus操作的日志
 * @param operation 操作名称
 * @param success 操作是否成功
 * @param details 额外信息
 */
void ModbusManager::logOperation(const QString &operation, bool success, const QString &details)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    // 更新成功或失败的操作计数
    if (success) {
        m_successfulOperations++;
    } else {
        m_failedOperations++;
    }

    // 格式化控制台日志消息
    QString logMessage = QString("[%1] [MODBUS] %2: %3").arg(timestamp, success ? "SUCCESS" : "FAILED", operation);
    if (!details.isEmpty()) {
        logMessage += QString(" (%1)").arg(details);
    }

    // 根据成功与否选择不同的日志级别输出
    if (success) {
        qDebug() << logMessage;
    } else {
        qWarning() << logMessage;
    }

    // 如果主窗口存在，则将日志添加到系统日志中
    if (m_mainWindow) {
        QString systemLogMessage = QString("[Modbus] %1").arg(operation);
        if (!details.isEmpty()) {
            systemLogMessage += QString(" - %1").arg(details);
        }

        QString logType = success ? "success" : "error";
        m_mainWindow->addLogEntry(systemLogMessage, logType);
    }
}

/**
 * @brief 获取当前连接的信息字符串
 * @return 连接信息的字符串描述
 */
QString ModbusManager::getConnectionInfo() const
{
    if (!m_isConnected) {
        return "未连接";
    }

    QString info;
    if (m_modbusClient) {
        if (qobject_cast<QModbusTcpClient*>(m_modbusClient)) {
            // TCP连接信息
            info = QString("TCP: %1:%2").arg(m_host).arg(m_port);
        }
        info += QString(" | 设备ID: %1").arg(m_deviceId);
    }

    return info;
}

// --- 连接管理 ---

/**
 * @brief 通过TCP连接到Modbus设备
 * @param host 主机地址
 * @param port 端口号
 * @param deviceId 设备ID
 * @return 是否成功发起连接
 */
bool ModbusManager::connectToDevice(const QString &host, int port, int deviceId)
{
    logOperation(QString("TCP连接请求"), true, QString("目标: %1:%2, 设备ID: %3").arg(host).arg(port).arg(deviceId));
    if (m_isConnected) {
        disconnectFromDevice();
    }
    m_host = host;
    m_port = port;
    m_deviceId = deviceId;

    try {
        m_modbusClient = new QModbusTcpClient(this);
        setupModbusClient();
        m_modbusClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
        m_modbusClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, host);
        m_modbusClient->setTimeout(1000);
        m_modbusClient->setNumberOfRetries(1);

        if (!m_modbusClient->connectDevice()) {
            logOperation("TCP连接失败", false, m_modbusClient->errorString());
            cleanup();
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        logOperation("TCP连接异常", false, e.what());
        cleanup();
        return false;
    }
}

/**
 * @brief 通过串口连接到Modbus设备
 * @param portName 串口名称
 * @param baudRate 波特率
 * @param deviceId 设备ID
 * @return 是否成功发起连接
 */
bool ModbusManager::connectToSerialDevice(const QString &portName, int baudRate, int deviceId)
{
    logOperation("串口连接请求", true, QString("端口: %1, 波特率: %2, 设备ID: %3").arg(portName).arg(baudRate).arg(deviceId));

    // 如果已存在连接，先断开
    if (m_isConnected) {
        logOperation("断开现有连接", true, "为新连接做准备");
        disconnectFromDevice();
    }

    // 保存连接参数
    m_deviceId = deviceId;

    try {
        // 创建并设置串口Modbus客户端
        m_modbusClient = new QModbusRtuSerialClient(this);
        setupModbusClient();

        // 设置串口参数
        m_modbusClient->setConnectionParameter(QModbusDevice::SerialPortNameParameter, portName);
        m_modbusClient->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baudRate);
        m_modbusClient->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
        m_modbusClient->setConnectionParameter(QModbusDevice::SerialParityParameter, QSerialPort::NoParity);
        m_modbusClient->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);

        // 设置超时和重试次数
        m_modbusClient->setTimeout(1000);
        m_modbusClient->setNumberOfRetries(1);

        logOperation("串口客户端配置", true, QString("数据位: 8, 校验: 无, 停止位: 1, 超时: 1000ms"));

        // 发起连接
        if (!m_modbusClient->connectDevice()) {
            QString error = m_modbusClient->errorString();
            logOperation("串口连接失败", false, error);
            emit connectionError(error);
            cleanup();
            return false;
        }

        logOperation("串口连接启动", true, QString("正在连接到 %1").arg(portName));
        return true;

    } catch (const std::exception& e) {
        QString error = QString("串口连接异常: %1").arg(e.what());
        logOperation("串口连接异常", false, error);
        emit connectionError(error);
        cleanup();
        return false;
    }
}

/**
 * @brief 从设备断开连接
 */
void ModbusManager::disconnectFromDevice()
{
    stopCyclicRead();
    if (m_modbusClient && m_modbusClient->state() == QModbusDevice::ConnectedState) {
        m_modbusClient->disconnectDevice();
    }
    cleanup();
}

/**
 * @brief 检查当前是否已连接
 * @return 如果已连接则返回true
 */
bool ModbusManager::isConnected() const
{
    return m_isConnected;
}

// --- 单动子高级控制接口实现 ---

/**
 * @brief 设置单动子使能状态
 * @param enable true为使能，false为禁用
 * @return 是否成功发送命令
 */
bool ModbusManager::setSingleAxisEnable(bool enable)
{
    logOperation(QString("设置单动子使能: %1").arg(enable ? "使能" : "禁用"), true);
    return setControlWordBit(ModbusRegisters::SingleAxis::ENABLE_BIT, enable);
}

/**
 * @brief 设置单动子运行模式
 * @param isAutoMode true为自动模式，false为点动/手动模式
 * @return 是否成功发送命令
 */
bool ModbusManager::setSingleAxisRunMode(bool isAutoMode)
{
    logOperation(QString("设置运行模式: %1").arg(isAutoMode ? "自动" : "手动"), true);
    return setControlWordBit(ModbusRegisters::SingleAxis::RUN_MODE_BIT, isAutoMode);
}

/**
 * @brief 设置是否允许手动控制
 * @param allow true为允许，false为禁止
 * @return 是否成功发送命令
 */
bool ModbusManager::setSingleAxisManualControl(bool allow)
{
    logOperation(QString("设置手动权限: %1").arg(allow ? "允许" : "禁止"), true);
    return setControlWordBit(ModbusRegisters::SingleAxis::MANUAL_ALLOW_BIT, allow);
}

/**
 * @brief 控制单动子JOG运动
 * @param direction 0=停止, 1=向左, 2=向右
 * @return 是否成功发送命令
 */
bool ModbusManager::setSingleAxisJog(int direction)
{
    logOperation(QString("发送JOG命令: 方向=%1").arg(direction), true);
    bool success = true;
    // JOG命令是脉冲式的，按下为1，松开为0，所以需要先清零另一个方向
    if (direction == 1) { //向左
        success &= setControlWordBit(ModbusRegisters::SingleAxis::JOG_RIGHT_BIT, false);
        success &= setControlWordBit(ModbusRegisters::SingleAxis::JOG_LEFT_BIT, true);
    } else if (direction == 2) { //向右
        success &= setControlWordBit(ModbusRegisters::SingleAxis::JOG_LEFT_BIT, false);
        success &= setControlWordBit(ModbusRegisters::SingleAxis::JOG_RIGHT_BIT, true);
    } else { //停止
        success &= setControlWordBit(ModbusRegisters::SingleAxis::JOG_LEFT_BIT, false);
        success &= setControlWordBit(ModbusRegisters::SingleAxis::JOG_RIGHT_BIT, false);
    }
    return success;
}

/**
 * @brief 控制单动子自动运行
 * @param run true为启动，false为停止
 * @return 是否成功发送命令
 */
bool ModbusManager::setSingleAxisAutoRun(bool run)
{
    logOperation(QString("设置自动运行: %1").arg(run ? "启动" : "停止"), true);
    return setControlWordBit(ModbusRegisters::SingleAxis::AUTO_RUN_BIT, run);
}

/**
 * @brief 设置单动子自动模式的速度 (32位)
 * @param speed 速度值
 * @return 是否成功发送命令
 */
bool ModbusManager::setSingleAxisAutoSpeed(qint32 speed)
{
    logOperation(QString("设置自动速度: %1").arg(speed), true);
    return writeHoldingRegisterDINT(ModbusRegisters::SingleAxis::AUTO_SPEED_LOW, speed);
}

/**
 * @brief 设置单动子JOG模式的位置 (16位)
 * @param position 位置值
 * @return 是否成功发送命令
 */
bool ModbusManager::setSingleAxisJogPosition(qint16 position)
{
    logOperation(QString("设置JOG位置: %1").arg(position), true);
    return writeHoldingRegister(ModbusRegisters::SingleAxis::JOG_POSITION, position);
}

/**
 * @brief 设置单动子JOG模式的速度 (32位)
 * @param speed 速度值
 * @return 是否成功发送命令
 */
bool ModbusManager::setSingleAxisJogSpeed(qint32 speed)
{
    logOperation(QString("设置JOG速度: %1").arg(speed), true);
    return writeHoldingRegisterDINT(ModbusRegisters::SingleAxis::JOG_SPEED_LOW, speed);
}

// --- 内部辅助函数 ---

// ModbusManager.cpp

/**
 * @brief 基础写操作：向单个保持寄存器写入一个16位值
 * @param address 寄存器地址
 * @param value 要写入的值
 * @return 操作是否成功
 */
bool ModbusManager::writeHoldingRegister(int address, quint16 value)
{
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState) {
        logOperation("写入单个寄存器失败", false, "客户端未连接");
        return false;
    }

    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, address, 1);
    writeUnit.setValue(0, value);

    if (auto *reply = m_modbusClient->sendWriteRequest(writeUnit, m_deviceId)) {
        // 使用事件循环等待异步操作完成
        QEventLoop loop;
        connect(reply, &QModbusReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QModbusDevice::NoError) {
            logOperation(QString("写入寄存器 %1").arg(address), true, QString("值: %2").arg(value));
            reply->deleteLater();
            return true;
        } else {
            logOperation(QString("写入寄存器 %1 失败").arg(address), false, reply->errorString());
            reply->deleteLater();
            return false;
        }
    }
    logOperation(QString("写入寄存器 %1 失败").arg(address), false, "发送请求失败");
    return false;
}

/**
 * @brief 写入一个32位整数 (DINT)，它会占用两个连续的16位寄存器
 * @param address 起始寄存器地址 (低位)
 * @param value 要写入的32位值
 * @return 操作是否成功
 */
bool ModbusManager::writeHoldingRegisterDINT(int address, qint32 value)
{
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState) {
        logOperation("写入32位整数失败", false, "客户端未连接");
        return false;
    }
    // 将32位值拆分为两个16位值（低位在前，高位在后）
    quint16 lowWord = value & 0xFFFF;
    quint16 highWord = (value >> 16) & 0xFFFF;

    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, address, 2);
    writeUnit.setValue(0, lowWord);
    writeUnit.setValue(1, highWord);

    if (auto *reply = m_modbusClient->sendWriteRequest(writeUnit, m_deviceId)) {
        QEventLoop loop;
        connect(reply, &QModbusReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QModbusDevice::NoError) {
            logOperation(QString("写入32位整数到 %1").arg(address), true, QString("值: %1").arg(value));
            reply->deleteLater();
            return true;
        } else {
            logOperation(QString("写入32位整数到 %1 失败").arg(address), false, reply->errorString());
            reply->deleteLater();
            return false;
        }
    }
    logOperation(QString("写入32位整数到 %1 失败").arg(address), false, "发送请求失败");
    return false;
}

/**
 * @brief 读取所有动子的状态数据
 * @param movers 动子数据列表，函数将用从PLC读取的数据更新此列表
 * @return 操作是否成功
 */
bool ModbusManager::readAllMoverData(QList<MoverData> &movers)
{
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState || movers.isEmpty()) {
        return false;
    }

    const int startAddress = ModbusRegisters::MoverStatus::BASE_ADDRESS;
    const int moverCount = movers.size();
    const int registersPerMover = ModbusRegisters::MoverStatus::REGISTERS_PER_MOVER;
    const int registerCount = moverCount * registersPerMover;

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, startAddress, registerCount);

    if (auto *reply = m_modbusClient->sendReadRequest(readUnit, m_deviceId)) {
        QEventLoop loop;
        connect(reply, &QModbusReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QModbusDevice::NoError) {
            const QVector<quint16> data = reply->result().values();
            // 解析数据并更新movers列表 (这部分逻辑与MainWindow中的类似)
            for (int i = 0; i < moverCount; ++i) {
                int baseIdx = i * registersPerMover;
                if (baseIdx + 3 < data.size()) { // 简单检查边界
                    qint32 pos = (data[baseIdx + 1] << 16) | data[baseIdx];
                    qint32 spd = (data[baseIdx + 3] << 16) | data[baseIdx + 2];
                    movers[i].position = pos / 1000.0;
                    movers[i].speed = spd / 1000.0;
                }
            }
            reply->deleteLater();
            return true;
        } else {
            logOperation("批量读取动子数据失败", false, reply->errorString());
            reply->deleteLater();
        }
    }
    return false;
}

/**
 * @brief 核心位操作函数：读取-修改-写回一个寄存器的特定位
 * @param bit 要操作的位索引 (0-15)
 * @param value true表示置1，false表示清0
 * @return 操作是否成功
 */
bool ModbusManager::setControlWordBit(int bit, bool value)
{
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState || bit < 0 || bit > 15) {
        logOperation("位操作失败", false, "客户端未连接或位索引无效");
        return false;
    }

    // 1. 同步读取当前的控制字
    quint16 currentWord = 0;
    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, ModbusRegisters::SingleAxis::CONTROL_WORD, 1);
    if (auto *readReply = m_modbusClient->sendReadRequest(readUnit, m_deviceId)) {
        QEventLoop loop;
        connect(readReply, &QModbusReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (readReply->error() == QModbusDevice::NoError) {
            currentWord = readReply->result().value(0);
        } else {
            logOperation("位操作读取阶段失败", false, readReply->errorString());
            readReply->deleteLater();
            return false;
        }
        readReply->deleteLater();
    } else {
        logOperation("位操作读取阶段失败", false, "发送请求失败");
        return false;
    }

    // 2. 修改位
    if (value) {
        currentWord |= (1 << bit); // 置1
    } else {
        currentWord &= ~(1 << bit); // 清0
    }

    // 3. 写回修改后的值
    return writeHoldingRegister(ModbusRegisters::SingleAxis::CONTROL_WORD, currentWord);
}


/**
 * @brief 设置Modbus客户端的信号槽连接
 */
void ModbusManager::setupModbusClient()
{
    if (!m_modbusClient) return;

    // 连接错误和状态变化的信号
    connect(m_modbusClient, &QModbusDevice::errorOccurred,
            this, &ModbusManager::onModbusError);

    logOperation("Modbus客户端事件绑定", true, "错误和状态变化监听已设置");
}

/**
 * @brief 清理Modbus客户端对象和缓存
 */
void ModbusManager::cleanup()
{
    if (m_modbusClient) {
        m_modbusClient->deleteLater();
        m_modbusClient = nullptr;
    }
    m_isConnected = false;
    logOperation("资源清理", true, "客户端对象和缓存已清理");
}


/**
 * @brief Modbus设备发生错误时的槽函数
 * @param error 错误类型
 */
void ModbusManager::onModbusError(QModbusDevice::Error error)
{
    QString errorMsg = errorString(error);
    logOperation("Modbus设备错误", false, QString("错误码: %1, 描述: %2").arg(error).arg(errorMsg));

    m_failedOperations++;
    emit connectionError(errorMsg);
}

/**
 * @brief 将Modbus错误枚举转换为字符串描述
 * @param error 错误类型
 * @return 错误描述字符串
 */
QString ModbusManager::errorString(QModbusDevice::Error error) const
{
    switch (error) {
    case QModbusDevice::ReadError:
        return "读取错误";
    case QModbusDevice::WriteError:
        return "写入错误";
    case QModbusDevice::ConnectionError:
        return "连接错误";
    case QModbusDevice::ConfigurationError:
        return "配置错误";
    case QModbusDevice::TimeoutError:
        return "超时错误";
    case QModbusDevice::ProtocolError:
        return "协议错误";
    case QModbusDevice::ReplyAbortedError:
        return "操作被中止";
    case QModbusDevice::UnknownError:
        return "未知错误";
    default:
        return QString("错误代码: %1").arg(error);
    }
}

// --- 参数设置命令实现 ---

/**
 * @brief 设置系统中的动子数量
 * @param count 动子数量
 * @return 是否成功发送命令
 */

// --- 周期性任务 ---

/**
 * @brief 启动周期性读取任务
 * @param intervalMs 读取间隔，单位毫秒
 */
void ModbusManager::startCyclicRead(int intervalMs)
{
    if (m_cyclicTimer->isActive()) {
        m_cyclicTimer->stop();
    }
    m_cyclicTimer->start(intervalMs);
    logOperation("开始周期性读取", true, QString("间隔: %1ms").arg(intervalMs));
}

/**
 * @brief 停止周期性读取任务
 */
void ModbusManager::stopCyclicRead()
{
    if (m_cyclicTimer->isActive()) {
        m_cyclicTimer->stop();
        logOperation("停止周期性读取", true, "定时器已停止");
    }
}
