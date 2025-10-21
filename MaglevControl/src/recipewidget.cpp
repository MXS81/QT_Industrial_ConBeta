#include "recipewidget.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

RecipeWidget::RecipeWidget(RecipeManager* recipeManager, QWidget *parent)
    : QWidget(parent)
    , m_recipeManager(recipeManager)
    , m_mainLayout(nullptr)
    , m_mainSplitter(nullptr)
    , m_editAndTableWidget(nullptr)
    , m_editAndTableLayout(nullptr)
    , m_stationTable(nullptr)
{
    initUI();
    setupConnections();
    // applyStyles(); // 注释掉以使用全局CSS样式
    
    // 初始化显示第一个工位
    m_currentStationSpin->setValue(1);
    
    // 设置工位总数为当前实际工位数量
    m_stationTotalSpin->setValue(m_recipeManager->getStationCount());
    
    updateEditControls();
    updateTableData();
    
    // 配方管理已简化，无需额外初始化
}

void RecipeWidget::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    // 在小屏幕上使用更紧凑的间距
    m_mainLayout->setSpacing(8);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    
    initControlArea();
    
    // 创建分割器，分离编辑区域和表格区域
    m_mainSplitter = new QSplitter(Qt::Vertical);
    m_mainSplitter->setChildrenCollapsible(false); // 不允许完全折叠
    m_mainSplitter->setHandleWidth(8); // 增加拖拽手柄宽度，便于操作
    
    initTableArea();
    initEditArea();
    
    // 设置分割器的初始大小（像素）
    QList<int> sizes;
    sizes << 400 << 200; // 表格400像素，编辑区200像素
    m_mainSplitter->setSizes(sizes);
    
    // 设置分割器的拉伸因子
    m_mainSplitter->setStretchFactor(0, 1); // 表格区域
    m_mainSplitter->setStretchFactor(1, 0); // 编辑区域固定高度优先
    
    m_mainLayout->addWidget(m_mainSplitter, 1); // 分割器占用最多空间
    
    // 移除状态标签，日志信息将通过信号发送到操作日志
}

void RecipeWidget::initControlArea()
{
    m_controlWidget = new QWidget();
    m_controlLayout = new QHBoxLayout(m_controlWidget);
    m_controlLayout->setSpacing(15);
    
    // 工位总数设置
    m_stationTotalLabel = new QLabel("工位总数：");
    m_stationTotalSpin = new QSpinBox();
    m_stationTotalSpin->setRange(1, 32);
    m_stationTotalSpin->setValue(8); // 默认8个工位
    m_stationTotalSpin->setToolTip("设定工位总数，表格会自动更新");
    m_stationTotalSpin->setMaximumWidth(80);
    
    m_controlLayout->addWidget(m_stationTotalLabel);
    m_controlLayout->addWidget(m_stationTotalSpin);
    
    // 隐藏的工位选择控件（保留功能但不显示）
    m_currentStationSpin = new QSpinBox();
    m_currentStationSpin->setRange(1, 32);
    m_currentStationSpin->setValue(1);
    m_currentStationSpin->setVisible(false); // 隐藏控件
    
    // 隐藏的工位数量显示（保留功能但不显示）
    m_stationCountLabel = new QLabel();
    m_stationCountLabel->setVisible(false); // 隐藏控件
    
    m_controlLayout->addSpacing(30); // 添加间距
    
    // 配方管理按钮移到这里与工位总数在同一行
    m_saveRecipeBtn = new QPushButton("保存配方文件");
    // 移除内联样式，使用全局CSS
    m_saveRecipeBtn->setObjectName("saveRecipeBtn");
    m_saveRecipeBtn->setToolTip("将所有工位参数保存为配方文件");
    m_controlLayout->addWidget(m_saveRecipeBtn);
    
    m_loadRecipeBtn = new QPushButton("加载配方文件");
    m_loadRecipeBtn->setObjectName("loadRecipeBtn");
    m_loadRecipeBtn->setToolTip("从配方文件加载所有工位参数");
    m_controlLayout->addWidget(m_loadRecipeBtn);
    
    m_applyRecipeBtn = new QPushButton("应用到设备");
    m_applyRecipeBtn->setObjectName("applyRecipeBtn");
    m_applyRecipeBtn->setToolTip("将当前参数写入设备");
    m_controlLayout->addWidget(m_applyRecipeBtn);
    
    m_resetBtn = new QPushButton("复位当前工位");
    m_resetBtn->setObjectName("resetBtn");
    m_resetBtn->setToolTip("将当前工位恢复为默认值");
    m_controlLayout->addWidget(m_resetBtn);
    
    m_controlLayout->addStretch();
    
    m_mainLayout->addWidget(m_controlWidget);
}


