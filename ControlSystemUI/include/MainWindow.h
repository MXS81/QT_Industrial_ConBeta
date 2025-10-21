#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QMutex>
#include "MoverData.h"
#include "LogWidget.h"
#include "ModbusConfigDialog.h"

struct ModbusConfig;
class ModbusManager;
class QTabWidget;
class QPushButton;
class QLabel;
class QTimer;
class QStatusBar;
class OverviewPage;
class JogControlPage;
class RecipeManagerPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 公共方法供其他页面访问
    ModbusManager* getModbusManager() const { return m_modbusManager; }
    bool isSimulationMode() const;
    void onMoverCountChanged(int count);

    // 提供全局日志接口
    LogWidget* getGlobalLogWidget() const { return m_globalLogWidget; }
    void addGlobalLogEntry(const QString &message, const QString &type = "info");
    void logToGlobal(const QString &message, const QString &type = "info", const QString &source = "");
    void addLogEntry(const QString &message, const QString &type = "info");
    void onModbusError(const QString &error);

signals:
    void moversUpdated(const QList<MoverData>& movers);
    void userChanged(const QString& username);
    void currentRecipeChanged(int id, const QString &name);
    void emergencyStopTriggered();
    void emergencyStopReset();
    void moverCountChanged(int count);   // 动子数量变化信号

private slots:
    void updateSystemStatus();
    void onEmergencyStop();
    void onSystemInit();
    void openJogControl();
    void openRecipeManager();
    void onLoginLogout();
    void onTabChanged(int index);
    void onUserLoginSuccess(const QString& username);
    void onRecipeApplied(int id, const QString &name);
    void openSystemLog();

    // Modbus相关槽函数
    void onModbusConnected();
    void onModbusDisconnected();
    void openModbusConfig();
    void onModbusConnectButtonClicked();

    void onModbusDataReceived(int startAddress, const QVector<quint16> &data);
    void onModbusCoilsReceived(int startAddress, const QVector<bool> &data);
    void onSystemStatusChanged(bool initialized, bool enabled);

private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void initializeMovers();
    void updateUIPermissions();
    bool showLoginDialog();
    void onEmergencyStopFromPLC();

    void processMoversStatusData(int startAddress, const QVector<quint16> &data);
    void updateMoverStatus(MoverData &mover, quint16 statusWord);
    void updateMoverError(MoverData &mover, quint16 errorCode);
    void processSystemStatusData(int startAddress, const QVector<quint16> &data);
    void applyModbusConfig(const ModbusConfig &config);
    void saveModbusConfigToSettings(const ModbusConfig &config);
    ModbusConfig loadModbusConfigFromSettings();
    void initializeModbusFromSettings();
    bool m_isTestingConnection = false;

    // 动子配置管理
    struct MoverConfig {
        int count = 1;
        double trackLength = 7455.75;
        double safetyDistance = 100.0;
        bool enableCollisionDetection = true;
    };

    MoverConfig m_moverConfig;

    void loadMoverConfig();
    void saveMoverConfig();
    void applyMoverConfig(const MoverConfig &config);

    // 全局日志组件 - 确保在这里声明
    LogWidget *m_globalLogWidget;

    // 初始化更新
    void initializeModbus();

    //动子数量
    int getMoverCount() const;
    void setMoverCount(int count);

    //急停相关
    bool m_isEmergencyStopPressed;
    void triggerEmergencyStop();
    void resetEmergencyStop();
    void updateEmergencyButtonState(bool isPressed);

    // 页面
    QTabWidget *m_tabWidget;
    OverviewPage *m_overviewPage;
    JogControlPage *m_jogPage;
    RecipeManagerPage *m_recipePage;

    // 数据
    QList<MoverData> m_movers;
    QTimer *m_updateTimer;

    // 用户相关
    bool m_isLoggedIn;
    QString m_currentUser;

    // 状态栏组件
    int m_currentRecipeId = -1;
    QString m_currentRecipeName = "无";
    QLabel *m_statusLabel;
    QLabel *m_timeLabel;
    QLabel *m_userLabel;
    QPushButton *m_loginLogoutButton;
    QPushButton *m_emergencyButton;

    // Modbus相关成员
    ModbusManager *m_modbusManager;
    bool m_modbusConnected;
    QLabel *m_modbusStatusLabel;
    QPushButton *m_modbusConnectBtn;

    // 系统状态管理
    QMutex m_dataUpdateMutex;
    bool m_systemReady;
    bool m_emergencyStopActive;

};

#endif // MAINWINDOW_H
