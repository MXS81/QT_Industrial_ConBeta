# PLC对接技术说明文档

## 概述

本文档详细说明磁浮控制系统上位机与PLC对接的关键技术要点，包括网络通信、寄存器映射、数据格式等核心内容。

## 1. 网络通信配置

### 1.1 通信协议
- **协议类型**: Modbus TCP over Ethernet
- **默认设置**: 
  - PLC IP地址: `192.168.5.5`
  - 端口: `502` (标准Modbus TCP端口)
  - 心跳间隔: 30秒

### 1.2 代码位置
```cpp
// 文件: modbusmanager.cpp:31-32
m_modbusClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, QVariant(ip));
m_modbusClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, QVariant(port));
```

### 1.3 连接状态管理
- **连接检测**: 实时监控连接状态变化
- **心跳机制**: 定期读取寄存器0保持连接活跃
- **错误处理**: 详细的错误分类和异常代码处理

## 2. 32位数据高低字节顺序处理

### 2.1 数据拆分规则
```cpp
// 文件: singleaxiscontroller.cpp:228-236
void SingleAxisController::writeSpeedToRegisters(int speed, int lowReg, int highReg)
{
    // 拆分32位速度值为高低16位
    quint16 high16 = static_cast<quint16>((speed >> 16) & 0xFFFF);
    quint16 low16 = static_cast<quint16>(speed & 0xFFFF);
    
    m_modbusManager->writeRegister(lowReg, low16);   // 先写低16位
    m_modbusManager->writeRegister(highReg, high16); // 后写高16位
}
```

### 2.2 寄存器分配
| 数据类型 | 低16位寄存器 | 高16位寄存器 | 说明 |
|----------|-------------|-------------|------|
| 自动速度 | 0x0001 | 0x0002 | auto speed |
| Jog速度 | 0x0004 | 0x0005 | jog speed |

### 2.3 数据示例
假设速度值为 `80000` (0x13880):
- `low16 = 0x3880` → 写入寄存器 0x0001
- `high16 = 0x0001` → 写入寄存器 0x0002

**PLC读取**: 需要按 `(high16 << 16) | low16` 重组为完整32位值

## 3. 控制寄存器位定义

### 3.1 寄存器地址
- **控制寄存器**: 0x0000 (CONTROL_REG)

### 3.2 位定义对照表
| 位编号 | 位名称 | 功能说明 | 代码常量 |
|--------|--------|----------|----------|
| bit0 | 使能位 | 0=禁用, 1=使能 | BIT_ENABLE |
| bit1 | 运行模式 | 0=点动模式, 1=自动模式 | BIT_MODE |
| bit2 | 手动权限 | 0=禁止手动, 1=允许手动 | BIT_MANUAL_ALLOW |
| bit3 | Jog左 | 0=不动作, 1=左移 | BIT_JOG_LEFT |
| bit4 | Jog右 | 0=不动作, 1=右移 | BIT_JOG_RIGHT |
| bit5 | 自动运行 | 0=停止, 1=运行 | BIT_AUTO_RUN |
| bit6-15 | 保留 | 保留位，暂未使用 | - |

### 3.3 位操作实现
```cpp
// 文件: singleaxiscontroller.cpp:206-226
bool SingleAxisController::setBit(int bitPosition, bool value)
{
    readControlRegister();
    
    quint16 newValue = m_controlRegister;
    const quint16 mask = 1 << bitPosition;
    
    if (value) {
        newValue |= mask;      // 设置位为1
    } else {
        newValue &= ~mask;     // 清除位为0
    }
    
    bool result = m_modbusManager->writeRegister(CONTROL_REG, newValue);
    
    if (result) {
        m_controlRegister = newValue;
    }
    
    return result;
}
```

### 3.4 控制寄存器状态示例
**寄存器值**: 0x0035 (二进制: 0000 0000 0011 0101)

- bit0=1: 使能开启
- bit1=0: 点动模式
- bit2=1: 允许手动控制
- bit3=0: 不按左Jog
- bit4=1: 按右Jog
- bit5=1: 自动运行中

## 4. 配方数据8个寄存器布局

### 4.1 总体架构
- **工位总数寄存器**: 0x0023 (寄存器35)
- **配方数据基址**: 0x0024 (寄存器36)
- **每工位占用**: 8个连续寄存器
- **工位N基址计算**: `0x0024 + N * 8`

### 4.2 寄存器详细布局

| 偏移 | 寄存器地址 | 数据内容 | 数据格式 | 字节分配 |
|------|------------|----------|----------|----------|
| 0 | BASE_ADDR + 0 | 工位号 + 任务ID | 8位+8位打包 | 低8位=工位号, 高8位=任务ID |
| 1 | BASE_ADDR + 1 | 路段号 + 目标工位号 | 8位+8位打包 | 低8位=路段号, 高8位=目标工位号 |
| 2 | BASE_ADDR + 2 | 路段位置 | 16位无符号整数 | 完整16位值 |
| 3 | BASE_ADDR + 3 | 路段速度 | 16位无符号整数 | 完整16位值 |
| 4 | BASE_ADDR + 4 | 起始位置 | 16位无符号整数 | 完整16位值 |
| 5 | BASE_ADDR + 5 | 终止位置 | 16位无符号整数 | 完整16位值 |
| 6 | BASE_ADDR + 6 | 到位延时 | 16位无符号整数 | 完整16位值(ms) |
| 7 | BASE_ADDR + 7 | 摆渡位置 + 工位屏蔽 | 8位+8位打包 | 低8位=摆渡位置, 高8位=工位屏蔽 |