void RecipeWidget::initEditArea()
{
    m_editGroup = new QGroupBox("工位设置");
    m_editLayout = new QGridLayout(m_editGroup);
    m_editLayout->setSpacing(10); // 增加间距避免重叠
    m_editLayout->setContentsMargins(12, 12, 12, 12);
    
    // 设置固定的列宽度，确保每行精确对齐
    m_editLayout->setColumnMinimumWidth(0, 120); // 标签列1
    m_editLayout->setColumnMinimumWidth(1, 120); // 输入列1
    m_editLayout->setColumnMinimumWidth(2, 120); // 标签列2
    m_editLayout->setColumnMinimumWidth(3, 120); // 输入列2
    m_editLayout->setColumnMinimumWidth(4, 120); // 标签列3
    m_editLayout->setColumnMinimumWidth(5, 120); // 输入列3
    
    // 设置列拉伸，标签列固定，输入列可拉伸
    m_editLayout->setColumnStretch(0, 0); // 标签列1 固定
    m_editLayout->setColumnStretch(1, 1); // 输入列1 拉伸
    m_editLayout->setColumnStretch(2, 0); // 标签列2 固定
    m_editLayout->setColumnStretch(3, 1); // 输入列2 拉伸
    m_editLayout->setColumnStretch(4, 0); // 标签列3 固定
    m_editLayout->setColumnStretch(5, 1); // 输入列3 拉伸
    
    // 设置大小策略，允许垂直方向调整但防止过度压缩
    m_editGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_editGroup->setMinimumHeight(240); // 增加最小高度，防止参数重叠
    m_editGroup->setMaximumHeight(400);
    
    int row = 0;
    
    // 第一行：工位、路段号、目标工位水平并排
    m_editLayout->addWidget(new QLabel("工位："), row, 0);
    m_stationNoEdit = new QLineEdit();
    m_stationNoEdit->setPlaceholderText("1-255");
    m_stationNoEdit->setMinimumWidth(80);
    m_editLayout->addWidget(m_stationNoEdit, row, 1);
    
    m_editLayout->addWidget(new QLabel("路段号："), row, 2);
    m_segmentNoEdit = new QLineEdit();
    m_segmentNoEdit->setPlaceholderText("1-255");
    m_segmentNoEdit->setMinimumWidth(80);
    m_editLayout->addWidget(m_segmentNoEdit, row, 3);
    
    m_editLayout->addWidget(new QLabel("目标工位："), row, 4);
    m_nextStationEdit = new QLineEdit();
    m_nextStationEdit->setPlaceholderText("1-255");
    m_nextStationEdit->setMinimumWidth(80);
    m_editLayout->addWidget(m_nextStationEdit, row, 5);
    
    // 任务ID (完全隐藏，不占用布局空间)
    QLabel* taskIdLabel = new QLabel("任务ID:");
    taskIdLabel->hide();
    m_taskIdSpin = new QSpinBox();
    m_taskIdSpin->setRange(1, 255);
    m_taskIdSpin->hide();
    // 不添加到布局中
    
    row++;
    
    // 路段位置
    m_editLayout->addWidget(new QLabel("路段位置（mm）："), row, 0);
    m_segmentPosEdit = new QLineEdit();
    m_segmentPosEdit->setPlaceholderText("0-65535");
    m_segmentPosEdit->setMinimumWidth(80);
    m_editLayout->addWidget(m_segmentPosEdit, row, 1);
    
    // 路段速度
    m_editLayout->addWidget(new QLabel("路段速度（mm/s）："), row, 2);
    m_segmentSpeedEdit = new QLineEdit();
    m_segmentSpeedEdit->setPlaceholderText("0-65535");
    m_segmentSpeedEdit->setMinimumWidth(80);
    m_editLayout->addWidget(m_segmentSpeedEdit, row, 3);
    
    // 起始位置
    m_editLayout->addWidget(new QLabel("起始位置（mm）："), row, 4);
    m_startPosEdit = new QLineEdit();
    m_startPosEdit->setPlaceholderText("0-65535");
    m_startPosEdit->setMinimumWidth(80);
    m_editLayout->addWidget(m_startPosEdit, row, 5);
    
    row++;
    
    // 终止位置
    m_editLayout->addWidget(new QLabel("终止位置（mm）："), row, 0);
    m_endPosEdit = new QLineEdit();
    m_endPosEdit->setPlaceholderText("0-65535");
    m_endPosEdit->setMinimumWidth(80);
    m_editLayout->addWidget(m_endPosEdit, row, 1);
    
    // 到位延时
    m_editLayout->addWidget(new QLabel("到位延时（ms）："), row, 2);
    m_arrivalDelayEdit = new QLineEdit();
    m_arrivalDelayEdit->setPlaceholderText("0-65535");
    m_arrivalDelayEdit->setMinimumWidth(80);
    m_editLayout->addWidget(m_arrivalDelayEdit, row, 3);
    
    // 摆渡位置
    m_editLayout->addWidget(new QLabel("摆渡位置："), row, 4);
    m_ferryPosCombo = new QComboBox();
    m_ferryPosCombo->addItem("None", static_cast<int>(FerryPosition::None));
    m_ferryPosCombo->addItem("Pos1", static_cast<int>(FerryPosition::Pos1));
    m_ferryPosCombo->setMinimumWidth(80);
    m_editLayout->addWidget(m_ferryPosCombo, row, 5);
    
    row++;
    
    // 工位屏蔽
    m_editLayout->addWidget(new QLabel("工位屏蔽："), row, 0);
    m_stationMaskCheck = new QCheckBox("屏蔽此工位");
    m_editLayout->addWidget(m_stationMaskCheck, row, 1);
    
    // 将编辑区域添加到分割器
    m_mainSplitter->addWidget(m_editGroup);
}

