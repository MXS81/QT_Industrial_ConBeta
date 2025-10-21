#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QTimer>

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    enum LogLevel {
        Info,
        Success,
        Warning,
        Error
    };

    explicit LogWidget(const QString &title = "日志", QWidget *parent = nullptr);
    ~LogWidget();

    // 添加日志条目
    void addLogEntry(const QString &message, LogLevel level = Info, const QString &user = "");
    void addLogEntry(const QString &message, const QString &type = "info", const QString &user = "");

    // 日志管理
    void clearLog();
    void exportLog();
    bool saveLogToFile(const QString &fileName);

    // 设置相关
    void setMaxLines(int maxLines);
    void setAutoScroll(bool autoScroll);
    void setShowTimestamp(bool showTimestamp);
    void setShowUser(bool showUser);
    void setGroupBoxStyle(const QString &style);

    // 获取日志内容
    QString getLogContent() const;
    int getLogCount() const;

signals:
    void logCleared();
    void logExported(const QString &fileName);
    void logLimitReached(int maxLines);

public slots:
    void onClearLog();
    void onExportLog();

private:
    void setupUI();
    void setupStyles();
    QString formatLogEntry(const QString &message, LogLevel level, const QString &user);
    QString getLevelColor(LogLevel level);
    QString getLevelString(LogLevel level);
    LogLevel stringToLevel(const QString &type);
    void trimLogIfNeeded();

    // UI组件
    QGroupBox *m_groupBox;
    QTextEdit *m_logDisplay;
    QPushButton *m_clearBtn;
    QPushButton *m_exportBtn;
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_buttonLayout;

    // 配置参数
    int m_maxLines;
    bool m_autoScroll;
    bool m_showTimestamp;
    bool m_showUser;
    int m_currentLineCount;
    QString m_title;

    // 缓存
    QStringList m_logEntries;
};

#endif // LOGWIDGET_H
