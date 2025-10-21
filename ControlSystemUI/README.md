# ControlSystemUI - 工业控制系统用户界面

基于Qt框架开发的现代化工业控制系统用户界面模块，提供直观、稳定的人机交互体验。

## 📊 模块概述

ControlSystemUI是一个功能完善的工业控制系统前端界面，专为工业自动化环境设计。它提供了用户友好的操作界面、实时数据监控、设备状态管理等核心功能。

## ✨ 核心功能

### 🔐 用户管理系统
- **多级权限控制** - 管理员/操作员/观察者权限分级
- **安全登录验证** - 用户身份认证和会话管理
- **操作日志记录** - 完整的用户操作追踪

### 📡 设备通信管理
- **Modbus通信支持** - RTU/TCP协议兼容
- **连接状态监控** - 实时显示设备连接状态
- **通信参数配置** - 灵活的通信参数设置界面

### 🎮 运动控制界面
- **手动控制模式** - 精确的设备手动操作
- **位置监控显示** - 实时位置和状态反馈
- **安全联锁保护** - 多重安全机制防护

### 📈 数据监控与日志
- **实时数据显示** - 关键参数实时监控
- **历史数据查询** - 数据趋势分析和查询
- **智能日志管理** - 分级日志记录和过滤

### 🎨 用户界面设计
- **现代化UI风格** - 符合工业标准的界面设计
- **响应式布局** - 适配不同分辨率显示设备
- **主题定制支持** - 可配置的界面主题和样式

## 🏗️ 技术架构

### 开发框架
- **Qt 5.12+** - 跨平台GUI框架
- **C++17** - 现代C++标准
- **CMake** - 跨平台构建系统

### 设计模式
- **MVC架构** - 模型-视图-控制器分离
- **信号槽机制** - Qt原生的事件处理
- **单例模式** - 样式和日志管理器

### 关键组件
- **StyleManager** - 统一的样式管理系统
- **ModbusManager** - Modbus通信管理器
- **LogWidget** - 智能日志显示组件
- **ConnectionStatusWidget** - 连接状态指示器

## 📁 项目结构

```
ControlSystemUI/
├── include/                    # 头文件目录
│   ├── MainWindow.h           # 主窗口类
│   ├── LoginDialog.h          # 登录对话框
│   ├── ModbusManager.h        # Modbus通信管理
│   ├── ModbusConfigDialog.h   # 通信配置对话框
│   ├── JogControlPage.h       # 手动控制页面
│   ├── LogWidget.h            # 日志显示组件
│   ├── ConnectionStatusWidget.h # 连接状态组件
│   ├── StyleManager.h         # 样式管理器
│   ├── StyleUtils.h           # 样式工具函数
│   ├── AnimatedButton.h       # 动画按钮组件
│   ├── Trackwidget.h          # 轨道监控组件
│   ├── Moverwidget.h          # 移动器监控组件
│   ├── overviewpage.h         # 总览页面
│   ├── recipemanagerpage.h    # 配方管理页面
│   ├── MoverData.h            # 移动器数据结构
│   └── DebugHelper.h          # 调试辅助工具
├── src/                       # 源代码目录
│   ├── Main.cpp              # 程序入口点
│   ├── MainWindow.cpp        # 主窗口实现
│   ├── logindialog.cpp       # 登录对话框实现
│   ├── ModbusManager.cpp     # Modbus通信实现
│   ├── ModbusConfigDialog.cpp # 通信配置实现
│   ├── JogControlPage.cpp    # 手动控制实现
│   ├── LogWidget.cpp         # 日志组件实现
│   ├── ConnectionStatusWidget.cpp # 连接状态实现
│   ├── StyleManager.cpp      # 样式管理实现
│   ├── StyleUtils.cpp        # 样式工具实现
│   ├── AnimatedButton.cpp    # 动画按钮实现
│   ├── Trackwidget.cpp       # 轨道监控实现
│   ├── moverwidget.cpp       # 移动器监控实现
│   ├── overviewpage.cpp      # 总览页面实现
│   ├── recipemanagerpage.cpp # 配方管理实现
│   └── DebugHelper.cpp       # 调试辅助实现
├── CMakeLists.txt            # CMake构建配置
├── Mainwindow.ui            # Qt Designer界面文件
├── ControlSystemUI_zh_CN.ts # 中文翻译文件
└── README.md                # 本文档
```