void RecipeWidget::initTableArea()
{
    m_stationTable = new QTableWidget();
    setupTable();
    
    // 设置表格的大小策略，允许分隔栏调整
    m_stationTable->setMinimumHeight(150);
    m_stationTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    // 将表格添加到分割器
    m_mainSplitter->addWidget(m_stationTable);
}

void RecipeWidget::setupTable()
{
    // 设置表格列（隐藏任务ID列）
    QStringList headers;
    headers << "工位" << "路段号" << "目标工位" 
            << "路段位置" << "路段速度" << "起始位置" << "终止位置" 
            << "到位延时" << "摆渡位置" << "工位屏蔽";
    
    m_stationTable->setColumnCount(headers.size());
    m_stationTable->setHorizontalHeaderLabels(headers);
    
    // 设置表格为只读显示（保留这个功能）
    m_stationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // 设置表格样式
    m_stationTable->setAlternatingRowColors(true);
    m_stationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_stationTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // 优化列宽设置，使用更合理的自适应方式
    QHeaderView* headerView = m_stationTable->horizontalHeader();
    
    // 核心列使用固定宽度
    m_stationTable->setColumnWidth(0, 60);   // 工位（更紧凑）
    m_stationTable->setColumnWidth(1, 70);   // 路段号
    m_stationTable->setColumnWidth(2, 70);   // 目标工位
    
    // 数值列使用统一宽度，支持自动调整
    headerView->setSectionResizeMode(3, QHeaderView::Stretch);  // 路段位置
    headerView->setSectionResizeMode(4, QHeaderView::Stretch);  // 路段速度
    headerView->setSectionResizeMode(5, QHeaderView::Stretch);  // 起始位置
    headerView->setSectionResizeMode(6, QHeaderView::Stretch);  // 终止位置
    headerView->setSectionResizeMode(7, QHeaderView::Stretch);  // 到位延时
    
    m_stationTable->setColumnWidth(8, 80);   // 摆渡位置
    m_stationTable->setColumnWidth(9, 80);   // 工位屏蔽
    
    // 设置最小列宽，防止过度压缩
    for (int i = 3; i <= 7; ++i) {
        headerView->setMinimumSectionSize(80);
    }
}

