# Qt C++开发环境与Conda隔离指南

## 背景说明

在同时进行Qt C++开发和Python开发时，可能会遇到Qt库版本冲突的问题。本指南提供几种解决方案来隔离开发环境。

## 推荐方案

### 方案A：移除base环境的PyQt（推荐）
```bash
# 从base环境移除PyQt，避免全局影响
conda remove pyqt qt -n base

# 创建专门的Python开发环境
conda create -n python-dev python=3.9 pyqt matplotlib numpy pandas
conda activate python-dev  # 使用Python GUI时激活
```

### 方案B：禁用conda自动激活
```bash
# 禁用conda自动激活base环境
conda config --set auto_activate_base false

# 创建专门的Python环境
conda create -n python-dev python=3.9 pyqt matplotlib numpy pandas

# 以后需要使用Python时手动激活
conda activate python-dev
```

### 方案C：创建独立开发环境
```bash
# 创建Qt C++专用的纯净环境（不包含Python Qt）
# 此方案适合完全分离两种开发环境

# 创建纯净Python环境用于一般开发
conda create -n clean-python python=3.9 numpy pandas matplotlib

# 创建GUI开发环境
conda create -n gui-python python=3.9 pyqt pyside2 tkinter
```

## 使用方式

| 开发场景 | 环境激活命令 | 说明 |
|----------|-------------|------|
| **Qt C++开发** | 无需激活conda环境 | 使用系统安装的Qt库和Qt Creator |
| **Python GUI开发** | `conda activate python-dev` | 使用Python的PyQt/PySide |
| **一般Python开发** | `conda activate clean-python` | 数据分析、脚本开发等 |
| **磁悬浮控制系统开发** | 无需激活conda环境 | 专注于C++ Qt开发 |

## 验证环境

### 检查Qt C++环境
```bash
# 确保没有激活conda环境
# 在Qt Creator中检查Qt版本和编译器配置
qmake --version
```

### 检查Python环境
```bash
# 激活Python环境后
conda activate python-dev
python -c "import PyQt5; print(PyQt5.Qt.PYQT_VERSION_STR)"
```

## 故障排除

### 问题1：编译时找不到Qt库
**解决方案**：确保没有激活任何conda环境，使用系统Qt库

### 问题2：Python导入PyQt失败
**解决方案**：激活正确的conda环境 `conda activate python-dev`

### 问题3：Qt Creator无法启动项目
**解决方案**：检查Qt Creator的Kit配置，确保指向正确的Qt安装路径

## 注意事项

1. **开发磁悬浮控制系统时务必不要激活conda环境**
2. 如果必须在同一命令行窗口中切换，记得 `conda deactivate`
3. Qt Creator的环境变量会继承命令行环境，启动前确保环境干净
4. 定期清理不需要的conda环境以节省磁盘空间

---

*此指南适用于Windows平台的Qt 5.12+ C++开发环境*