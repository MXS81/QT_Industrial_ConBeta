#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QGroupBox>
#include <QSettings>
#include <QCloseEvent>

class LogWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow();

    // 获取日志显示控件
    QTextEdit* getLogAllWidget() const { return m_logAll; }
    QTextEdit* getLogInfoWidget() const { return m_logInfo; }
    QTextEdit* getLogWarnWidget() const { return m_logWarn; }
    QTextEdit* getLogErrorWidget() const { return m_logError; }

public slots:
    void clearAllLogs();

signals:
    void windowClosed();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onClearLogClicked();

private:
    void setupUI();
    void setupLogTabs();
    QHBoxLayout* createToolBar();
    void saveWindowState();
    void restoreWindowState();
    
    // 布局工具方法
    QHBoxLayout* createStandardHBoxLayout();
    QVBoxLayout* createStandardVBoxLayout();

    // UI组件
    QTabWidget *m_logTabs;
    QTextEdit *m_logAll;
    QTextEdit *m_logInfo; 
    QTextEdit *m_logWarn;
    QTextEdit *m_logError;
    
    QToolButton *m_clearLogBtn;
    QGroupBox *m_mainGroup;
};

#endif // LOGWINDOW_H