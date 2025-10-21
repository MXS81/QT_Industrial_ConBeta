#include "ConnectionStatusWidget.h"
#include <QVBoxLayout>

ConnectionStatusWidget::ConnectionStatusWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentMode(Disconnected)
    , m_blinkState(false)
{
    setupUI();

    // 设置闪烁定时器
    m_blinkTimer = new QTimer(this);
    connect(m_blinkTimer, &QTimer::timeout, this, &ConnectionStatusWidget::updateBlinkState);
    m_blinkTimer->start(1000); // 1秒闪烁
}

void ConnectionStatusWidget::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 5, 10, 5);
    mainLayout->setSpacing(10);

    // 状态图标
    m_statusIcon = new QLabel();
    m_statusIcon->setFixedSize(16, 16);
    m_statusIcon->setStyleSheet(R"(
        QLabel {
            border-radius: 8px;
            background-color: #ef4444;
        }
    )");

    // 状态文本
    m_statusText = new QLabel("未连接");
    m_statusText->setStyleSheet("color: white; font-weight: bold; font-size: 13px;");

    // 信息文本
    m_infoText = new QLabel("模拟模式");
    m_infoText->setStyleSheet("color: #888; font-size: 11px;");

    // 连接按钮
    m_connectionBtn = new QPushButton("连接PLC");
    m_connectionBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #22c55e;
            color: white;
            border: none;
            padding: 6px 12px;
            border-radius: 4px;
            font-size: 11px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #16a34a;
        }
        QPushButton:pressed {
            background-color: #15803d;
        }
    )");

    connect(m_connectionBtn, &QPushButton::clicked,
            this, &ConnectionStatusWidget::onConnectionButtonClicked);

    // 组装布局
    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);
    textLayout->addWidget(m_statusText);
    textLayout->addWidget(m_infoText);

    mainLayout->addWidget(m_statusIcon);
    mainLayout->addLayout(textLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(m_connectionBtn);

    // 设置背景样式
    setStyleSheet(R"(
        ConnectionStatusWidget {
            background-color: #0f3460;
            border: 1px solid #3a3a5e;
            border-radius: 6px;
        }
    )");

    updateDisplay();
}

void ConnectionStatusWidget::setConnectionMode(ConnectionMode mode)
{
    if (m_currentMode != mode) {
        m_currentMode = mode;
        updateDisplay();
    }
}

void ConnectionStatusWidget::setConnectionInfo(const QString &info)
{
    m_connectionInfo = info;
    updateDisplay();
}

void ConnectionStatusWidget::updateDisplay()
{
    switch (m_currentMode) {
    case Disconnected:
        m_statusIcon->setStyleSheet(R"(
            QLabel {
                border-radius: 8px;
                background-color: #ef4444;
            }
        )");
        m_statusText->setText("未连接");
        m_statusText->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 13px;");
        m_infoText->setText("点击连接PLC或使用模拟模式");
        m_connectionBtn->setText("连接PLC");
        m_connectionBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #22c55e;
                color: white;
                border: none;
                padding: 6px 12px;
                border-radius: 4px;
                font-size: 11px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #16a34a;
            }
        )");
        break;

    case Simulation:
        m_statusIcon->setStyleSheet(R"(
            QLabel {
                border-radius: 8px;
                background-color: #fbbf24;
            }
        )");
        m_statusText->setText("模拟模式");
        m_statusText->setStyleSheet("color: #fbbf24; font-weight: bold; font-size: 13px;");
        m_infoText->setText("本地模拟运行 - 数据非实时");
        m_connectionBtn->setText("尝试连接");
        break;

    case PLCConnected:
        m_statusIcon->setStyleSheet(R"(
            QLabel {
                border-radius: 8px;
                background-color: #22c55e;
            }
        )");
        m_statusText->setText("PLC已连接");
        m_statusText->setStyleSheet("color: #22c55e; font-weight: bold; font-size: 13px;");
        m_infoText->setText(m_connectionInfo.isEmpty() ? "实时控制模式" : m_connectionInfo);
        m_connectionBtn->setText("断开连接");
        m_connectionBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #ef4444;
                color: white;
                border: none;
                padding: 6px 12px;
                border-radius: 4px;
                font-size: 11px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #dc2626;
            }
        )");
        break;
    }
}

void ConnectionStatusWidget::onConnectionButtonClicked()
{
    if (m_currentMode == PLCConnected) {
        emit disconnectRequested();
    } else {
        emit connectRequested();
    }
}

void ConnectionStatusWidget::updateBlinkState()
{
    if (m_currentMode == Simulation) {
        m_blinkState = !m_blinkState;
        QString color = m_blinkState ? "#fbbf24" : "#f59e0b";
        m_statusIcon->setStyleSheet(QString(R"(
            QLabel {
                border-radius: 8px;
                background-color: %1;
            }
        )").arg(color));
    }
}
