#include "basecontroller.h"

BaseController::BaseController(ModbusManager* modbusManager, QObject *parent)
    : QObject(parent)
    , m_modbusManager(modbusManager)
{
}

bool BaseController::checkConnection()
{
    if (!m_modbusManager || m_modbusManager->connectionState() != QModbusDevice::ConnectedState) {
        emit errorOccurred(ErrorMessages::DEVICE_NOT_CONNECTED);
        return false;
    }
    return true;
}

void BaseController::emitError(const QString& baseMessage, const QString& detail)
{
    QString fullMessage = detail.isEmpty() ? baseMessage : baseMessage + ": " + detail;
    emit errorOccurred(fullMessage);
}

void BaseController::emitError(const QString& baseMessage, int value)
{
    emit errorOccurred(QString("%1: %2").arg(baseMessage).arg(value));
}