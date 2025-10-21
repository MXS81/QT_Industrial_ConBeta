// DebugHelper.h - 调试和诊断工具
#ifndef DEBUGHELPER_H
#define DEBUGHELPER_H

#include <QObject>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QFile>
#include "MoverData.h"

class DebugHelper : public QObject
{
    Q_OBJECT

public:
    static DebugHelper* instance();

    // 动子数据验证
    static bool validateMoverData(const QList<MoverData>& movers, const QString& context = "");
    static bool validateMoverIndex(int index, int totalCount, const QString& context = "");

    // 内存和状态检查
    static void logSystemState(const QList<MoverData>& movers, const QString& operation = "");
    static void logMemoryUsage();

    // 崩溃预防检查
    static bool safeIndexAccess(int index, int size, const QString& arrayName = "array");
    static bool isValidPointer(void* ptr, const QString& ptrName = "pointer");

    // 错误记录
    static void logError(const QString& function, const QString& error, const QString& details = "");
    static void logCrashPrevention(const QString& function, const QString& reason);

private:
    explicit DebugHelper(QObject *parent = nullptr);
    static DebugHelper* s_instance;

    void writeToLogFile(const QString& message);
    QString m_logFilePath;
};

#endif // DEBUGHELPER_H
