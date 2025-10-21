/********************************************************************************
* Copyright (h) 2025, 沈阳盛科祝融科技有限公司
* Date: 2025-8-28
* Description: 全局控制面板
*********************************************************************************/
#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <cstdint>

#include "TrackWidget.h"

// 前向声明
class ModbusManager;

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = nullptr);
    ~ControlPanel() override;

    // 赋值和获取当前寄存器值
    void setRegisterValue(uint16_t registerVal);
    uint16_t getRegisterValue() const;
    
    // 设置Modbus管理器引用
    void setModbusManager(ModbusManager* modbusManager);
    
    // 连接状态管理
    void updateConnectionState(bool connected);

private slots:
    // 按钮点击处理函数
    void onResetClicked();
    void onStartClicked();
    void onPauseClicked();
    void onEmergencyStopClicked();
    void onInitialClicked();
    void onManualModeClicked();
    void onAutoModeClicked();

private:
    // 16位寄存器
    uint16_t m_register;

    // 按钮状态跟踪（用于上升沿检测）
    bool m_resetState;
    bool m_startState;
    bool m_pauseState;
    bool m_eStopState;
    bool m_initialState;
    bool m_manualModeState;
    bool m_autoModeState;

    // 按钮控件
    QPushButton *m_btnReset;
    QPushButton *m_btnStart;
    QPushButton *m_btnPause;
    QPushButton *m_btnEStop;
    QPushButton *m_btnInitial;
    QPushButton *m_btnManualMode;
    QPushButton *m_btnAutoMode;

    // === 可视化UI组件 ===
    TrackWidget *m_trackWidget;          // 轨道可视化控件

    // === Modbus连接管理 ===
    ModbusManager* m_modbusManager;      // Modbus管理器引用
    bool m_isConnected;                  // 连接状态标志

    // 初始化UI
    void initUI();
    
    // 检查连接状态并处理未连接情况
    bool checkConnection();

    // 处理上升沿信号
    void handleRisingEdge(int bitPosition, bool &currentState);

    // 更新按钮状态
    void updateButtonStates();

signals:
    // 定义发送消息的信号
    void sendMessageToMainWindow(const uint16_t &registerVal);
    // 发送操作描述信号
    void sendOperationMessage(const QString &operationMsg);
};

#endif // ControlPanel_H
