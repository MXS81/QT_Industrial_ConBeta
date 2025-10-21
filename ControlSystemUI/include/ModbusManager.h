#ifndef MODBUSMANAGER_H
#define MODBUSMANAGER_H

#include <QObject>
#include <QTimer>
#include <QModbusClient>
#include <QModbusDevice>
#include <QModbusDataUnit>
#include <QModbusReply>
#include <QModbusTcpClient>
#include <QModbusTcpServer>
#include <QTcpSocket>
#include <QSerialPort>
#include <QMutex>
#include "MoverData.h"

class MainWindow;
namespace ModbusRegisters {

    // --- 单动子控制寄存器 (基于 mainwindow.cpp 分析) ---
    namespace SingleAxis {
        const int CONTROL_WORD = 0x0000;         // 控制字 (保持寄存器)
        const int AUTO_SPEED_LOW = 0x0001;       // 自动速度-低16位
        const int AUTO_SPEED_HIGH = 0x0002;      // 自动速度-高16位
        const int JOG_POSITION = 0x0003;         // JOG位置
        const int JOG_SPEED_LOW = 0x0004;        // JOG速度-低16位
        const int JOG_SPEED_HIGH = 0x0005;       // JOG速度-高16位

    // 控制字 (CONTROL_WORD) 的位定义
        const int ENABLE_BIT = 0;                // bit0: 使能 (1=使能, 0=禁用)
        const int RUN_MODE_BIT = 1;              // bit1: 运行模式 (1=自动, 0=点动)
        const int MANUAL_ALLOW_BIT = 2;          // bit2: 允许手动 (1=允许, 0=禁止)
        const int JOG_LEFT_BIT = 3;              // bit3: 向左点动
        const int JOG_RIGHT_BIT = 4;             // bit4: 向右点动
        const int AUTO_RUN_BIT = 5;              // bit5: 自动运行 (1=运行, 0=停止)
    }

    // 动子状态（用于配方和主窗口）
    namespace MoverStatus {
        const int BASE_ADDRESS = 100;
        const int REGISTERS_PER_MOVER = 10;
        const int POSITION_LOW = 0;              // Position low word offset
        const int VELOCITY_LOW = 2;              // Velocity low word offset
        const int TARGET_LOW = 8;
        }

    // 系统状态
    namespace SystemStatus {
        const int SYSTEM_READY = 200;
        const int SYSTEM_ERROR = 201;
        const int EMERGENCY_STOP = 202;
        const int MOVER_COUNT = 203;
        }

    // --- 多动子控制寄存器 ---
    namespace MultiAxis {
        const int ENABLE_BASE_ADDRESS = 0x0100;  // 动子使能状态基地址 (保持寄存器)
        const int SPEED_BASE_ADDRESS = 0x0200;   // 动子速度设置基地址 (保持寄存器)
    }
}

class ModbusManager : public QObject
{
    Q_OBJECT

public:
    explicit ModbusManager(QObject *parent = nullptr);
    ~ModbusManager();

    // 连接管理
    bool connectToDevice(const QString &host, int port = 502, int deviceId = 1);
    bool connectToSerialDevice(const QString &portName, int baudRate = 9600, int deviceId = 1);
    void disconnectFromDevice();
    bool isConnected() const;

    // --- 单动子高级控制接口 ---
    bool setSingleAxisEnable(bool enable);
    bool setSingleAxisRunMode(bool isAutoMode);
    bool setSingleAxisManualControl(bool allow);
    bool setSingleAxisJog(int direction); // 0: 停止, 1: 左, 2: 右
    bool setSingleAxisAutoRun(bool run);
    bool setSingleAxisAutoSpeed(qint32 speed);
    bool setSingleAxisJogPosition(qint16 position);
    bool setSingleAxisJogSpeed(qint32 speed);

    // --- 多动子控制高级接口 ---
    bool setMultiAxisEnable(int moverId, bool enable);
    bool setMultiAxisSpeed(int moverId, quint16 speed);

    // 获取连接模式
    QString getConnectionInfo() const;
    int getSuccessfulOperations() const { return m_successfulOperations; }
    int getFailedOperations() const { return m_failedOperations; }

    // 批量数据读取（更高效）
    bool readAllMoverData(QList<MoverData> &movers);
    bool writeHoldingRegisterDINT(int address, qint32 value);

    // 循环读取
    void startCyclicRead(int intervalMs = 200);
    void stopCyclicRead();

    // 日志记录接口
    void setMainWindow(MainWindow *mainWindow) { m_mainWindow = mainWindow; }

signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    void dataReceived(int startAddress, const QVector<quint16> &data);
    void systemStatusChanged(bool initialized, bool enabled);

private slots:
    // Modbus内部响应处理
    void onModbusError(QModbusDevice::Error error);
private:
    void setupModbusClient();
    void cleanup();
    QString errorString(QModbusDevice::Error error) const;
    void logOperation(const QString &operation, bool success, const QString &details = QString());

    // 基础读写方法
    bool writeHoldingRegister(int address, quint16 value);
    bool readHoldingRegisters(int startAddress, int count);
    QVector<quint16> readRegistersSync(int address, int count);

    // 核心位操作函数
    bool setControlWordBit(int bit, bool value);
    quint16 readControlWordSync();

    QModbusClient *m_modbusClient;
    QTimer *m_cyclicTimer;
    QMutex m_mutex;
    MainWindow *m_mainWindow;

    // 连接参数
    QString m_host;
    int m_port;
    int m_deviceId;
    bool m_isConnected;

    // 统计信息
    int m_successfulOperations;
    int m_failedOperations;

};

#endif // MODBUSMANAGER_H