void RecipeWidget::setupConnections()
{
    // 基本控制按钮连接
    connect(m_currentStationSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RecipeWidget::onStationSelectionChanged);
    connect(m_stationTotalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RecipeWidget::onStationTotalChanged);
    connect(m_resetBtn, &QPushButton::clicked, this, &RecipeWidget::onResetCurrentStation);
    
    // 简化的配方管理按钮连接
    connect(m_saveRecipeBtn, &QPushButton::clicked, this, &RecipeWidget::onSaveRecipeFile);
    connect(m_loadRecipeBtn, &QPushButton::clicked, this, &RecipeWidget::onLoadRecipeFile);
    connect(m_applyRecipeBtn, &QPushButton::clicked, this, &RecipeWidget::onApplyToDevice);
    
    // 表格连接
    connect(m_stationTable, &QTableWidget::cellClicked, this, &RecipeWidget::onTableCellClicked);
    
    // 配方管理器连接
    connect(m_recipeManager, &RecipeManager::dataChanged, this, &RecipeWidget::onRecipeDataChanged);
    connect(m_recipeManager, &RecipeManager::stationUpdated, this, &RecipeWidget::onStationUpdated);
    connect(m_recipeManager, &RecipeManager::errorOccurred, this, &RecipeWidget::onErrorOccurred);
    connect(m_recipeManager, &RecipeManager::statusChanged, this, &RecipeWidget::onStatusChanged);
    
    // 完整配方相关信号连接
    connect(m_recipeManager, &RecipeManager::completeRecipeSaved, this, &RecipeWidget::onCompleteRecipeSaved);
    connect(m_recipeManager, &RecipeManager::completeRecipeLoaded, this, &RecipeWidget::onCompleteRecipeLoaded);
    connect(m_recipeManager, &RecipeManager::completeRecipeDeleted, this, &RecipeWidget::onCompleteRecipeDeleted);
    connect(m_recipeManager, &RecipeManager::completeRecipeApplied, this, &RecipeWidget::onCompleteRecipeApplied);
    
    // 添加控件变更时的自动保存
    connect(m_stationMaskCheck, &QCheckBox::toggled, this, &RecipeWidget::saveCurrentStationToMemory);
    connect(m_taskIdSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &RecipeWidget::saveCurrentStationToMemory);
    connect(m_ferryPosCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RecipeWidget::saveCurrentStationToMemory);
    
    // 对于数值输入框，在editingFinished时保存，避免频繁触发
    connect(m_stationNoEdit, &QLineEdit::editingFinished, this, &RecipeWidget::saveCurrentStationToMemory);
    connect(m_segmentNoEdit, &QLineEdit::editingFinished, this, &RecipeWidget::saveCurrentStationToMemory);
    connect(m_nextStationEdit, &QLineEdit::editingFinished, this, &RecipeWidget::saveCurrentStationToMemory);
    connect(m_segmentPosEdit, &QLineEdit::editingFinished, this, &RecipeWidget::saveCurrentStationToMemory);
    connect(m_segmentSpeedEdit, &QLineEdit::editingFinished, this, &RecipeWidget::saveCurrentStationToMemory);
    connect(m_startPosEdit, &QLineEdit::editingFinished, this, &RecipeWidget::saveCurrentStationToMemory);
    connect(m_endPosEdit, &QLineEdit::editingFinished, this, &RecipeWidget::saveCurrentStationToMemory);
    connect(m_arrivalDelayEdit, &QLineEdit::editingFinished, this, &RecipeWidget::saveCurrentStationToMemory);
}

void RecipeWidget::applyStyles()
{
    setStyleSheet(R"(
        /* 主窗口背景 */
        RecipeWidget {
            background-color: #1e1e1e;
            color: #ffffff;
        }
        
        /* 分组框样式 */
        QGroupBox {
            font-weight: bold;
            border: 2px solid #555555;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 15px;
            background-color: #2d2d2d;
            color: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
            color: #ffffff;
        }
        
        /* 输入控件样式 */
        QSpinBox, QDoubleSpinBox, QComboBox, QLineEdit {
            padding: 5px;
            border: 1px solid #555555;
            border-radius: 4px;
            min-width: 100px;
            background-color: #3d3d3d;
            color: #ffffff;
        }
        QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QLineEdit:focus {
            border: 2px solid #0078d4;
        }
        
        /* 复选框样式 */
        QCheckBox {
            font-weight: normal;
            color: #ffffff;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #555555;
            border-radius: 3px;
            background-color: #3d3d3d;
        }
        QCheckBox::indicator:checked {
            background-color: #0078d4;
            border: 1px solid #0078d4;
        }
        
        /* 标签样式 */
        QLabel {
            color: #ffffff;
        }
        
        /* 按钮样式 */
        QPushButton {
            color: #ffffff;
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 6px 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            border: 2px solid #0078d4;
        }
        QPushButton:pressed {
            background-color: #2d2d2d;
        }
        
        /* 表格样式 */
        QTableWidget {
            gridline-color: #555555;
            background-color: #1e1e1e;
            alternate-background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid #555555;
        }
        QTableWidget::item {
            border: none;
            padding: 5px;
        }
        QTableWidget::item:selected {
            background-color: #0078d4;
        }
        
        /* 表头样式 */
        QHeaderView::section {
            background-color: #3d3d3d;
            padding: 6px;
            border: 1px solid #555555;
            font-weight: bold;
            color: #ffffff;
        }
        QHeaderView::section:hover {
            background-color: #4d4d4d;
        }
        
        /* 下拉框下拉项样式 */
        QComboBox QAbstractItemView {
            background-color: #2d2d2d;
            color: #ffffff;
            border: 1px solid #555555;
            selection-background-color: #0078d4;
        }
        
        /* 分割器样式 */
        QSplitter::handle {
            background-color: #555555;
            border: 1px solid #404040;
        }
        QSplitter::handle:vertical {
            height: 6px;
            background-color: #555555;
            border-radius: 3px;
            margin: 2px 4px;
        }
        QSplitter::handle:vertical:hover {
            background-color: #0078d4;
        }
        QSplitter::handle:vertical:pressed {
            background-color: #106ebe;
        }
    )");
}

