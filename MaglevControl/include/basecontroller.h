#ifndef BASECONTROLLER_H
#define BASECONTROLLER_H

#include <QObject>
#include "modbusmanager.h"
#include "stylemanager.h"

/**
 * 控制器基类，提供通用功能
 */
class BaseController : public QObject
{
    Q_OBJECT

public:
    explicit BaseController(ModbusManager* modbusManager, QObject *parent = nullptr);
    virtual ~BaseController() = default;

protected:
    // 检查设备连接状态的通用方法
    bool checkConnection();
    
    // 发送格式化错误信息的便利方法
    void emitError(const QString& baseMessage, const QString& detail = "");
    void emitError(const QString& baseMessage, int value);

signals:
    void errorOccurred(const QString& error);
    void statusChanged(const QString& message);

protected:
    ModbusManager* m_modbusManager;
};

#endif // BASECONTROLLER_H