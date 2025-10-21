#include "logmanager.h"
#include <QTextCursor>
#include <QScrollBar>
#include <QMutexLocker>

LogManager::LogManager(QObject *parent)
    : QObject(parent)
    , m_logWidget(nullptr)
    , m_logFile(nullptr)
    , m_logStream(nullptr)
    , m_fileLoggingEnabled(false)
    , m_currentLogLevel(Info)
    , m_logDirectory("logs")
{
}

LogManager::~LogManager()
{
    closeFileLogging();
}

void LogManager::setLogWidget(QTextEdit* logWidget)
{
    m_logWidget = logWidget;
    if (m_logWidget) {
        m_logWidget->setReadOnly(true);
        m_logWidget->setLineWrapMode(QTextEdit::NoWrap);
        // 初始化日志
        logInfo("程序启动成功");
    }
}

void LogManager::setFileLogging(bool enabled, const QString& logDir)
{
    QMutexLocker locker(&m_mutex);
    
    if (enabled == m_fileLoggingEnabled) {
        return;
    }
    
    m_fileLoggingEnabled = enabled;
    m_logDirectory = logDir;
    
    if (enabled) {
        initFileLogging();
    } else {
        closeFileLogging();
    }
}

void LogManager::setLogLevel(LogLevel level)
{
    m_currentLogLevel = level;
}

void LogManager::logInfo(const QString& message)
{
    log(Info, message);
}

void LogManager::logWarning(const QString& message)
{
    log(Warning, message);
}

void LogManager::logError(const QString& message)
{
    log(Error, message);
}

void LogManager::logDebug(const QString& message)
{
    log(Debug, message);
}

void LogManager::log(LogLevel level, const QString& message)
{
    // 检查日志级别
    if (level < m_currentLogLevel) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QString formattedMessage = formatMessage(level, message);
    
    // 写入UI控件
    writeToWidget(formattedMessage, level);
    
    // 写入文件
    if (m_fileLoggingEnabled) {
        writeToFile(formattedMessage);
    }
    
    // 发送信号
    emit logMessage(formattedMessage);
}

void LogManager::clearLog()
{
    if (m_logWidget) {
        m_logWidget->clear();
    }
}

QString LogManager::getLogContent() const
{
    if (m_logWidget) {
        return m_logWidget->toPlainText();
    }
    return QString();
}

QString LogManager::formatMessage(LogLevel level, const QString& message) const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString levelStr = levelToString(level);
    
    return QString("[%1] [%2] %3").arg(timestamp).arg(levelStr).arg(message);
}

QString LogManager::levelToString(LogLevel level) const
{
    switch (level) {
    case Info:    return "INFO";
    case Warning: return "WARN";
    case Error:   return "ERROR";
    case Debug:   return "DEBUG";
    default:      return "UNKNOWN";
    }
}

QString LogManager::levelToColor(LogLevel level) const
{
    switch (level) {
    case Info:    return "#ffffff";      // 白色
    case Warning: return "#ffeb3b";      // 黄色
    case Error:   return "#f44336";      // 红色
    case Debug:   return "#9e9e9e";      // 灰色
    default:      return "#ffffff";
    }
}

void LogManager::writeToWidget(const QString& formattedMessage, LogLevel level)
{
    if (!m_logWidget) {
        return;
    }
    
    // 设置文本颜色
    QString color = levelToColor(level);
    QString htmlMessage = QString("<span style=\"color: %1;\">%2</span>")
                          .arg(color)
                          .arg(formattedMessage.toHtmlEscaped());
    
    m_logWidget->append(htmlMessage);
    
    // 自动滚动到底部
    QTextCursor cursor = m_logWidget->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logWidget->setTextCursor(cursor);
    
    // 确保滚动条在底部
    QScrollBar* scrollBar = m_logWidget->verticalScrollBar();
    if (scrollBar) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void LogManager::writeToFile(const QString& formattedMessage)
{
    if (!m_logStream || !m_logFile || !m_logFile->isOpen()) {
        return;
    }
    
    *m_logStream << formattedMessage << Qt::endl;
    m_logStream->flush();
}

void LogManager::initFileLogging()
{
    // 创建日志目录
    QDir dir;
    if (!dir.exists(m_logDirectory)) {
        dir.mkpath(m_logDirectory);
    }
    
    // 关闭现有文件
    closeFileLogging();
    
    // 生成日志文件名
    QString fileName = generateLogFileName();
    QString fullPath = QDir(m_logDirectory).filePath(fileName);
    
    // 创建新的日志文件
    m_logFile = new QFile(fullPath, this);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_logStream = new QTextStream(m_logFile);
        m_logStream->setEncoding(QStringConverter::Utf8);
        
        // 写入启动信息
        QString startMessage = formatMessage(Info, "=== 日志记录开始 ===");
        writeToFile(startMessage);
    } else {
        delete m_logFile;
        m_logFile = nullptr;
        m_fileLoggingEnabled = false;
    }
}

void LogManager::closeFileLogging()
{
    if (m_logStream) {
        QString endMessage = formatMessage(Info, "=== 日志记录结束 ===");
        writeToFile(endMessage);
        
        delete m_logStream;
        m_logStream = nullptr;
    }
    
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
}

QString LogManager::generateLogFileName() const
{
    QString dateStr = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    return QString("MaglevControl_%1.log").arg(dateStr);
}