void RecipeWidget::onStationSelectionChanged()
{
    updateEditControls();
}

void RecipeWidget::onStationTotalChanged()
{
    int totalStations = m_stationTotalSpin->value();
    int currentStationCount = m_recipeManager->getStationCount();
    
    // 如果需要的工位数量超过当前数量，添加新的工位
    if (totalStations > currentStationCount) {
        for (int i = currentStationCount; i < totalStations; ++i) {
            // 创建默认的工位配方
            StationRecipe defaultRecipe;
            defaultRecipe.stationNo = i + 1;
            defaultRecipe.taskId = 1;
            defaultRecipe.segmentNo = i + 1;
            defaultRecipe.nextStationId = i + 2 > totalStations ? 1 : i + 2;
            defaultRecipe.segmentPosition = 188 + i * 120;
            defaultRecipe.segmentSpeed = 0;
            defaultRecipe.startPosition = 0;
            defaultRecipe.endPosition = 0;
            defaultRecipe.arrivalDelay = 0;
            defaultRecipe.ferryPos = (i == 0) ? FerryPosition::Pos1 : FerryPosition::None;
            defaultRecipe.stationMask = false;
            defaultRecipe.recipeId = 1;
            
            m_recipeManager->addStation(defaultRecipe);
        }
    }
    // 如果需要的工位数量少于当前数量，移除多余的工位
    else if (totalStations < currentStationCount) {
        for (int i = currentStationCount - 1; i >= totalStations; --i) {
            m_recipeManager->removeStation(i);
        }
    }
    
    // 更新表格显示
    updateTableData();
    
    // 更新当前工位选择范围
    m_currentStationSpin->setRange(1, totalStations);
    
    // 确保当前选中的工位不超出范围
    if (m_currentStationSpin->value() > totalStations) {
        m_currentStationSpin->setValue(totalStations);
    }
}

void RecipeWidget::onSaveCurrentStation()
{
    int currentStation = m_currentStationSpin->value() - 1;
    StationRecipe recipe = getRecipeFromControls();
    
    if (m_recipeManager->saveRecipe(currentStation, recipe)) {
        emit logMessage("[RECIPE] 当前工位配方已保存");
        updateTableData();
    } else {
        emit logMessage("[RECIPE] 当前工位配方保存失败");
    }
}

void RecipeWidget::onSaveAllStations()
{
    // 这里应该是保存到文件，修改实现
    QString filename = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/MaglevControl/recipes.json";
    m_recipeManager->saveDataToFile(filename);
}

void RecipeWidget::onLoadAllStations()
{
    // 这里应该是从文件加载，修改实现
    QString filename = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/MaglevControl/recipes.json";
    m_recipeManager->loadDataFromFile(filename);
    updateTableData();
    updateEditControls();
    updateRecipeCombo(); // 更新配方列表
}

void RecipeWidget::onResetCurrentStation()
{
    int currentStation = m_currentStationSpin->value() - 1;
    
    // 重置为默认值
    StationRecipe defaultRecipe;
    defaultRecipe.stationNo = currentStation + 1;
    defaultRecipe.segmentNo = currentStation + 1;
    defaultRecipe.segmentPosition = 188.000f + currentStation * 120.000f;
    defaultRecipe.segmentSpeed = 0.000f;
    defaultRecipe.startPosition = 0.000f;
    defaultRecipe.endPosition = 0.000f;
    defaultRecipe.arrivalDelay = 0.000f;
    defaultRecipe.ferryPos = (currentStation == 0) ? FerryPosition::Pos1 : FerryPosition::None;
    defaultRecipe.stationMask = false;
    defaultRecipe.recipeId = 1; // 保持默认值
    defaultRecipe.taskId = 1;
    
    m_recipeManager->updateStation(currentStation, defaultRecipe);
    updateEditControls();
    updateTableData();
    
    emit logMessage("[RECIPE] 当前工位已重置");
}

void RecipeWidget::onTableCellClicked(int row, int column)
{
    Q_UNUSED(column)
    m_currentStationSpin->setValue(row + 1);
}

