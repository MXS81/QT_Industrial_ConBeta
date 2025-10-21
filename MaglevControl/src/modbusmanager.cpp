#include "modbusmanager.h"
#include <QEventLoop>

ModbusManager::ModbusManager(QObject *parent)
    : QObject(parent)
    , m_modbusClient(nullptr)
    , m_heartbeatTimer(nullptr)
    , m_lastPort(0)
{
    m_modbusClient = new QModbusTcpClient(this);

    // 连接状态变化信号
    connect(m_modbusClient, &QModbusDevice::stateChanged,
            this, &ModbusManager::onStateChanged);

    setupHeartbeat();
}


ModbusManager::~ModbusManager()
{
    if (m_modbusClient && m_modbusClient->state() == QModbusDevice::ConnectedState) {
        m_modbusClient->disconnectDevice();
    }
}


bool ModbusManager::connectToDevice(const QString& ip, int port)
{
    if (m_modbusClient->state() == QModbusDevice::ConnectedState) {
        return true;
    }

    m_modbusClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, QVariant(ip));
    m_modbusClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, QVariant(port));

    // 记录最近连接参数，便于重连
    m_lastIP = ip;
    m_lastPort = port;

    return m_modbusClient->connectDevice();
}


void ModbusManager::disconnectDevice()
{
    stopHeartbeat();
    if (m_modbusClient && m_modbusClient->state() == QModbusDevice::ConnectedState) {
        m_modbusClient->disconnectDevice();
    } else if (m_modbusClient && m_modbusClient->state() == QModbusDevice::ConnectingState) {
        m_modbusClient->disconnectDevice();
    }
}


QModbusDevice::State ModbusManager::connectionState() const
{
    return m_modbusClient ? m_modbusClient->state() : QModbusDevice::UnconnectedState;
}


QString ModbusManager::errorString() const
{
    return m_modbusClient ? m_modbusClient->errorString() : QString("Client not initialized");
}


// 掩码写（0x16）同步；失败时自动降级为读-改-写
bool ModbusManager::maskWriteRegisterSync(int address, quint16 andMask, quint16 orMask, int timeoutMs)
{
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState) {
        emit errorOccurred(QStringLiteral("设备未连接"));
        return false;
    }

    // 超时分配：优先保证降级路径有充足时间（适配部分网关对0x16慢/不稳定）
    const int base = (timeoutMs > 0 ? timeoutMs : 2800);
    int t1 = qMax(400, (base * 4) / 10); // 约40%给0x16
    int t2 = base - t1;                  // 约60%给降级

    auto tryMask16 = [&]() -> bool {
        QByteArray payload;
        payload.append(char((address >> 8) & 0xFF));
        payload.append(char(address & 0xFF));
        payload.append(char((andMask >> 8) & 0xFF));
        payload.append(char(andMask & 0xFF));
        payload.append(char((orMask >> 8) & 0xFF));
        payload.append(char(orMask & 0xFF));

        QModbusRequest req(QModbusPdu::MaskWriteRegister, payload);
        if (QModbusReply* r = m_modbusClient->sendRawRequest(req, m_unitId)) {
            QEventLoop loop; QTimer timer; timer.setSingleShot(true);
            bool tmo = false; QModbusDevice::Error err = QModbusDevice::NoError; QString errStr;
            QObject::connect(&timer, &QTimer::timeout, [&](){ tmo = true; loop.quit(); });
            QObject::connect(r, &QModbusReply::finished, [&](){ err = r->error(); errStr = r->errorString(); loop.quit(); });
            timer.start(t1);
            loop.exec();
            r->deleteLater();
            if (!tmo && err == QModbusDevice::NoError) return true;
            qDebug() << "[MASK16] fail:" << (tmo ? "timeout" : errStr);
            return false;
        }
        qDebug() << "[MASK16] send fail:" << m_modbusClient->errorString();
        return false;
    };

    if (tryMask16()) return true;

    // 降级：读-改-写
    quint16 oldVal = 0;
    // 降级读-改-写：读写分别分配较宽的超时，避免“读取超时”
    int readT = qMax(800, (t2 * 2) / 3);
    int writeT = qMax(600, t2 - readT);

    if (!readRegisterSync(address, oldVal, readT)) {
        emit errorOccurred(QString("掩码写失败: 0x16失败且读取寄存器0x%1失败")
                           .arg(address, 4, 16, QChar('0')));
        return false;
    }
    quint16 newVal = (oldVal & andMask) | orMask;

    QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, address, 1);
    unit.setValue(0, newVal);
    if (QModbusReply* w = m_modbusClient->sendWriteRequest(unit, m_unitId)) {
        QEventLoop loop; QTimer timer; timer.setSingleShot(true);
        bool tmo = false; QModbusDevice::Error err = QModbusDevice::NoError; QString errStr;
        QObject::connect(&timer, &QTimer::timeout, [&](){ tmo = true; loop.quit(); });
        QObject::connect(w, &QModbusReply::finished, [&](){ err = w->error(); errStr = w->errorString(); loop.quit(); });
        timer.start(writeT);
        loop.exec();
        w->deleteLater();
        if (!tmo && err == QModbusDevice::NoError) return true; // 降级成功

        emit errorOccurred(QString("掩码写失败且降级写回失败: addr=0x%1, 错误=%2")
                           .arg(address, 4, 16, QChar('0'))
                           .arg(tmo ? "timeout" : errStr));
        return false;
    } else {
        emit errorOccurred(QString("掩码写失败且降级发送失败: %1").arg(m_modbusClient->errorString()));
        return false;
    }
}


