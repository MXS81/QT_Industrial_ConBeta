#include "LogWidget.h"
#include <QRegularExpression>

LogWidget::LogWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_maxLines(1000)
    , m_autoScroll(true)
    , m_showTimestamp(true)
    , m_showUser(true)
    , m_currentLineCount(0)
    , m_title(title)
{
    setupUI();
    setupStyles();
}

LogWidget::~LogWidget()
{
}

void LogWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(5);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    // 创建分组框
    m_groupBox = new QGroupBox(m_title);
    m_mainLayout->addWidget(m_groupBox);

    QVBoxLayout *groupLayout = new QVBoxLayout(m_groupBox);

    // 日志显示区域
    m_logDisplay = new QTextEdit();
    m_logDisplay->setReadOnly(true);
    groupLayout->addWidget(m_logDisplay);

    // 按钮布局
    m_buttonLayout = new QHBoxLayout();

    m_clearBtn = new QPushButton("清空日志");
    m_exportBtn = new QPushButton("导出日志");

    m_buttonLayout->addWidget(m_clearBtn);
    m_buttonLayout->addWidget(m_exportBtn);
    m_buttonLayout->addStretch();

    groupLayout->addLayout(m_buttonLayout);

    // 连接信号
    connect(m_clearBtn, &QPushButton::clicked, this, &LogWidget::onClearLog);
    connect(m_exportBtn, &QPushButton::clicked, this, &LogWidget::onExportLog);
}

void LogWidget::setupStyles()
{
    // 默认样式
    QString groupBoxStyle = R"(
        QGroupBox {
            font-size: 14px;
            font-weight: bold;
            color: #e94560;
            border: 2px solid #3a3a5e;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 20px;
            padding: 0 10px 0 10px;
        }
    )";

    QString logDisplayStyle = R"(
        QTextEdit {
            background-color: #0a0e27;
            color: #00ff41;
            font-family: 'Consolas', monospace;
            font-size: 11px;
            border: 1px solid #3a3a5e;
        }
    )";

    QString buttonStyle = R"(
        QPushButton {
            background-color: #533483;
            color: white;
            border: none;
            padding: 5px 10px;
            font-size: 11px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: #e94560;
        }
    )";

    m_groupBox->setStyleSheet(groupBoxStyle);
    m_logDisplay->setStyleSheet(logDisplayStyle);
    m_clearBtn->setStyleSheet(buttonStyle);
    m_exportBtn->setStyleSheet(buttonStyle);
}

void LogWidget::addLogEntry(const QString &message, LogLevel level, const QString &user)
{
    QString formattedEntry = formatLogEntry(message, level, user);

    // 添加到缓存
    m_logEntries.append(formattedEntry);

    // 显示日志
    m_logDisplay->append(formattedEntry);
    m_currentLineCount++;

    // 自动滚动到底部
    if (m_autoScroll) {
        m_logDisplay->moveCursor(QTextCursor::End);
    }

    // 检查是否需要裁剪
    trimLogIfNeeded();
}

void LogWidget::addLogEntry(const QString &message, const QString &type, const QString &user)
{
    LogLevel level = stringToLevel(type);
    addLogEntry(message, level, user);
}

QString LogWidget::formatLogEntry(const QString &message, LogLevel level, const QString &user)
{
    QString timestamp = m_showTimestamp ?
                            QDateTime::currentDateTime().toString("hh:mm:ss") : "";

    QString levelColor = getLevelColor(level);
    QString levelStr = getLevelString(level);

    QString formattedEntry;

    if (m_showTimestamp && m_showUser && !user.isEmpty()) {
        formattedEntry = QString("<span style='color: #888;'>[%1]</span> "
                                 "<span style='color: #3b82f6;'>[%2]</span> "
                                 "<span style='color: %3;'>[%4]</span> "
                                 "<span style='color: %3;'>%5</span>")
                             .arg(timestamp,
                                 user,
                                 levelColor,
                                 levelStr,
                                 message);
    } else if (m_showTimestamp) {
        formattedEntry = QString("<span style='color: #888;'>[%1]</span> "
                                 "<span style='color: %2;'>[%3]</span> "
                                 "<span style='color: %2;'>%4</span>")
                             .arg(timestamp,
                             levelColor,
                             levelStr,
                             message);
    } else {
        formattedEntry = QString("<span style='color: %1;'>[%2]</span> "
                                 "<span style='color: %1;'>%3</span>")
                             .arg(levelColor,
                                 levelStr,
                                 message);
    }

    return formattedEntry;
}

