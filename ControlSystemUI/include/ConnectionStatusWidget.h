#ifndef CONNECTIONSTATUSWIDGET_H
#define CONNECTIONSTATUSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTimer>

class ConnectionStatusWidget : public QWidget
{
    Q_OBJECT

public:
    enum ConnectionMode {
        Disconnected,
        Simulation,
        PLCConnected
    };

    explicit ConnectionStatusWidget(QWidget *parent = nullptr);

    void setConnectionMode(ConnectionMode mode);
    void setConnectionInfo(const QString &info);

signals:
    void connectRequested();
    void disconnectRequested();

private slots:
    void onConnectionButtonClicked();
    void updateBlinkState();

private:
    void setupUI();
    void updateDisplay();

    ConnectionMode m_currentMode;
    QString m_connectionInfo;

    QLabel *m_statusIcon;
    QLabel *m_statusText;
    QLabel *m_infoText;
    QPushButton *m_connectionBtn;
    QTimer *m_blinkTimer;
    bool m_blinkState;
};

#endif // CONNECTIONSTATUSWIDGET_H
