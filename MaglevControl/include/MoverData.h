#ifndef MOVERDATA_H
#define MOVERDATA_H

#include <QString>
#include <QDateTime>

struct MoverData {
    int id;
    // 位置相关 (mm)
    double position;        // 当前实际位置 (从PLC读取)
    double target;         // 目标位置
    double commandedTarget; // 已发送给PLC的目标位置

    // 速度相关 (mm/s)
    double speed;          // 当前实际速度 (从PLC读取)
    double targetSpeed;    // 目标速度
    double commandedSpeed; // 已发送给PLC的速度

    // 方向相关（运动方向规定逆时针为前进方向）
    enum MovementDirection {
        AUTO,           // 自动选择最短路径（用于位置移动）
        CLOCKWISE,      // 顺时针（后退）
        COUNTERCLOCKWISE // 逆时针（前进）
    };
    MovementDirection movementDirection;

    // 加速度 (mm/s²)
    double acceleration;   // 当前加速度
    double targetAcceleration; // 目标加速度

    // 状态信息
    QString status;        // 当前状态 (从PLC读取)
    QString lastCommand;   // 最后发送的命令
    double temperature;    // 温度 (从PLC读取)
    bool isSelected;

    // PLC特定状态
    bool plcConnected;     // PLC连接状态
    bool inPosition;       // 是否到达目标位置
    bool hasError;         // 是否有错误
    QString errorMessage;  // 错误信息
    bool isEnabled;        // 动子使能

    // 时间戳
    QDateTime lastUpdateTime;  // 最后更新时间
    QDateTime lastCommandTime; // 最后命令时间

    // 构造函数
    MoverData()
        : id(0)
        , position(0.0)
        , target(0.0)
        , commandedTarget(0.0)
        , speed(0.0)
        , targetSpeed(100.0)
        , commandedSpeed(100.0)
        , movementDirection(AUTO)
        , acceleration(500.0)
        , targetAcceleration(500.0)
        , status("未知")
        , lastCommand("无")
        , temperature(25.0)
        , isSelected(false)
        , plcConnected(false)
        , inPosition(false)
        , hasError(false)
        , errorMessage("")
        , isEnabled(false)
        , lastUpdateTime(QDateTime::currentDateTime())
        , lastCommandTime(QDateTime::currentDateTime())
    {
    }

    MoverData(int _id, double _pos = 0.0)
        : id(_id)
        , position(_pos)
        , target(_pos)
        , commandedTarget(_pos)
        , speed(0.0)
        , targetSpeed(100.0)
        , commandedSpeed(100.0)
        , movementDirection(AUTO)
        , acceleration(500.0)
        , targetAcceleration(500.0)
        , status("停止")
        , lastCommand("无")
        , temperature(25.0)
        , isSelected(false)
        , plcConnected(false)
        , inPosition(true)
        , hasError(false)
        , errorMessage("")
        , isEnabled(false)
        , lastUpdateTime(QDateTime::currentDateTime())
        , lastCommandTime(QDateTime::currentDateTime())
    {
    }

    // 检查数据是否过时 (超过2秒没有更新)
    bool isDataStale() const {
        return lastUpdateTime.msecsTo(QDateTime::currentDateTime()) > 2000;
    }

    // 检查命令是否执行完成
    bool isCommandCompleted() const {
        return inPosition && (qAbs(position - commandedTarget) < 1.0);
    }

};

#endif // MOVERDATA_H