void RecipeWidget::onRecipeDataChanged()
{
    updateTableData();
    m_stationCountLabel->setText(QString("(共 %1 个工位)").arg(m_recipeManager->getStationCount()));
}

void RecipeWidget::onStationUpdated(int index)
{
    Q_UNUSED(index)
    updateTableData();
}

void RecipeWidget::onErrorOccurred(const QString& error)
{
    emit logMessage("[RECIPE] 错误: " + error);
    QMessageBox::warning(this, "错误", error);
}

void RecipeWidget::onStatusChanged(const QString& message)
{
    emit logMessage("[RECIPE] " + message);
}

void RecipeWidget::updateEditControls()
{
    int currentStation = m_currentStationSpin->value() - 1;
    StationRecipe recipe = m_recipeManager->getStation(currentStation);
    setControlsFromRecipe(recipe);
}

void RecipeWidget::updateTableData()
{
    populateTable();
    m_stationCountLabel->setText(QString("(共 %1 个工位)").arg(m_recipeManager->getStationCount()));
}

StationRecipe RecipeWidget::getRecipeFromControls()
{
    StationRecipe recipe;
    recipe.stationNo = static_cast<quint8>(m_stationNoEdit->text().toUInt());
    recipe.taskId = static_cast<quint8>(m_taskIdSpin->value());
    recipe.segmentNo = static_cast<quint8>(m_segmentNoEdit->text().toUInt());
    recipe.nextStationId = static_cast<quint8>(m_nextStationEdit->text().toUInt());
    recipe.segmentPosition = static_cast<quint16>(m_segmentPosEdit->text().toUInt());
    recipe.segmentSpeed = static_cast<quint16>(m_segmentSpeedEdit->text().toUInt());
    recipe.startPosition = static_cast<quint16>(m_startPosEdit->text().toUInt());
    recipe.endPosition = static_cast<quint16>(m_endPosEdit->text().toUInt());
    recipe.arrivalDelay = static_cast<quint16>(m_arrivalDelayEdit->text().toUInt());
    recipe.ferryPos = static_cast<FerryPosition>(m_ferryPosCombo->currentData().toInt());
    recipe.stationMask = m_stationMaskCheck->isChecked();
    recipe.recipeId = 1; // 设置默认值，保持数据结构兼容
    
    return recipe;
}

void RecipeWidget::setControlsFromRecipe(const StationRecipe& recipe)
{
    m_stationNoEdit->setText(QString::number(recipe.stationNo));
    m_taskIdSpin->setValue(recipe.taskId);
    m_segmentNoEdit->setText(QString::number(recipe.segmentNo));
    m_nextStationEdit->setText(QString::number(recipe.nextStationId));
    m_segmentPosEdit->setText(QString::number(recipe.segmentPosition));
    m_segmentSpeedEdit->setText(QString::number(recipe.segmentSpeed));
    m_startPosEdit->setText(QString::number(recipe.startPosition));
    m_endPosEdit->setText(QString::number(recipe.endPosition));
    m_arrivalDelayEdit->setText(QString::number(recipe.arrivalDelay));
    
    // 设置摆渡位置下拉框
    int index = m_ferryPosCombo->findData(static_cast<int>(recipe.ferryPos));
    if (index != -1) {
        m_ferryPosCombo->setCurrentIndex(index);
    }
    
    m_stationMaskCheck->setChecked(recipe.stationMask);
}

void RecipeWidget::populateTable()
{
    QVector<StationRecipe> stations = m_recipeManager->getAllStations();
    m_stationTable->setRowCount(stations.size());
    
    for (int i = 0; i < stations.size(); ++i) {
        const StationRecipe& station = stations[i];
        
        // 创建只读的表格项（保留这个功能）
        auto createReadOnlyItem = [](const QString& text) -> QTableWidgetItem* {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable); // 设置为只读
            return item;
        };
        
        // 工位
        m_stationTable->setItem(i, 0, createReadOnlyItem(QString::number(station.stationNo)));
        
        // 路段号
        m_stationTable->setItem(i, 1, createReadOnlyItem(QString::number(station.segmentNo)));
        
        // 目标工位
        m_stationTable->setItem(i, 2, createReadOnlyItem(QString::number(station.nextStationId)));
        
        // 路段位置 - 现在是整数
        m_stationTable->setItem(i, 3, createReadOnlyItem(QString::number(station.segmentPosition)));
        
        // 路段速度 - 现在是整数
        m_stationTable->setItem(i, 4, createReadOnlyItem(QString::number(station.segmentSpeed)));
        
        // 起始位置 - 现在是整数
        m_stationTable->setItem(i, 5, createReadOnlyItem(QString::number(station.startPosition)));
        
        // 终止位置 - 现在是整数
        m_stationTable->setItem(i, 6, createReadOnlyItem(QString::number(station.endPosition)));
        
        // 到位延时 - 现在是整数
        m_stationTable->setItem(i, 7, createReadOnlyItem(QString::number(station.arrivalDelay)));
        
        // 摆渡位置
        m_stationTable->setItem(i, 8, createReadOnlyItem(ferryPositionToString(station.ferryPos)));
        
        // 工位屏蔽
        m_stationTable->setItem(i, 9, createReadOnlyItem(station.stationMask ? "是" : "否"));
        
        // 表格项会自动使用交替行颜色
    }
}

