/********************************************************************************
* Copyright (h) 2025, 沈阳盛科祝融科技有限公司
* Date: 2025-8-28
* Description: 全局参数设定
*********************************************************************************/
#ifndef GlobalParameterSetting_H
#define GlobalParameterSetting_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFormLayout>
#include <QGroupBox>
#include <QValidator>
#include <QGroupBox>

class GlobalParameterSetting : public QWidget
{
    Q_OBJECT

public:
    GlobalParameterSetting(QWidget *parent = nullptr);
    ~GlobalParameterSetting() override;

private slots:
    void saveParameters();

private:
    // 左侧参数
    QComboBox *workflowDirection;
    QLineEdit *motorTotalCount;
    QLineEdit *motorSafeDistance;
    QLineEdit *ferry1MotorPos;
    QLineEdit *ferry2MotorPos;
    QLineEdit *ferry1EntryPos;
    QLineEdit *ferry2EntryPos;
    QLineEdit *maglevToBeltPos;
    QLineEdit *beltToMaglevPos;
    QLineEdit *rfidPos;

    // 右侧参数
    QLineEdit *jogSpeed;
    QLineEdit *manualSpeed;
    QLineEdit *autoSpeed;
    QLineEdit *acceleration;
    QLineEdit *deceleration;

    // 按钮
    QPushButton *saveButton;

    // 验证器：仅用QIntValidator，限定0~65535（16位无符号整数范围）
    QIntValidator *uint16Validator;

    void setupUI();
    void createLeftPanel(QWidget *parent);
    void createRightPanel(QWidget *parent);
    void loadParameters();

signals:
    // 定义发送消息的信号
    void sendMessageToMainWindow(const QString &msg);
};

#endif // GlobalParameterSetting_H
