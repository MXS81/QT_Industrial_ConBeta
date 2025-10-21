# 多功能磁浮作业配置系统 - 项目开发指南

## 目录
- [项目概述](#项目概述)
- [开发环境搭建](#开发环境搭建)
- [项目结构详解](#项目结构详解)
- [核心技术解析](#核心技术解析)
- [开发流程指南](#开发流程指南)
- [测试与调试](#测试与调试)
- [部署发布](#部署发布)
- [维护与扩展](#维护与扩展)

## 项目概述

### 技术架构
- **框架**: Qt 5.12+ (跨平台GUI框架)
- **语言**: C++11/14 标准
- **通信**: Modbus TCP协议
- **构建**: qmake + MSVC/MinGW
- **平台**: Windows (7/8/10/11)

### 功能模块
```
磁浮控制系统
├── 连接管理模块 (Modbus TCP)
├── 单动子控制模块 (精密控制)
├── 多动子控制模块 (批量管理)
├── 参数配置模块 (实时调节)
├── 日志记录模块 (操作追踪)
└── 界面管理模块 (工业UI)
```

## 开发环境搭建

### 1. 基础环境要求
```bash
# 系统要求
OS: Windows 7/8/10/11 (x64)
RAM: 最少4GB，推荐8GB
Storage: 10GB可用空间
Network: 以太网接口

# 开发工具版本
Qt: 5.12.0 或更高版本
Qt Creator: 4.8.0+
编译器: MSVC 2017/2019/2022 或 MinGW 7.3+
```

### 2. Qt开发环境安装
```bash
# 下载Qt在线安装器
https://www.qt.io/download-qt-installer

# 必需的Qt组件
- Qt 5.12.x MSVC 2017 64-bit
- Qt Creator
- Qt Designer
- Qt SerialBus (重要：Modbus支持)
- Qt Charts (可选：数据可视化)
- Qt Quick (可选：现代UI)

# MinGW工具链（如果不使用MSVC）
- MinGW 7.3.0 64-bit
```

### 3. 编译器配置
```bash
# MSVC方式 (推荐)
# 安装Visual Studio 2017/2019/2022
# 确保包含C++工具和Windows SDK

# MinGW方式
# Qt安装器会自动配置MinGW
# 或单独下载MinGW-w64
```

### 4. 项目导入
```bash
# 克隆项目
git clone <repository-url>
cd MaglevControl

# 用Qt Creator打开
File -> Open File or Project -> 选择MaglevControl.pro

# 配置构建套件
Projects -> Build & Run -> 选择合适的Kit
```

## 项目结构详解

### 源码组织
```
MaglevControl/
├── main.cpp                    # 程序入口点
├── mainwindow.h                # 主窗口类声明
├── mainwindow.cpp              # 主窗口类实现
├── MaglevControl.pro           # Qt项目配置文件
├── MaglevControl.pro.user      # Qt Creator用户配置
├── mcs.ico                     # Windows应用图标
├── msvc_make.bat              # MSVC命令行编译脚本
├── Resource/                   # 资源文件目录
│   ├── Icon.qrc               # Qt资源配置
│   ├── mcs.png                # 应用图标PNG格式
│   └── style.qss              # 自定义样式表
├── build/                     # 构建输出目录
│   ├── debug/                 # 调试版本
│   └── release/               # 发布版本
├── README.md                  # 项目说明文档
└── PROJECT_GUIDE.md           # 本开发指南
```

### 核心文件解析

#### MaglevControl.pro
```qmake
QT += core gui serialbus widgets  # Qt模块依赖
TARGET = MagControl               # 生成的可执行文件名
TEMPLATE = app                    # 应用程序模板

# Windows平台特定配置
win32 {
    RC_ICONS = mcs.ico           # Windows图标
    msvc: QMAKE_CXXFLAGS += /utf-8  # MSVC UTF-8支持
}

# 源文件和头文件
SOURCES += main.cpp mainwindow.cpp
HEADERS += mainwindow.h
RESOURCES += Resource/Icon.qrc   # 嵌入式资源
```

#### main.cpp 分析
```cpp
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 中文显示支持
    // QApplication::setApplicationName("Modbus上位机");
    
    MainWindow w;
    w.setAttribute(Qt::WA_QuitOnClose, true);  // 关闭时退出
    
    // 设置应用图标
    QIcon windowIcon(":/mcs.png");
    w.setWindowIcon(windowIcon);
    w.setWindowTitle("多功能磁浮作业配置系统");
    
    w.show();
    return a.exec();
}
```

## 核心技术解析

### 1. Modbus TCP通信实现

#### 连接管理
```cpp
// MainWindow构造函数中
modbusClient = new QModbusTcpClient(this);

// 状态变化监听
connect(modbusClient, &QModbusDevice::stateChanged,
        this, &MainWindow::onModbusStateChanged);
```

#### 心跳机制
```cpp
// 30秒心跳防止连接断开
QTimer *heartbeatTimer = new QTimer(this);
heartbeatTimer->setInterval(30000);
connect(heartbeatTimer, &QTimer::timeout, this, [=]() {
    if (modbusClient->state() == QModbusClient::ConnectedState) {
        QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, 0, 1);
        auto *reply = modbusClient->sendReadRequest(readUnit, 1);
        if (reply) {
            connect(reply, &QModbusReply::finished, 
                   reply, &QModbusReply::deleteLater);
        }
    }
});
```

#### 寄存器读写
```cpp
// 写寄存器
bool MainWindow::writeModbusRegister(int address, quint16 value) {
    if (modbusClient->state() != QModbusClient::ConnectedState) {
        logMessage("错误：Modbus未连接，无法写入数据");
        return false;
    }
    
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, address, 1);
    writeUnit.setValue(0, value);
    
    auto *reply = modbusClient->sendWriteRequest(writeUnit, 1);
    if (!reply) {
        logMessage(QString("错误：写入地址 %1 失败").arg(address));
        return false;
    }
    
    connect(reply, &QModbusReply::finished, this, [=]() {
        if (reply->error() == QModbusDevice::NoError) {
            logMessage(QString("成功：写入地址 %1 值 %2").arg(address).arg(value));
        } else {
            logMessage(QString("错误：%1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });
    
    return true;
}
```

### 2. 数据结构设计

#### 单动子参数结构
```cpp
struct SingleAxisParams {
    bool enable;             // 使能状态 (0=禁用, 1=使能)
    bool allowManual;        // 手动控制权限 (0=禁止, 1=允许)
    bool runMode;           // 运行模式 (0=自动, 1=手动)
    
    qint32 autoSpeed;       // 自动模式速度 (rpm)
    bool clickOKBtn;        // 自动模式确认按钮
    
    qint32 jogSpeed;        // 手动Jog速度 (rpm)
    qint32 jogPosition;     // 手动Jog位置 (mm)
    short jog;              // Jog方向 (0=停止, 1=左, 2=右)
};
```

#### 多动子参数结构
```cpp
struct MultiAxisParams {
    bool enable;           // 使能状态
    qint32 autoSpeed;      // 自动速度
};
```

### 3. UI界面架构

#### 主界面布局
```cpp
void MainWindow::initUI() {
    // 创建中央窗口
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // 添加各功能区域
    mainLayout->addWidget(createConnectionArea());    // 连接设置
    
    // 水平布局：单动子 + 多动子
    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->addWidget(createSingleAxisArea());
    controlLayout->addWidget(createMultiAxisArea());
    mainLayout->addLayout(controlLayout);
    
    mainLayout->addWidget(createLogArea());          // 日志区域
}
```

#### 工业风格样式
```cpp
void MainWindow::setupDarkIndustrialStyles() {
    // 加载QSS样式文件或内嵌样式
    QString styleSheet = R"(
        QMainWindow {
            background-color: #2b2b2b;
            color: #ffffff;
        }
        
        QPushButton {
            background-color: #404040;
            border: 2px solid #606060;
            border-radius: 5px;
            color: #ffffff;
            padding: 8px 16px;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: #505050;
            border-color: #808080;
        }
        
        QPushButton:pressed {
            background-color: #353535;
        }
        
        /* 使能按钮特殊样式 */
        QPushButton[objectName="enableButton"] {
            background-color: #d32f2f;
        }
        
        QPushButton[objectName="enableButton"]:checked {
            background-color: #388e3c;
        }
    )";
    
    setStyleSheet(styleSheet);
}
```

## 开发流程指南

### 1. 功能开发流程

#### 新增控制功能步骤
```cpp
// 1. 在mainwindow.h中声明槽函数
private slots:
    void onNewFeatureClicked();

// 2. 在mainwindow.h中声明UI控件
private:
    QPushButton *newFeatureButton;

// 3. 在UI创建函数中添加控件
QPushButton *newFeatureButton = new QPushButton("新功能", this);
layout->addWidget(newFeatureButton);

// 4. 连接信号和槽
connect(newFeatureButton, &QPushButton::clicked,
        this, &MainWindow::onNewFeatureClicked);

// 5. 实现槽函数
void MainWindow::onNewFeatureClicked() {
    // 功能实现
    logMessage("执行新功能");
    
    // 如果需要Modbus通信
    writeModbusRegister(address, value);
}
```

#### 参数配置扩展
```cpp
// 1. 扩展参数结构体
struct ExtendedParams {
    SingleAxisParams single;
    MultiAxisParams multi;
    // 新增参数
    qint32 accelerationRate;    // 加速度
    qint32 decelerationRate;    // 减速度
    bool safetyLimit;           // 安全限制
};

// 2. 添加对应的UI控件
QSpinBox *accelerationSpinBox;
QSpinBox *decelerationSpinBox;
QCheckBox *safetyCheckBox;

// 3. 实现参数同步函数
void MainWindow::syncExtendedParams() {
    // UI -> 参数结构
    extParams.accelerationRate = accelerationSpinBox->value();
    extParams.decelerationRate = decelerationSpinBox->value();
    extParams.safetyLimit = safetyCheckBox->isChecked();
    
    // 参数结构 -> Modbus寄存器
    writeModbusRegister(ACCEL_ADDR, extParams.accelerationRate);
    writeModbusRegister(DECEL_ADDR, extParams.decelerationRate);
    writeModbusRegister(SAFETY_ADDR, extParams.safetyLimit ? 1 : 0);
}
```

### 2. 调试技巧

#### Modbus通信调试
```cpp
// 启用详细日志
void MainWindow::enableModbusDebug() {
    // 连接所有Modbus信号用于调试
    connect(modbusClient, &QModbusDevice::errorOccurred,
            this, [=](QModbusDevice::Error error) {
        logMessage(QString("Modbus错误: %1").arg(error));
    });
    
    connect(modbusClient, &QModbusDevice::stateChanged,
            this, [=](QModbusDevice::State state) {
        QString stateStr;
        switch(state) {
            case QModbusDevice::UnconnectedState:
                stateStr = "未连接"; break;
            case QModbusDevice::ConnectingState:
                stateStr = "连接中"; break;
            case QModbusDevice::ConnectedState:
                stateStr = "已连接"; break;
            case QModbusDevice::ClosingState:
                stateStr = "断开中"; break;
        }
        logMessage(QString("连接状态: %1").arg(stateStr));
    });
}
```

#### UI调试工具
```cpp
// 添加调试面板
QWidget* MainWindow::createDebugPanel() {
    QGroupBox *debugGroup = new QGroupBox("调试面板", this);
    QVBoxLayout *debugLayout = new QVBoxLayout(debugGroup);
    
    // 寄存器直接读写
    QHBoxLayout *regLayout = new QHBoxLayout();
    QSpinBox *regAddrSpin = new QSpinBox();
    regAddrSpin->setRange(0, 65535);
    QSpinBox *regValueSpin = new QSpinBox();
    regValueSpin->setRange(0, 65535);
    
    QPushButton *readBtn = new QPushButton("读取");
    QPushButton *writeBtn = new QPushButton("写入");
    
    regLayout->addWidget(new QLabel("地址:"));
    regLayout->addWidget(regAddrSpin);
    regLayout->addWidget(new QLabel("值:"));
    regLayout->addWidget(regValueSpin);
    regLayout->addWidget(readBtn);
    regLayout->addWidget(writeBtn);
    
    debugLayout->addLayout(regLayout);
    
    // 连接调试按钮
    connect(readBtn, &QPushButton::clicked, [=]() {
        readModbusRegister(regAddrSpin->value());
    });
    
    connect(writeBtn, &QPushButton::clicked, [=]() {
        writeModbusRegister(regAddrSpin->value(), regValueSpin->value());
    });
    
    return debugGroup;
}
```

### 3. 性能优化

#### UI响应优化
```cpp
// 使用定时器批量更新UI
void MainWindow::setupUIUpdateTimer() {
    QTimer *uiTimer = new QTimer(this);
    uiTimer->setInterval(100);  // 100ms更新一次
    
    connect(uiTimer, &QTimer::timeout, this, [=]() {
        // 批量更新UI状态
        updateConnectionStatus();
        updateControlStatus();
        updateParameterDisplay();
    });
    
    uiTimer->start();
}
```

#### 内存管理
```cpp
// 正确的对象生命周期管理
MainWindow::~MainWindow() {
    // Qt父子关系会自动清理UI对象
    // 但需要手动清理一些资源
    
    if (modbusClient && modbusClient->state() != QModbusDevice::UnconnectedState) {
        modbusClient->disconnectDevice();
    }
    
    // 清理定时器（通常不需要，父对象会处理）
    // heartbeatTimer->stop();  // 如果timer不是this的子对象
}
```

## 测试与调试

### 1. 单元测试框架
```cpp
// 创建测试项目 MaglevControlTests
// tests/test_modbus.cpp
#include <QtTest>
#include "../mainwindow.h"

class TestModbus : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testConnection();
    void testRegisterWrite();
    void testRegisterRead();
    void cleanupTestCase();

private:
    MainWindow *mainWindow;
};

void TestModbus::initTestCase() {
    mainWindow = new MainWindow();
}

void TestModbus::testConnection() {
    // 模拟连接测试
    QVERIFY(mainWindow != nullptr);
    // 更多测试逻辑
}

void TestModbus::cleanupTestCase() {
    delete mainWindow;
}

QTEST_MAIN(TestModbus)
#include "test_modbus.moc"
```

### 2. 集成测试
```bash
# 创建测试批处理文件 run_tests.bat
@echo off
echo 开始运行集成测试...

echo 测试1: 基本功能测试
MagControl.exe --test-basic

echo 测试2: Modbus通信测试  
MagControl.exe --test-modbus

echo 测试3: UI交互测试
MagControl.exe --test-ui

echo 所有测试完成
pause
```

### 3. 模拟器开发
```cpp
// 创建Modbus设备模拟器用于测试
class ModbusSimulator : public QObject {
    Q_OBJECT
    
public:
    ModbusSimulator(QObject *parent = nullptr);
    void startServer(int port = 9000);
    
private slots:
    void handleModbusRequest();
    
private:
    QTcpServer *tcpServer;
    QHash<int, quint16> registers;  // 模拟寄存器
};

void ModbusSimulator::startServer(int port) {
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection,
            this, &ModbusSimulator::handleModbusRequest);
    
    if (tcpServer->listen(QHostAddress::Any, port)) {
        qDebug() << "Modbus模拟器启动在端口:" << port;
    }
}
```

## 部署发布

### 1. 构建配置

#### 发布版本构建
```bash
# 命令行构建
qmake MaglevControl.pro -config release
nmake  # 或 mingw32-make

# 或使用批处理文件
msvc_make.bat
```

#### 依赖库收集
```bash
# 使用Qt部署工具
windeployqt.exe --qmldir . MagControl.exe

# 手动收集依赖（如果需要）
copy "%QTDIR%\bin\Qt5Core.dll" .\
copy "%QTDIR%\bin\Qt5Gui.dll" .\
copy "%QTDIR%\bin\Qt5Widgets.dll" .\
copy "%QTDIR%\bin\Qt5SerialBus.dll" .\
```

### 2. 安装包制作

#### 使用NSIS创建安装包
```nsis
; installer.nsi
!define APP_NAME "多功能磁浮作业配置系统"
!define APP_VERSION "1.0.0"
!define APP_PUBLISHER "YourCompany"
!define APP_EXECUTABLE "MagControl.exe"

; 安装路径
InstallDir "$PROGRAMFILES\${APP_NAME}"

; 安装文件
Section "MainSection" SEC01
    SetOutPath "$INSTDIR"
    File "MagControl.exe"
    File /r "platforms\"
    File /r "imageformats\"
    File "*.dll"
    
    ; 创建开始菜单快捷方式
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXECUTABLE}"
    
    ; 创建桌面快捷方式
    CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXECUTABLE}"
    
    ; 注册表项
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" "$INSTDIR\uninstall.exe"
    
    ; 创建卸载程序
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd
```

#### 使用Inno Setup
```pascal
; setup.iss
[Setup]
AppName=多功能磁浮作业配置系统
AppVersion=1.0.0
AppPublisher=YourCompany
AppPublisherURL=http://www.yourcompany.com
DefaultDirName={pf}\MaglevControl
DefaultGroupName=磁浮控制系统
AllowNoIcons=yes
OutputDir=installer
OutputBaseFilename=MaglevControl_Setup_v1.0.0
Compression=lzma
SolidCompression=yes

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Files]
Source: "MagControl.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\磁浮控制系统"; Filename: "{app}\MagControl.exe"
Name: "{commondesktop}\磁浮控制系统"; Filename: "{app}\MagControl.exe"

[Run]
Filename: "{app}\MagControl.exe"; Description: "启动磁浮控制系统"; Flags: nowait postinstall skipifsilent
```

## 维护与扩展

### 1. 版本管理策略

#### Git工作流
```bash
# 主要分支
main        # 稳定发布版本
develop     # 开发集成分支
feature/*   # 功能开发分支
hotfix/*    # 紧急修复分支
release/*   # 发布准备分支

# 功能开发流程
git checkout develop
git checkout -b feature/new-axis-control
# 开发新功能...
git add .
git commit -m "feat: 添加新轴控制功能"
git checkout develop
git merge feature/new-axis-control
```

#### 版本号规则
```
v主版本.次版本.修订版本-构建号
例如: v1.2.3-build20240315
```

### 2. 代码规范

#### 命名约定
```cpp
// 类名：大驼峰命名
class MainWindow;
class ModbusController;

// 函数名：小驼峰命名
void onConnectButtonClicked();
void writeModbusRegister();

// 变量名：小驼峰命名
QModbusTcpClient *modbusClient;
QPushButton *connectButton;

// 常量：全大写+下划线
const int MAX_SPEED = 100000;
const int MIN_SPEED = 10000;

// 成员变量：m_前缀
int m_currentSpeed;
bool m_isConnected;

// 枚举：大写+下划线
enum AxisState {
    AXIS_DISABLED,
    AXIS_ENABLED,
    AXIS_RUNNING
};
```

#### 文档注释
```cpp
/**
 * @brief 写入Modbus寄存器
 * @param address 寄存器地址 (0-65535)
 * @param value 要写入的值 (0-65535)
 * @return 成功返回true，失败返回false
 * 
 * 示例用法：
 * @code
 * bool success = writeModbusRegister(0x0001, 50000);
 * if (!success) {
 *     logMessage("写入失败");
 * }
 * @endcode
 */
bool writeModbusRegister(int address, quint16 value);
```

### 3. 性能监控

#### 添加性能统计
```cpp
class PerformanceMonitor : public QObject {
    Q_OBJECT
    
public:
    static PerformanceMonitor* instance();
    void recordOperation(const QString &operation, qint64 duration);
    void generateReport();
    
private:
    QHash<QString, QList<qint64>> operationTimes;
    static PerformanceMonitor* m_instance;
};

// 使用示例
void MainWindow::onConnectButtonClicked() {
    QElapsedTimer timer;
    timer.start();
    
    // 执行连接操作
    modbusClient->connectDevice();
    
    qint64 elapsed = timer.elapsed();
    PerformanceMonitor::instance()->recordOperation("ModbusConnect", elapsed);
}
```

### 4. 错误处理增强

#### 统一错误处理系统
```cpp
enum ErrorCode {
    ERROR_NONE = 0,
    ERROR_MODBUS_CONNECTION = 1001,
    ERROR_MODBUS_TIMEOUT = 1002,
    ERROR_INVALID_PARAMETER = 2001,
    ERROR_DEVICE_NOT_RESPONSE = 3001
};

class ErrorHandler : public QObject {
    Q_OBJECT
    
public:
    static void handleError(ErrorCode code, const QString &detail = "");
    static QString getErrorMessage(ErrorCode code);
    
signals:
    void errorOccurred(ErrorCode code, const QString &message);
};

// 使用示例
if (!modbusClient->connectDevice()) {
    ErrorHandler::handleError(ERROR_MODBUS_CONNECTION, 
                             QString("无法连接到%1:%2").arg(ip).arg(port));
}
```

---

## 版本历史

### v1.2.0 (当前版本)
- 新增完整的配方管理系统
- 实现RecipeManager和RecipeWidget模块
- 支持8寄存器工位配方标准化写入
- 优化心跳机制为10秒间隔
- 新增掩码写操作(功能码0x16)
- 实现32位数据高低字处理
- 添加BaseController基类架构
- 完善错误处理和日志系统

### v1.1.0
- 重构模块化架构
- 添加StyleManager和LogManager
- 优化Modbus通信稳定性
- 改进工业化UI设计

### v1.0.0
- 首次发布
- 实现基本的单动子和多动子控制功能
- 支持Modbus TCP通信
- 提供工业风格用户界面

---

## 总结

这份开发指南涵盖了磁浮控制系统项目的完整开发流程，从环境搭建到部署维护。开发者可以根据具体需求调整和扩展相关内容。

主要要点：
- 严格遵循Qt开发规范和模块化架构
- 重视Modbus通信的稳定性和10秒心跳机制
- 保持工业级软件的可靠性和配方管理标准化
- 持续优化用户体验和多模块协同工作
- 做好文档和版本管理，更新PLC对接规范

如有技术问题，请参考Qt官方文档、本项目的技术文档或联系项目维护团队。