QString RecipeWidget::ferryPositionToString(FerryPosition pos)
{
    switch (pos) {
    case FerryPosition::Pos1: return "Pos1";
    case FerryPosition::None: return "None";
    case FerryPosition::Pos2: return "Pos2";
    default: return "Unknown";
    }
}

FerryPosition RecipeWidget::stringToFerryPosition(const QString& str)
{
    if (str == "Pos1") return FerryPosition::Pos1;
    if (str == "Pos2") return FerryPosition::Pos2;
    return FerryPosition::None;
}

// 简化的配方管理槽函数实现
void RecipeWidget::onSaveRecipeFile()
{
    QString filePath = getRecipeFilePath(true); // true表示保存对话框
    if (filePath.isEmpty()) {
        return; // 用户取消了操作
    }
    
    // 保存所有工位数据到文件
    m_recipeManager->saveDataToFile(filePath);
    showSaveSuccess(filePath);
}

void RecipeWidget::onLoadRecipeFile()
{
    QString filePath = getRecipeFilePath(false); // false表示打开对话框
    if (filePath.isEmpty()) {
        return; // 用户取消了操作
    }
    
    // 从文件加载所有工位数据
    m_recipeManager->loadDataFromFile(filePath);
    updateTableData();
    updateEditControls();
    showLoadSuccess(filePath);
}

void RecipeWidget::onDeleteCompleteRecipe()
{
    QString selectedRecipe = m_recipeCombo->currentText();
    if (selectedRecipe.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要删除的配方");
        return;
    }
    
    int ret = QMessageBox::question(this, "确认删除", 
                                   QString("确定要删除配方 '%1' 吗？\n此操作不可撤销。").arg(selectedRecipe),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_recipeManager->deleteCompleteRecipe(selectedRecipe);
    }
}

void RecipeWidget::onApplyCompleteRecipe()
{
    QString selectedRecipe = m_recipeCombo->currentText();
    if (selectedRecipe.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要应用的配方");
        return;
    }
    
    int ret = QMessageBox::question(this, "确认应用", 
                                   QString("确定要将配方 '%1' 应用到设备吗？\n这将加载配方并覆盖设备当前的所有工位参数。").arg(selectedRecipe),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_recipeManager->applyCompleteRecipeToDevice(selectedRecipe);
    }
}

void RecipeWidget::onRenameCompleteRecipe()
{
    QString selectedRecipe = m_recipeCombo->currentText();
    if (selectedRecipe.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要重命名的配方");
        return;
    }
    
    bool ok;
    QString newName = QInputDialog::getText(this, "重命名配方", 
                                           QString("为配方 '%1' 输入新名称:").arg(selectedRecipe),
                                           QLineEdit::Normal, selectedRecipe, &ok);
    
    if (ok && !newName.isEmpty() && newName != selectedRecipe) {
        m_recipeManager->renameCompleteRecipe(selectedRecipe, newName);
    }
}

void RecipeWidget::onCompleteRecipeSelectionChanged()
{
    updateCurrentRecipeLabel();
    
    // 更新按钮状态
    bool hasSelection = !m_recipeCombo->currentText().isEmpty();
    m_loadRecipeBtn->setEnabled(hasSelection);
    m_deleteRecipeBtn->setEnabled(hasSelection);
    m_applyRecipeBtn->setEnabled(hasSelection);
    m_renameRecipeBtn->setEnabled(hasSelection);
}

// 完整配方相关槽函数
void RecipeWidget::onCompleteRecipeSaved(const QString& recipeName)
{
    updateRecipeCombo();
    
    // 选中刚保存的配方
    int index = m_recipeCombo->findText(recipeName);
    if (index != -1) {
        m_recipeCombo->setCurrentIndex(index);
    }
    
    updateCurrentRecipeLabel();
}

