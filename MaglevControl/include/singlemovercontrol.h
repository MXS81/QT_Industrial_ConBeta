/********************************************************************************
* Copyright (h) 2025, 沈阳盛科祝融科技有限公司
* Date: 2025-8-12
* Description: 单动子控制
*********************************************************************************/
#ifndef SINGLEMOVERCONTROL_H
#define SINGLEMOVERCONTROL_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QSpinBox>

class SingleMoverControl : public QWidget
{
    Q_OBJECT
public:
    explicit SingleMoverControl(QWidget *parent = nullptr);


private:
    // 单动子区域控件
    QPushButton *singleEnableButton;
    QComboBox *modeComboBox;
    QCheckBox *allowManualCheckBox;
    QStackedWidget *paramStackedWidget;

    // 手动模式参数
    QSpinBox *jogSpeedSpin;
    QSpinBox *jogPositionSpin;
    // 自动模式参数
    QSpinBox *autoSpeedSpin;


    // 初始化UI
    void initUI();

signals:
    void readModbusRegisterToMainWindow(int address);
    void writeModbusRegisterToMainWindow(int address, const uint16_t &registerVal);


private slots:
    // 单动子相关槽函数
    void onSingleEnableClicked();  // 使能设定
    void onAllowManualControlClicked(bool checked);  // 是否允许手动控制
    void onSingleAutoRunClicked(bool isRun);  //自动模式下开始运行
    void onSingleJogLeftPressed();  //JogLeft
    void onSingleJogLeftReleased();
    void onSingleJogRightPressed();  //JogRight
    void onSingleJogRightReleased();

    void onModeChanged(int index);
    void onJogSpeedChanged(int value);
    void onJogPositionChanged(int value);
    void onAutoSpeedChanged(int value);

};

#endif // SINGLEMOVERCONTROL_H
