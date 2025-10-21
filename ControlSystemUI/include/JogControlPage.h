// JogControlPage.h
#ifndef JOGCONTROLPAGE_H
#define JOGCONTROLPAGE_H

#include <QWidget>
#include <QList>
#include <QTimer>
#include "ModbusManager.h"
#include "MoverData.h"
#include "LogWidget.h"
#include "MainWindow.h"

class TrackWidget;
class QScrollArea;
class QComboBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;


/**
 * @brief JOG手动控制与单动子控制页面
 *
 * 提供了对单个动子的详细手动控制功能，包括：
 * - 使能/禁用
 * - JOG点动（短按/长按连续移动）
 * - 绝对位置移动
 * - 自动往复运行
 * 同时集成了轨道可视化和操作日志。
 */
class JogControlPage : public QWidget
{
    Q_OBJECT

public:
    explicit JogControlPage(QList<MoverData> *movers, const QString& currentUser, ModbusManager *modbusManager, QWidget *parent = nullptr);
    ~JogControlPage();

public slots:
    // 从主窗口接收动子数据的更新
    void updateMovers(const QList<MoverData> &movers);
    // 响应主窗口的急停信号
    void onEmergencyStopTriggered();
    void onEmergencyStopReset();
    // 更新动子选择下拉框
    void updateMoverSelector();

private slots:
    // --- UI交互槽函数 ---
    void onMoverSelectionChanged(int index);
    void onGoToPosition();
    void onStopMover();
    void onEnableStateChanged(bool enabled);

    // --- 实时数据更新 ---
    void onRealTimeDataUpdate();

    // --- JOG控制 (长按与短按) ---
    void onJogForwardPressed();
    void onJogForwardReleased();
    void onJogBackwardPressed();
    void onJogBackwardReleased();
    void performContinuousJog();

    // --- 自动运行控制 ---
    void onAutoRunModeToggled(bool enabled);
    void onAutoRunStart();
    void onAutoRunStop();
    void onAutoRunPause();
    void performAutoRunStep();

private:
    // --- 内部辅助函数 ---
    void setupUI();
    void updateMoverInfo();
    void setControlsEnabled(bool enabled);
    void addLogEntry(const QString &message, const QString &type = "info");
    bool checkCollision(int moverId, double targetPosition);
    double normalizePosition(double position);
    double calculateShortestDistance(double pos1, double pos2);

    // JOG控制逻辑
    void startLongPressDetection(bool isForward);
    void stopLongPressDetection();
    void startContinuousJog();

    // 实时更新控制
    void startRealTimeUpdates();
    void stopRealTimeUpdates();
    void updateEnableStatusDisplay(bool enabled);

    // 自动运行UI
    void setupAutoRunUI();
    void sendAutoRunCommandToPLC();
    void connectAutoRunSignals();

    // --- UI组件 ---
    QComboBox *m_moverSelector;
    QGroupBox *m_enableGroup;
    QGroupBox *m_currentStatusGroup;
    QLabel *m_enableStatusLabel;
    QLabel *m_positionLabel;
    QLabel *m_speedLabel;
    QLabel *m_statusLabel;
    QLabel *m_userLabel;
    QPushButton *m_enableBtn;
    QPushButton *m_disableBtn;
    QDoubleSpinBox *m_targetPosSpinBox;
    QSpinBox *m_speedSpinBox;
    QPushButton *m_goToBtn;
    QPushButton *m_stopBtn;
    QSpinBox *m_jogStepSpinBox;
    QPushButton *m_jogForwardBtn;
    QPushButton *m_jogBackwardBtn;
    TrackWidget *m_trackWidget;
    LogWidget *m_logWidget;
    QScrollArea *m_trackScrollArea;

    // 自动运行UI组件
    QGroupBox *m_autoRunGroup;
    QCheckBox *m_autoRunEnableCheckBox;
    QPushButton *m_autoRunStartBtn;
    QPushButton *m_autoRunPauseBtn;
    QPushButton *m_autoRunStopBtn;

    // --- 核心数据成员 ---
    QList<MoverData> *m_movers; // 指向主窗口维护的动子数据列表
    int m_selectedMover;        // 当前选中的动子索引
    QString m_currentUser;      // 当前操作员用户名
    ModbusManager *m_modbusManager; // Modbus通信管理器实例
    MainWindow *m_mainWindow;       // 主窗口指针

    // --- 状态与定时器 ---
    QTimer *m_realTimeUpdateTimer;
    bool m_isRealTimeEnabled;
    bool m_emergencyActive;

    // JOG长按相关
    QTimer *m_longPressTimer;
    QTimer *m_continuousJogTimer;
    bool m_isLongPressing;
    bool m_isContinuousJogging;
    bool m_jogDirection; // true=前进, false=后退

    // 自动运行相关
    QTimer *m_autoRunTimer;
    bool m_isAutoRunActive;
    bool m_isAutoRunPaused;

    // --- 静态常量 ---
    static const int LONG_PRESS_THRESHOLD = 500;  // 长按阈值 (ms)
    static const int CONTINUOUS_JOG_INTERVAL = 100; // 连续JOG间隔 (ms)
    static const int AUTO_RUN_INTERVAL = 1000;      // 自动运行更新间隔(ms)
    static const double TRACK_LENGTH;
    static const double SAFETY_DISTANCE;
};

#endif // JOGCONTROLPAGE_H