QString LogWidget::getLevelColor(LogLevel level)
{
    switch (level) {
    case Info:    return "#3b82f6";
    case Success: return "#22c55e";
    case Warning: return "#fbbf24";
    case Error:   return "#ef4444";
    default:      return "#00ff41";
    }
}

QString LogWidget::getLevelString(LogLevel level)
{
    switch (level) {
    case Info:    return "信息";
    case Success: return "成功";
    case Warning: return "警告";
    case Error:   return "错误";
    default:      return "信息";
    }
}

LogWidget::LogLevel LogWidget::stringToLevel(const QString &type)
{
    if (type == "success") return Success;
    else if (type == "warning") return Warning;
    else if (type == "error") return Error;
    else return Info;
}

void LogWidget::trimLogIfNeeded()
{
    if (m_currentLineCount > m_maxLines) {
        // 移除前面的日志条目
        int linesToRemove = m_currentLineCount - m_maxLines;
        for (int i = 0; i < linesToRemove && !m_logEntries.isEmpty(); ++i) {
            m_logEntries.removeFirst();
        }

        // 重新构建显示
        m_logDisplay->clear();
        for (const QString &entry : static_cast<const QStringList&>(m_logEntries)) {
            m_logDisplay->append(entry);
        }

        m_currentLineCount = m_logEntries.size();

        emit logLimitReached(m_maxLines);
    }
}

void LogWidget::clearLog()
{
    m_logDisplay->clear();
    m_logEntries.clear();
    m_currentLineCount = 0;
    emit logCleared();
}

void LogWidget::exportLog()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "导出日志文件",
                                                    QString("log_%1.txt")
                                                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
                                                    "文本文件 (*.txt);;所有文件 (*.*)");

    if (!fileName.isEmpty()) {
        if (saveLogToFile(fileName)) {
            QMessageBox::information(this, "导出成功", QString("日志已成功导出到:\n%1").arg(fileName));
            emit logExported(fileName);
        } else {
            QMessageBox::warning(this, "导出失败", "无法保存日志文件！");
        }
    }
}

bool LogWidget::saveLogToFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // 写入头部信息
    out << QString("=== %1 日志导出 ===\n").arg(m_title);
    out << QString("导出时间: %1\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    out << QString("日志条目: %1 条\n").arg(m_logEntries.size());
    out << "======================================\n\n";

    // 写入日志内容（去除HTML标签）
    for (const QString &entry : static_cast<const QStringList&>(m_logEntries)) {
        QString plainText = entry;
        static const QRegularExpression regExp("<[^>]*>");
        plainText.remove(regExp);
        out << plainText << "\n";
    }

    file.close();
    return true;
}

// 公共接口实现
void LogWidget::setMaxLines(int maxLines)
{
    m_maxLines = maxLines;
    trimLogIfNeeded();
}

void LogWidget::setAutoScroll(bool autoScroll)
{
    m_autoScroll = autoScroll;
}

void LogWidget::setShowTimestamp(bool showTimestamp)
{
    m_showTimestamp = showTimestamp;
}

void LogWidget::setShowUser(bool showUser)
{
    m_showUser = showUser;
}

void LogWidget::setGroupBoxStyle(const QString &style)
{
    m_groupBox->setStyleSheet(style);
}

QString LogWidget::getLogContent() const
{
    return m_logDisplay->toPlainText();
}

int LogWidget::getLogCount() const
{
    return m_currentLineCount;
}

// 槽函数实现
void LogWidget::onClearLog()
{
    if (m_currentLineCount > 0) {
        int ret = QMessageBox::question(this, "确认清空",
                                        "确定要清空所有日志吗？",
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            clearLog();
            addLogEntry("日志已清空", Info);
        }
    }
}

void LogWidget::onExportLog()
{
    if (m_currentLineCount == 0) {
        QMessageBox::information(this, "提示", "当前没有日志可以导出！");
        return;
    }
    exportLog();
}
