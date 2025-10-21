#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QTextEdit>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMutex>

class LogManager : public QObject
{
    Q_OBJECT

public:
    enum LogLevel {
        Info,
        Warning,
        Error,
        Debug
    };

    explicit LogManager(QObject *parent = nullptr);
    ~LogManager();
    
    // 设置日志显示控件
    void setLogWidget(QTextEdit* logWidget);
    
    // 启用/禁用文件日志
    void setFileLogging(bool enabled, const QString& logDir = "logs");
    
    // 设置日志级别
    void setLogLevel(LogLevel level);
    
    // 日志记录方法
    void logInfo(const QString& message);
    void logWarning(const QString& message);
    void logError(const QString& message);
    void logDebug(const QString& message);
    void log(LogLevel level, const QString& message);
    
    // 清空日志
    void clearLog();
    
    // 获取日志内容
    QString getLogContent() const;

signals:
    void logMessage(const QString& formattedMessage);

private:
    QTextEdit* m_logWidget;
    QFile* m_logFile;
    QTextStream* m_logStream;
    QMutex m_mutex;
    
    bool m_fileLoggingEnabled;
    LogLevel m_currentLogLevel;
    QString m_logDirectory;
    
    QString formatMessage(LogLevel level, const QString& message) const;
    QString levelToString(LogLevel level) const;
    QString levelToColor(LogLevel level) const;
    void writeToWidget(const QString& formattedMessage, LogLevel level);
    void writeToFile(const QString& formattedMessage);
    void initFileLogging();
    void closeFileLogging();
    QString generateLogFileName() const;
};

#endif // LOGMANAGER_H