// DebugHelper.cpp
#include "DebugHelper.h"
#include <QApplication>
#include <QStandardPaths>

DebugHelper* DebugHelper::s_instance = nullptr;

DebugHelper::DebugHelper(QObject *parent) : QObject(parent)
{
    // 创建日志文件路径
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    m_logFilePath = logDir + "/debug_log.txt";

    qDebug() << "DebugHelper initialized, log file:" << m_logFilePath;
}

DebugHelper* DebugHelper::instance()
{
    if (!s_instance) {
        s_instance = new DebugHelper();
    }
    return s_instance;
}

bool DebugHelper::validateMoverData(const QList<MoverData>& movers, const QString& context)
{
    QString func = QString("validateMoverData[%1]").arg(context);

    if (movers.isEmpty()) {
        logError(func, "动子列表为空");
        return false;
    }

    for (int i = 0; i < movers.size(); ++i) {
        const MoverData& mover = movers[i];

        // 检查ID一致性
        if (mover.id != i) {
            logError(func, QString("动子%1的ID不匹配（期望%2，实际%3）").arg(i).arg(i).arg(mover.id));
        }

        // 检查位置有效性
        if (mover.position < 0 || mover.position > 10000) {
            logError(func, QString("动子%1位置异常：%2mm").arg(i).arg(mover.position));
        }

        // 检查速度有效性
        if (mover.speed < 0 || mover.speed > 2000) {
            logError(func, QString("动子%1速度异常：%2mm/s").arg(i).arg(mover.speed));
        }

        // 检查状态字符串
        if (mover.status.isEmpty()) {
            logError(func, QString("动子%1状态字符串为空").arg(i));
        }

        // 检查温度
        if (mover.temperature < 0 || mover.temperature > 100) {
            logError(func, QString("动子%1温度异常：%2°C").arg(i).arg(mover.temperature));
        }
    }

    qDebug() << func << "验证完成，动子数量:" << movers.size();
    return true;
}

bool DebugHelper::validateMoverIndex(int index, int totalCount, const QString& context)
{
    QString func = QString("validateMoverIndex[%1]").arg(context);

    if (index < 0) {
        logError(func, QString("索引为负数：%1").arg(index));
        return false;
    }

    if (index >= totalCount) {
        logError(func, QString("索引越界：%1 >= %2").arg(index).arg(totalCount));
        return false;
    }

    return true;
}

void DebugHelper::logSystemState(const QList<MoverData>& movers, const QString& operation)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString message = QString("[%1] 系统状态检查 - 操作: %2").arg(timestamp,operation);

    message += QString("\n  动子数量: %1").arg(movers.size());

    for (int i = 0; i < movers.size(); ++i) {
        const MoverData& mover = movers[i];
        message += QString("\n  动子%1: 位置=%2mm, 速度=%3mm/s, 状态=%4")
                       .arg(i)
                       .arg(mover.position, 0, 'f', 1)
                       .arg(mover.speed, 0, 'f', 1)
                       .arg(mover.status);
    }

    qDebug().noquote() << message;
    instance()->writeToLogFile(message);
}

bool DebugHelper::safeIndexAccess(int index, int size, const QString& arrayName)
{
    if (index < 0 || index >= size) {
        QString error = QString("安全检查失败：%1[%2]，数组大小=%3").arg(arrayName).arg(index).arg(size);
        logCrashPrevention("safeIndexAccess", error);
        return false;
    }
    return true;
}

bool DebugHelper::isValidPointer(void* ptr, const QString& ptrName)
{
    if (ptr == nullptr) {
        QString error = QString("空指针检测：%1 为 nullptr").arg(ptrName);
        logCrashPrevention("isValidPointer", error);
        return false;
    }
    return true;
}

void DebugHelper::logError(const QString& function, const QString& error, const QString& details)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString message = QString("[%1] ERROR in %2: %3").arg(timestamp,function,error);

    if (!details.isEmpty()) {
        message += QString(" | Details: %1").arg(details);
    }

    qCritical().noquote() << message;
    instance()->writeToLogFile(message);
}

void DebugHelper::logCrashPrevention(const QString& function, const QString& reason)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString message = QString("[%1] CRASH PREVENTED in %2: %3").arg(timestamp,function,reason);

    qWarning().noquote() << message;
    instance()->writeToLogFile(message);
}

void DebugHelper::writeToLogFile(const QString& message)
{
    QFile file(m_logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << message << "\n";
        stream.flush();
    }
}

// 便于使用的宏定义
#define VALIDATE_MOVERS(movers, context) \
if (!DebugHelper::validateMoverData(movers, context)) { \
        qWarning() << "动子数据验证失败:" << context; \
}

#define SAFE_MOVER_ACCESS(movers, index, context) \
(DebugHelper::safeIndexAccess(index, movers.size(), QString("movers[%1]").arg(context)))

#define CHECK_POINTER(ptr, name) \
    DebugHelper::isValidPointer(ptr, name)

#define LOG_OPERATION(movers, op) \
    DebugHelper::logSystemState(movers, op)
