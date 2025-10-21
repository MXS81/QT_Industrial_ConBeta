#include "logwindow.h"
#include <QApplication>
#include <QStyle>
#include <QDateTime>
#include <QScreen>


LogWindow::LogWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_logTabs(nullptr)
    , m_logAll(nullptr)
    , m_logInfo(nullptr)
    , m_logWarn(nullptr)
    , m_logError(nullptr)
    , m_clearLogBtn(nullptr)
    , m_mainGroup(nullptr)
{
    setWindowTitle(QStringLiteral("操作日志"));
    setWindowIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    
    // 设置窗口属性，确保可以正常移动
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose, false);
    
    setupUI();
    restoreWindowState();
}


LogWindow::~LogWindow()
{
    saveWindowState();
}


void LogWindow::setupUI()
{
    // 创建中央控件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 主布局
    QVBoxLayout *mainLayout = createStandardVBoxLayout();
    centralWidget->setLayout(mainLayout);
    
    // 创建主组件
    m_mainGroup = new QGroupBox(QStringLiteral("操作日志"), this);
    QVBoxLayout *groupLayout = createStandardVBoxLayout();
    m_mainGroup->setLayout(groupLayout);
    groupLayout->setContentsMargins(10, 10, 10, 10);
    
    // 创建工具栏
    groupLayout->addLayout(createToolBar());
    
    // 创建日志标签页
    setupLogTabs();
    groupLayout->addWidget(m_logTabs);
    
    mainLayout->addWidget(m_mainGroup);
}


void LogWindow::setupLogTabs()
{
    m_logTabs = new QTabWidget(this);
    
    // 创建QTextEdit的lambda函数
    auto makeTextEdit = [this]() {
        auto *textEdit = new QTextEdit(this);
        textEdit->setReadOnly(true);
        textEdit->setLineWrapMode(QTextEdit::NoWrap);
        textEdit->setFont(QFont("Consolas", 9));
        return textEdit;
    };
    
    // 创建各个标签页
    m_logAll = makeTextEdit();
    m_logInfo = makeTextEdit(); 
    m_logWarn = makeTextEdit();
    m_logError = makeTextEdit();
    
    m_logTabs->addTab(m_logAll, QStringLiteral("全部"));
    m_logTabs->addTab(m_logInfo, QStringLiteral("消息"));
    m_logTabs->addTab(m_logWarn, QStringLiteral("警告"));
    m_logTabs->addTab(m_logError, QStringLiteral("错误"));
}


QHBoxLayout* LogWindow::createToolBar()
{
    QHBoxLayout *toolbar = createStandardHBoxLayout();
    toolbar->addStretch();
    
    // 清空按钮
    m_clearLogBtn = new QToolButton(this);
    m_clearLogBtn->setAutoRaise(true);
    m_clearLogBtn->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    m_clearLogBtn->setToolTip(QStringLiteral("清空日志"));
    m_clearLogBtn->setObjectName("clearLogBtn");
    toolbar->addWidget(m_clearLogBtn);
    
    // 连接信号槽
    connect(m_clearLogBtn, &QToolButton::clicked, this, &LogWindow::onClearLogClicked);
    
    return toolbar;
}


void LogWindow::clearAllLogs()
{
    if (m_logAll) m_logAll->clear();
    if (m_logInfo) m_logInfo->clear();
    if (m_logWarn) m_logWarn->clear();
    if (m_logError) m_logError->clear();
}


void LogWindow::onClearLogClicked()
{
    clearAllLogs();
    if (m_logAll) {
        m_logAll->append(QString("<span style=\"color: #ffffff;\">[%1] [INFO] 日志已清空</span>")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")));
    }
}


void LogWindow::closeEvent(QCloseEvent *event)
{
    saveWindowState();
    emit windowClosed();
    event->accept();
}


void LogWindow::saveWindowState()
{
    QSettings settings;
    settings.beginGroup("LogWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.endGroup();
}


void LogWindow::restoreWindowState()
{
    QSettings settings;
    settings.beginGroup("LogWindow");
    
    // 设置默认大小
    resize(800, 600);
    
    // 恢复窗口几何信息
    QByteArray geometry = settings.value("geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    } else {
        // 默认位置：居中显示在屏幕中央
        QRect screenGeometry = QApplication::primaryScreen()->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
    
    // 恢复窗口状态
    QByteArray state = settings.value("windowState").toByteArray();
    if (!state.isEmpty()) {
        restoreState(state);
    }
    
    settings.endGroup();
}


QHBoxLayout* LogWindow::createStandardHBoxLayout()
{
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    return layout;
}


QVBoxLayout* LogWindow::createStandardVBoxLayout()
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    return layout;
}