void RecipeWidget::onCompleteRecipeLoaded(const QString& recipeName)
{
    updateRecipeCombo();
    updateCurrentRecipeLabel();
    
    // 选中加载的配方
    int index = m_recipeCombo->findText(recipeName);
    if (index != -1) {
        m_recipeCombo->setCurrentIndex(index);
    }
}

void RecipeWidget::onCompleteRecipeDeleted(const QString& recipeName)
{
    Q_UNUSED(recipeName)
    updateRecipeCombo();
    updateCurrentRecipeLabel();
}

void RecipeWidget::onCompleteRecipeApplied(const QString& recipeName)
{
    Q_UNUSED(recipeName)
    updateCurrentRecipeLabel();
}

// 辅助方法实现
void RecipeWidget::updateRecipeCombo()
{
    QString currentText = m_recipeCombo->currentText();
    
    m_recipeCombo->clear();
    QStringList recipeNames = m_recipeManager->getCompleteRecipeNames();
    
    if (!recipeNames.isEmpty()) {
        m_recipeCombo->addItems(recipeNames);
        
        // 尝试保持之前的选择
        int index = m_recipeCombo->findText(currentText);
        if (index != -1) {
            m_recipeCombo->setCurrentIndex(index);
        }
    }
    
    onCompleteRecipeSelectionChanged();
}

void RecipeWidget::updateCurrentRecipeLabel()
{
    QStringList recipeNames = m_recipeManager->getCompleteRecipeNames();
    QString currentRecipe = "未选择";
    
    // 这里可以从RecipeManager获取当前配方名称
    if (!recipeNames.isEmpty()) {
        QString selectedRecipe = m_recipeCombo->currentText();
        if (!selectedRecipe.isEmpty()) {
            CompleteRecipe recipe = m_recipeManager->getCompleteRecipe(selectedRecipe);
            if (recipe.isValid()) {
                currentRecipe = QString("%1 (共%2个工位)").arg(selectedRecipe).arg(recipe.stationCount());
            }
        }
    }
    
    m_currentRecipeLabel->setText("当前配方: " + currentRecipe);
}

QString RecipeWidget::getNewRecipeName()
{
    bool ok;
    QString recipeName = QInputDialog::getText(this, "保存配方", 
                                              "请输入配方名称:",
                                              QLineEdit::Normal, 
                                              QString("配方%1").arg(m_recipeManager->getCompleteRecipeNames().size() + 1), 
                                              &ok);
    
    if (!ok || recipeName.isEmpty()) {
        return QString(); // 用户取消或输入为空
    }
    
    // 检查配方名称是否已存在
    if (m_recipeManager->getCompleteRecipeNames().contains(recipeName)) {
        int ret = QMessageBox::question(this, "配方已存在", 
                                       QString("配方 '%1' 已存在，是否覆盖？").arg(recipeName),
                                       QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No) {
            return getNewRecipeName(); // 递归重新输入
        }
    }
    
    return recipeName;
}

void RecipeWidget::onApplyToDevice()
{
    int ret = QMessageBox::question(this, "确认应用", 
                                   "确定要将当前参数应用到设备吗？\n这将覆盖设备中的所有工位参数。",
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        // 将所有工位参数写入设备
        if (m_recipeManager->saveAllRecipes()) {
            emit logMessage("[RECIPE] 参数已成功应用到设备");
        } else {
            emit logMessage("[RECIPE] 应用到设备失败");
        }
    }
}

// 辅助方法实现
QString RecipeWidget::getRecipeFilePath(bool save)
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/配方文件";
    QDir().mkpath(defaultPath); // 确保目录存在
    
    QString filePath;
    if (save) {
        filePath = QFileDialog::getSaveFileName(this, "保存配方文件", 
                                              defaultPath + "/配方.json",
                                              "配方文件 (*.json);;所有文件 (*.*)");
    } else {
        filePath = QFileDialog::getOpenFileName(this, "加载配方文件",
                                              defaultPath,
                                              "配方文件 (*.json);;所有文件 (*.*)");
    }
    
    return filePath;
}

void RecipeWidget::showSaveSuccess(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    emit logMessage(QString("[RECIPE] 配方文件已保存: %1").arg(fileInfo.fileName()));
}

void RecipeWidget::showLoadSuccess(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    emit logMessage(QString("[RECIPE] 配方文件已加载: %1").arg(fileInfo.fileName()));
}

void RecipeWidget::saveCurrentStationToMemory()
{
    int currentStation = m_currentStationSpin->value() - 1;
    StationRecipe recipe = getRecipeFromControls();
    m_recipeManager->updateStation(currentStation, recipe);
    updateTableData(); // 更新表格显示
}
