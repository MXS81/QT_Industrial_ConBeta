// ModbusConfigDialog.h
#ifndef MODBUSCONFIGDIALOG_H
#define MODBUSCONFIGDIALOG_H

#include <QDialog>

// 前置声明，避免不必要的头文件包含
class QLineEdit;
class QSpinBox;
class QComboBox;
class QPushButton;
class QGroupBox;
class QRadioButton;
class MainWindow;

/**
 * @brief Modbus连接配置信息结构体
 *
 * 用于存储和传递Modbus连接所需的所有参数。
 * 提供了默认值，方便初始化。
 */
struct ModbusConfig {
    // 连接类型枚举
    enum ConnectionType {
        TCP,    // TCP/IP模式
        Serial  // 串口模式
    };

    ConnectionType type;

    // TCP/IP 参数
    QString host;
    int port;

    // 串口参数
    QString serialPort;
    int baudRate;
    int dataBits;
    int stopBits;
    int parity;

    // 通用参数
    int deviceId; // 从站地址
    int timeout;  // 超时时间 (ms)
    int retries;  // 重试次数

    // 默认构造函数，提供一套常用的初始配置
    ModbusConfig() {
        type = TCP;
        host = "192.168.5.5";
        port = 502;
        serialPort = "COM1";
        baudRate = 9600;
        dataBits = 8;
        stopBits = 1;
        parity = 0; // 0: NoParity
        deviceId = 1;
        timeout = 3000;
        retries = 3;
    }
};

/**
 * @brief Modbus连接配置对话框
 *
 * 提供一个图形化界面，用于设置、保存和加载Modbus连接参数。
 * 支持TCP和串口两种连接方式。
 */
class ModbusConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ModbusConfigDialog(const ModbusConfig &config = ModbusConfig(), QWidget *parent = nullptr);

    // 公共接口，用于获取用户在对话框中配置的最终参数
    ModbusConfig getConfig() const;

signals:
    // 用于向主窗口发送日志消息的信号
    void logMessage(const QString &message, const QString &type);

private slots:
    // --- UI交互槽函数 ---
    void onConnectionTypeChanged(); // 切换TCP/串口模式时更新UI
    void onLoadProfile();           // 从文件加载配置
    void onSaveProfile();           // 将当前配置保存到文件

private:
    // --- 内部辅助函数 ---
    void setupUI();                 // 初始化和布局UI控件
    void setConfig(const ModbusConfig &config); // 将传入的配置数据显示到UI上
    void loadSettings();            // 从QSettings (注册表/配置文件) 加载上一次的配置
    void saveSettings();            // 将当前UI上的配置保存到QSettings
    bool validateConfig(const ModbusConfig &config, QString &errorMessage); // 验证配置的有效性
    void logToSystem(const QString &message, const QString &type = "info"); // 记录日志
    MainWindow* findMainWindow();   // 查找主窗口实例

    // --- UI控件成员变量 ---
    QRadioButton *m_tcpRadio;
    QRadioButton *m_serialRadio;

    // TCP设置
    QGroupBox *m_tcpGroup;
    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpin;

    // 串口设置
    QGroupBox *m_serialGroup;
    QComboBox *m_serialPortCombo;
    QComboBox *m_baudRateCombo;
    QComboBox *m_dataBitsCombo;
    QComboBox *m_stopBitsCombo;
    QComboBox *m_parityCombo;

    // 通用设置
    QSpinBox *m_deviceIdSpin;
    QSpinBox *m_timeoutSpin;
    QSpinBox *m_retriesSpin;

    // 按钮
    QPushButton *m_loadBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;

    // --- 数据成员 ---
    ModbusConfig m_config;      // 当前配置
    MainWindow *m_mainWindow;   // 指向主窗口的指针，用于日志记录
};

#endif // MODBUSCONFIGDIALOG_H