bool ModbusManager::writeRegister(int address, quint16 value)
{
    // 对寄存器[0]添加互斥锁保护，避免与心跳冲突
    QMutexLocker locker(address == m_heartbeatRegisterAddress ? &m_registerMutex : nullptr);
    
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState) {
        emit errorOccurred(QStringLiteral("设备未连接"));
        return false;
    }

    // 地址校验
    if (address < 0 || address > 65535) {
        emit errorOccurred(QString("寄存器地址 0x%1 越界").arg(address, 4, 16, QChar('0')));
        return false;
    }
    
    // 如果是心跳寄存器，添加调试信息
    if (address == m_heartbeatRegisterAddress) {
        qDebug() << "控制寄存器写入，值:" << QString("0x%1").arg(value, 4, 16, QChar('0'));
    }

    QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, address, 1);
    unit.setValue(0, value);

    if (QModbusReply* reply = m_modbusClient->sendWriteRequest(unit, m_unitId)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply, address, value]() {
                if (reply->error() != QModbusDevice::NoError) {
                    QString errorMsg = QString("寄存器 0x%1 写入失败: %2")
                                       .arg(address, 4, 16, QChar('0'))
                                       .arg(reply->errorString());
                    emit errorOccurred(errorMsg);
                } else {
                    qDebug() << QString("寄存器 0x%1 写入成功, 值: %2")
                                .arg(address, 4, 16, QChar('0'))
                                .arg(value);
                }
                reply->deleteLater();
            });
        } else {
            // 已完成（广播写或立即完成）
            reply->deleteLater();
        }
        return true;
    }

    emit errorOccurred(QString("发送写请求失败: %1").arg(m_modbusClient->errorString()));
    return false;
}


bool ModbusManager::writeRegisterSync(int address, quint16 value, int timeoutMs)
{
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState) {
        emit errorOccurred(QStringLiteral("设备未连接"));
        return false;
    }
    QModbusDataUnit unit(QModbusDataUnit::HoldingRegisters, address, 1);
    unit.setValue(0, value);
    if (QModbusReply* reply = m_modbusClient->sendWriteRequest(unit, 1)) {
        QEventLoop loop; QTimer timer; timer.setSingleShot(true);
        bool timeoutOccurred = false; QModbusDevice::Error err = QModbusDevice::NoError; QString errStr;
        QObject::connect(&timer, &QTimer::timeout, [&](){ timeoutOccurred = true; loop.quit(); });
        QObject::connect(reply, &QModbusReply::finished, [&](){ err = reply->error(); errStr = reply->errorString(); loop.quit(); });
        timer.start(timeoutMs);
        loop.exec();
        reply->deleteLater();
        if (!timeoutOccurred && err == QModbusDevice::NoError) return true;
        emit errorOccurred(QString("寄存器 0x%1 同步写失败: %2")
                           .arg(address, 4, 16, QChar('0')).arg(timeoutOccurred ? "timeout" : errStr));
        return false;
    }
    emit errorOccurred(QString("发送写请求失败: %1").arg(m_modbusClient->errorString()));
    return false;
}


bool ModbusManager::readRegisterSync(int address, quint16& value, int timeoutMs)
{
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState) {
        emit errorOccurred(QStringLiteral("设备未连接"));
        return false;
    }

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, address, 1);
    if (QModbusReply* reply = m_modbusClient->sendReadRequest(readUnit, m_unitId)) {
        if (reply->isFinished()) {
            reply->deleteLater();
            return false;
        }

        QEventLoop loop; QTimer timer; timer.setSingleShot(true);
        bool timeoutOccurred = false;
        QModbusDevice::Error error = QModbusDevice::NoError; QString errorString;

        QObject::connect(&timer, &QTimer::timeout, [&]() {
            timeoutOccurred = true; loop.quit();
        });

        QObject::connect(reply, &QModbusReply::finished, [&]() {
            error = reply->error();
            if (error == QModbusDevice::NoError) {
                QModbusDataUnit result = reply->result();
                value = result.value(0);
            } else {
                errorString = reply->errorString();
            }
            loop.quit();
        });

        timer.start(timeoutMs);
        loop.exec();

        bool success = false;
        if (timeoutOccurred) {
            emit errorOccurred(QString("寄存器 0x%1 读取超时").arg(address, 4, 16, QChar('0')));
        } else if (error == QModbusDevice::NoError) {
            qDebug() << QString("[心跳降级] 寄存器 0x%1 同步读取成功, 值: %2").arg(address, 4, 16, QChar('0')).arg(value);
            success = true;
        } else {
            emit errorOccurred(QString("寄存器 0x%1 读取失败: %2").arg(address, 4, 16, QChar('0')).arg(errorString));
        }

        reply->deleteLater();
        return success;
    }

    emit errorOccurred(QString("发送读取请求失败: %1").arg(m_modbusClient->errorString()));
    return false;
}