### 4.3 8位数据打包算法
```cpp
// 文件: recipemanager.cpp:491-500
// 打包：两个8位值合并为一个16位寄存器
quint16 packTwoBytes(quint8 lowByte, quint8 highByte) const
{
    return (static_cast<quint16>(highByte) << 8) | static_cast<quint16>(lowByte);
}

// 解包：从16位寄存器提取两个8位值
void unpackTwoBytes(quint16 value, quint8& lowByte, quint8& highByte) const
{
    lowByte = static_cast<quint8>(value & 0xFF);        // 低8位
    highByte = static_cast<quint8>((value >> 8) & 0xFF); // 高8位
}
```

### 4.4 实际写入示例

#### 工位1配方数据写入
```cpp
// 基址计算: 0x0024 + 0 * 8 = 0x0024
int baseAddr = 0x0024;

// 寄存器0: 工位号=1, 任务ID=3 (焊接工艺)
quint16 reg0 = packTwoBytes(1, 3);  // 结果: 0x0301
writeRegister(baseAddr + 0, reg0);

// 寄存器1: 路段号=1, 目标工位=2  
quint16 reg1 = packTwoBytes(1, 2);  // 结果: 0x0201
writeRegister(baseAddr + 1, reg1);

// 寄存器2-6: 直接16位值
writeRegister(baseAddr + 2, 188);   // 路段位置
writeRegister(baseAddr + 3, 1000);  // 路段速度
writeRegister(baseAddr + 4, 0);     // 起始位置
writeRegister(baseAddr + 5, 100);   // 终止位置
writeRegister(baseAddr + 6, 500);   // 到位延时

// 寄存器7: 摆渡位置=1, 工位屏蔽=0
quint16 reg7 = packTwoBytes(1, 0);  // 结果: 0x0001
writeRegister(baseAddr + 7, reg7);
```

#### 工位2配方数据写入
```cpp
// 基址计算: 0x0024 + 1 * 8 = 0x002C
int baseAddr = 0x002C;
// ... 按相同格式写入工位2的8个寄存器
```

### 4.5 任务ID枚举定义
```cpp
enum class ProcessType : quint16 {
    Winding = 1,    // 绕线
    Gluing = 2,     // 点胶  
    Welding = 3,    // 焊接
    Molding = 4,    // 注塑
    Unloading = 5   // 下料
};
```

### 4.6 摆渡位置枚举定义
```cpp
enum class FerryPosition : quint16 {
    None = 0,       // 无摆渡
    Pos1 = 1,       // 位置1
    Pos2 = 2        // 位置2 (扩展用)
};
```

## 5. PLC对接验证要点

### 5.1 网络层验证
- [ ] 确认PLC的实际IP地址和端口
- [ ] 验证网络连通性（ping测试）
- [ ] 确认Modbus TCP服务是否启用

### 5.2 寄存器映射验证
- [ ] 确认单轴控制寄存器地址(0x0000-0x0005)
- [ ] 验证32位数据的高低字组合方式
- [ ] 确认控制寄存器位定义一致性

### 5.3 配方数据验证
- [ ] 确认配方基址从寄存器36开始
- [ ] 验证每工位8个寄存器的分配
- [ ] 确认8位数据打包的字节顺序
- [ ] 测试工位总数寄存器(35号)的读写

### 5.4 数据格式验证
- [ ] 确认所有16位数据的字节序(大端/小端)
- [ ] 验证枚举值的数值对应关系
- [ ] 测试边界值和异常值的处理

## 6. 调试建议

### 6.1 分步测试策略
1. **连接测试**: 先确保基本的Modbus TCP连接
2. **单寄存器测试**: 测试简单的16位寄存器读写
3. **32位数据测试**: 验证速度参数的高低字处理
4. **位操作测试**: 验证控制寄存器的各个位
5. **配方数据测试**: 逐个工位验证8寄存器布局

### 6.2 测试用例
```cpp
// 简单值测试32位数据处理
speed = 0x12340000;  // 高位有值，低位为0
// 预期: low16=0x0000, high16=0x1234

speed = 0x00005678;  // 高位为0，低位有值  
// 预期: low16=0x5678, high16=0x0000

// 控制寄存器位测试
controlReg = 0x0001;  // 仅bit0=1(使能)
controlReg = 0x0006;  // bit1=1(自动模式) + bit2=1(允许手动)
```

## 7. 故障排查

### 7.1 常见问题
1. **连接超时**: 检查网络配置和PLC服务状态
2. **数据错乱**: 确认字节序和数据格式
3. **位操作无效**: 验证控制寄存器位定义
4. **配方数据异常**: 检查寄存器地址计算和打包算法

### 7.2 日志监控
上位机提供详细的操作日志，包括：
- 连接状态变化
- 寄存器读写操作
- 错误信息和异常代码
- 参数设置和状态变化

---

**文档版本**: 1.0  
**创建日期**: 2025-07-18  
**适用系统**: 磁浮控制系统 v1.0+