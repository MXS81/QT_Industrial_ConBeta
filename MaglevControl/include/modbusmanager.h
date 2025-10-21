#ifndef MODBUSMANAGER_H
#define MODBUSMANAGER_H

#include <QObject>
#include <QModbusTcpClient>
#include <QModbusDataUnit>
#include <QModbusReply>
#include <QTimer>
#include <QDebug>
#include <QVariant>
#include <QMutex>

/**
 * ModbusManager
 * 职责：集中管理 Modbus TCP 连接与寄存器读写；提供心跳掩码写（优先0x16，失败降级“读-改-写”）。
 */
class ModbusManager : public QObject
{
    Q_OBJECT

public:
    explicit ModbusManager(QObject *parent = nullptr);
    ~ModbusManager();

    // 统一心跳配置
    static constexpr int kHeartbeatIntervalMs = 3000;        // 默认3秒
    static constexpr int kHeartbeatRegisterAddress = 0x0000; // 心跳寄存器地址，默认0

    // 连接管理
    bool connectToDevice(const QString& ip, int port);
    void disconnectDevice();
    QModbusDevice::State connectionState() const;
    QString errorString() const;

    // 从站地址（Unit ID）管理
    void setUnitId(int unitId) { m_unitId = unitId; }
    int unitId() const { return m_unitId; }

    // 寄存器读写
    bool writeRegister(int address, quint16 value);
    bool writeRegisterSync(int address, quint16 value, int timeoutMs = 5000);
    void readRegister(int address);
    bool readRegisterSync(int address, quint16& value, int timeoutMs = 5000);

    // 心跳（默认2秒，可自定义）
    void startHeartbeat(int intervalMs = kHeartbeatIntervalMs);
    void stopHeartbeat();

    // 心跳写控制
    void setHeartbeatRegisterAddress(int address) { m_heartbeatRegisterAddress = address; }
    void setHeartbeatToggleEnabled(bool enabled) { m_heartbeatToggleEnabled = enabled; }

    /**
     * 掩码写（0x16）同步接口，必要时自动降级为“读-改-写”。
     * 1) 优先尝试功能码0x16；
     * 2) 若失败（无效响应/协议错误/超时等），读取当前值并按 (old & andMask) | orMask 写回；
     * 3) 仅当两条路径均失败时才触发错误信号。
     */
    bool maskWriteRegisterSync(int address, quint16 andMask, quint16 orMask, int timeoutMs = 5000);

signals:
    // 连接状态变化
    void stateChanged(QModbusDevice::State newState);
    // 数据读取成功
    void registerDataRead(quint16 value, int address);
    // 错误信息
    void errorOccurred(const QString& errorMessage);

private slots:
    void onStateChanged(QModbusDevice::State newState);
    void onHeartbeatTimeout();

private:
    QModbusTcpClient* m_modbusClient;
    QTimer* m_heartbeatTimer;

    // 最近连接参数（用于重连或诊断）
    QString m_lastIP;
    int m_lastPort;

    void setupHeartbeat();

    // 心跳写：仅翻转 bit15，不影响其他位
    int m_heartbeatRegisterAddress = kHeartbeatRegisterAddress;
    bool m_heartbeatToggleEnabled = true; // 启用bit15翻转心跳（已添加互斥锁保护）
    bool m_heartbeatToggleState = false;  // false->写0x0000, true->写0x8000

    // Modbus TCP 从站地址（Unit ID），默认1
    int m_unitId = 1;
    
    // 寄存器访问互斥锁（防止心跳与控制操作冲突）
    QMutex m_registerMutex;
};

#endif // MODBUSMANAGER_H