void ModbusManager::readRegister(int address)
{
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState) {
        emit errorOccurred(QStringLiteral("设备未连接"));
        return;
    }

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, address, 1);

    if (QModbusReply* reply = m_modbusClient->sendReadRequest(readUnit, m_unitId)) {
        connect(reply, &QModbusReply::finished, this, [this, reply, address]() {
            if (reply->error() == QModbusDevice::NoError) {
                QModbusDataUnit result = reply->result();
                quint16 value = result.value(0);
                emit registerDataRead(value, address);
            } else if (reply->error() == QModbusDevice::ProtocolError) {
                QModbusResponse response = reply->rawResult();
                if (response.isException()) {
                    quint8 exceptionCode = response.exceptionCode();
                    QString errorMsg = QString("Modbus异常应答(地址 0x%1), 码: %2")
                                       .arg(address, 4, 16, QChar('0'))
                                       .arg(exceptionCode);
                    emit errorOccurred(errorMsg);
                }
            } else {
                QString errorMsg = QString("读取寄存器 0x%1 失败: %2")
                                   .arg(address, 4, 16, QChar('0'))
                                   .arg(reply->errorString());
                emit errorOccurred(errorMsg);
            }
            reply->deleteLater();
        });
    } else {
        QString errorMsg = QString("发送读取请求失败: %1").arg(m_modbusClient->errorString());
        emit errorOccurred(errorMsg);
    }
}


void ModbusManager::startHeartbeat(int intervalMs)
{
    if (m_heartbeatTimer) {
        m_heartbeatTimer->setInterval(intervalMs);
        m_heartbeatTimer->start();
    }
}


void ModbusManager::stopHeartbeat()
{
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
    }
}


void ModbusManager::setupHeartbeat()
{
    m_heartbeatTimer = new QTimer(this);
    // 默认2秒（可自定义）
    m_heartbeatTimer->setInterval(kHeartbeatIntervalMs);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ModbusManager::onHeartbeatTimeout);
}


void ModbusManager::onStateChanged(QModbusDevice::State newState)
{
    emit stateChanged(newState);

    // 连接成功后延时启动心跳（给网关/PLC预热时间）
    if (newState == QModbusDevice::ConnectedState) {
        m_heartbeatToggleState = false; // 重置心跳写入状态
        QTimer::singleShot(400, this, [this]() { startHeartbeat(kHeartbeatIntervalMs); });
    } else if (newState == QModbusDevice::UnconnectedState) {
        stopHeartbeat();
    }
}


void ModbusManager::onHeartbeatTimeout()
{
    if (m_modbusClient->state() != QModbusDevice::ConnectedState)
        return;

    if (m_heartbeatToggleEnabled) {
        // 尝试获取互斥锁，如果控制操作正在进行则跳过本次心跳
        if (!m_registerMutex.tryLock()) {
            qDebug() << "心跳跳过：控制操作正在进行";
            return;
        }

        // 仅翻转 bit15，不影响其他位
        m_heartbeatToggleState = !m_heartbeatToggleState;
        
        qDebug() << "心跳执行，bit15状态:" << (m_heartbeatToggleState ? "1" : "0");

        quint16 andMask, orMask;
        if (m_heartbeatToggleState) {
            andMask = 0xFFFF;  // 保持其他位
            orMask = 0x8000;   // 置位 bit15
        } else {
            andMask = 0x7FFF;  // 清除 bit15，其余位保持
            orMask = 0x0000;
        }

        // 优先掩码写，失败则降级，缩短超时时间避免长时间占用锁
        int opTimeout = qMax(800, kHeartbeatIntervalMs / 3); // 缩短超时时间
        bool success = maskWriteRegisterSync(m_heartbeatRegisterAddress, andMask, orMask, opTimeout);
        
        m_registerMutex.unlock(); // 释放锁
        
        if (!success) {
            qDebug() << "心跳写入失败";
        }
    } else {
        // 仅读取作为保活（不需要锁）
        QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, m_heartbeatRegisterAddress, 1);
        if (QModbusReply* reply = m_modbusClient->sendReadRequest(readUnit, m_unitId)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, reply, &QObject::deleteLater);
            } else {
                reply->deleteLater();
            }
        }
    }
}