## 🚀 编译指南

### 环境要求
- Qt 5.12 或更高版本
- CMake 3.16+
- C++17兼容编译器 (GCC 7+, MSVC 2017+, Clang 5+)

### 编译步骤

#### Windows (Visual Studio)
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

#### Linux / macOS
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### Qt Creator
1. 打开Qt Creator
2. File → Open File or Project
3. 选择`CMakeLists.txt`文件
4. 配置构建套件
5. 构建项目

## ⚙️ 配置说明

### Modbus通信配置
```cpp
// 串口配置示例
ModbusManager* manager = ModbusManager::getInstance();
manager->setSerialPort("COM1", 9600, QSerialPort::NoParity);
manager->setSlaveAddress(1);
```

### 样式主题配置
```cpp
// 应用自定义主题
StyleManager* styleManager = StyleManager::getInstance();
styleManager->applyTheme("industrial_blue");
```

### 日志级别配置
```cpp
// 设置日志级别
LogWidget* logWidget = new LogWidget();
logWidget->setLogLevel(LogWidget::INFO);
```

## 🎯 使用示例

### 基本启动流程
```cpp
#include <QApplication>
#include "MainWindow.h"
#include "LoginDialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 显示登录对话框
    LoginDialog loginDialog;
    if (loginDialog.exec() == QDialog::Accepted) {
        // 启动主界面
        MainWindow mainWindow;
        mainWindow.show();
        return app.exec();
    }
    
    return 0;
}
```

### 添加自定义页面
```cpp
// 在主窗口中添加新的功能页面
void MainWindow::addCustomPage()
{
    QWidget* customPage = new QWidget();
    // 设计自定义页面布局
    ui->stackedWidget->addWidget(customPage);
}
```

## 🔧 自定义扩展

### 添加新的通信协议
1. 继承基础通信管理器
2. 实现协议特定的通信方法
3. 注册到ModbusManager中

### 自定义UI组件
1. 继承相应的Qt基础组件
2. 实现自定义绘制和交互逻辑
3. 应用统一的样式管理

### 扩展日志功能
1. 实现自定义日志处理器
2. 集成到LogWidget组件中
3. 配置日志输出格式和级别

## 📋 功能特性

- ✅ 多语言支持 (中文/英文)
- ✅ 响应式UI设计
- ✅ 实时数据更新
- ✅ 安全权限控制
- ✅ 模块化架构设计
- ✅ 易于扩展和维护
- ✅ 完整的错误处理
- ✅ 丰富的调试工具

## 🐛 故障排除

### 常见问题

**Q: 编译时提示找不到Qt库**
A: 确保Qt安装路径正确配置在CMAKE_PREFIX_PATH中

**Q: Modbus连接失败**
A: 检查串口权限和通信参数配置

**Q: 界面显示异常**
A: 验证样式表文件是否正确加载

### 调试模式
```cpp
// 启用调试模式
#define DEBUG_MODE
#include "DebugHelper.h"

DebugHelper::enableVerboseLogging();
```

## 🤝 贡献说明

欢迎贡献代码和提出改进建议！

1. 遵循现有的代码风格和命名规范
2. 添加必要的单元测试
3. 更新相关文档
4. 提交Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](../LICENSE) 文件

---

🔗 **相关链接**
- [主项目仓库](https://github.com/MXS81/QT_Industrial_ConBeta)
- [MaglevControl模块](../MaglevControl/)
- [项目Wiki](https://github.com/MXS81/QT_Industrial_ConBeta/wiki)