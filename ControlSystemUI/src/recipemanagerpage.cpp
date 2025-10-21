// RecipeManagerPage.cpp
#include "RecipeManagerPage.h"
#include "ModbusManager.h"
#include "LogWidget.h"
#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QFile>
#include <QHeaderView>
#include <QThread>

int RecipeManagerPage::s_nextRecipeId = 1;

RecipeManagerPage::RecipeManagerPage(QList<MoverData> *movers, const QString &currentUser, ModbusManager *modbusManager, QWidget *parent)
    : QWidget(parent)
    , m_movers(movers) // 使用初始化列表保存传入的指针和值
    , m_currentUser(currentUser)
    , m_modbusManager(modbusManager)
    , m_currentRecipeIndex(-1)
    , m_isModified(false)
    , m_mainWindow(qobject_cast<MainWindow*>(parent))
{
    setupUI();
    createSampleRecipes();
    loadRecipes();
    addLogEntry(QString("配方管理页面已加载 - 管理员: %1").arg(m_currentUser), "success");
}

RecipeManagerPage::~RecipeManagerPage()
{
    if (m_isModified) {
        saveRecipes();
    }
}

void RecipeManagerPage::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    // 使用分割器
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // 左侧：配方列表
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    QGroupBox *listGroup = new QGroupBox("配方列表");
    listGroup->setStyleSheet(R"(
        QGroupBox {
            font-size: 14px;
            font-weight: bold;
            color: #e94560;
            border: 2px solid #3a3a5e;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 20px;
            padding: 0 10px 0 10px;
        }
    )");

    QVBoxLayout *listLayout = new QVBoxLayout(listGroup);

    m_recipeList = new QListWidget();
    m_recipeList->setStyleSheet(R"(
        QListWidget {
            background-color: #16213e;
            color: white;
            border: 1px solid #3a3a5e;
            font-size: 13px;
        }
        QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #3a3a5e;
        }
        QListWidget::item:selected {
            background-color: #533483;
        }
        QListWidget::item:hover {
            background-color: #0f3460;
        }
    )");

    // 列表操作按钮
    QHBoxLayout *listBtnLayout = new QHBoxLayout();
    m_newBtn = new QPushButton("➕ 新建");
    m_deleteBtn = new QPushButton("🗑️ 删除");
    m_importBtn = new QPushButton("📥 导入");
    m_exportBtn = new QPushButton("📤 导出");

    QString btnStyle = R"(
        QPushButton {
            background-color: #533483;
            color: white;
            border: none;
            padding: 8px 15px;
            font-size: 12px;
            font-weight: bold;
            border-radius: 4px;
        }
        QPushButton:hover {
            background-color: #e94560;
        }
        QPushButton:pressed {
            background-color: #c23652;
        }
        QPushButton:disabled {
            background-color: #3a3a5e;
            color: #888;
        }
    )";

    m_newBtn->setStyleSheet(btnStyle);
    m_deleteBtn->setStyleSheet(btnStyle);
    m_importBtn->setStyleSheet(btnStyle);
    m_exportBtn->setStyleSheet(btnStyle);

    listBtnLayout->addWidget(m_newBtn);
    listBtnLayout->addWidget(m_deleteBtn);
    listBtnLayout->addStretch();
    listBtnLayout->addWidget(m_importBtn);
    listBtnLayout->addWidget(m_exportBtn);

    listLayout->addWidget(m_recipeList);
    listLayout->addLayout(listBtnLayout);

    leftLayout->addWidget(listGroup);

    // 中间：配方详情
    QWidget *centerPanel = new QWidget();
    QVBoxLayout *centerLayout = new QVBoxLayout(centerPanel);

    QGroupBox *detailGroup = new QGroupBox("配方详情");
    detailGroup->setStyleSheet(listGroup->styleSheet());
    QVBoxLayout *detailLayout = new QVBoxLayout(detailGroup);

    // 配方名称
    QHBoxLayout *nameLayout = new QHBoxLayout();
    QLabel *nameLabel = new QLabel("配方名称:");
    nameLabel->setStyleSheet("color: white; font-size: 13px;");
    m_recipeNameEdit = new QLineEdit();
    m_recipeNameEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #0f3460;
            color: white;
            border: 1px solid #3a3a5e;
            padding: 8px;
            font-size: 14px;
        }
    )");
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_recipeNameEdit);

    // 配方描述
    QLabel *descLabel = new QLabel("配方描述:");
    descLabel->setStyleSheet("color: white; font-size: 13px;");
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(80);
    m_descriptionEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: #0f3460;
            color: white;
            border: 1px solid #3a3a5e;
            padding: 5px;
            font-size: 12px;
        }
    )");

    // 时间信息
    QHBoxLayout *timeLayout = new QHBoxLayout();
    m_createTimeLabel = new QLabel("创建时间: -");
    m_modifyTimeLabel = new QLabel("修改时间: -");
    m_createTimeLabel->setStyleSheet("color: #888; font-size: 11px;");
    m_modifyTimeLabel->setStyleSheet("color: #888; font-size: 11px;");
    timeLayout->addWidget(m_createTimeLabel);
    timeLayout->addStretch();
    timeLayout->addWidget(m_modifyTimeLabel);

    // 配置表格
    QLabel *configLabel = new QLabel("动子配置:");
    configLabel->setStyleSheet("color: white; font-size: 13px; font-weight: bold;");

    m_configTable = new QTableWidget(4, 5);
    m_configTable->setHorizontalHeaderLabels({
        "动子ID", "目标位置(mm)", "速度(mm/s)", "加速度(mm/s²)", "延时(ms)"
    });
    m_configTable->horizontalHeader()->setStretchLastSection(true);
    m_configTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #16213e;
            color: white;
            gridline-color: #3a3a5e;
            border: 1px solid #3a3a5e;
        }
        QTableWidget::item {
            padding: 5px;
        }
        QTableWidget::item:selected {
            background-color: #533483;
        }
        QHeaderView::section {
            background-color: #0f3460;
            color: white;
            padding: 5px;
            border: 1px solid #3a3a5e;
            font-weight: bold;
        }
    )");

    // 初始化表格
    for (int i = 0; i < 4; ++i) {
        m_configTable->setItem(i, 0, new QTableWidgetItem(QString::number(i)));
        m_configTable->item(i, 0)->setFlags(Qt::ItemIsEnabled); // ID不可编辑

        m_configTable->setItem(i, 1, new QTableWidgetItem("0.0"));
        m_configTable->setItem(i, 2, new QTableWidgetItem("100.0"));
        m_configTable->setItem(i, 3, new QTableWidgetItem("500.0"));
        m_configTable->setItem(i, 4, new QTableWidgetItem("0"));
    }

    // 操作按钮
    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_saveBtn = new QPushButton("💾 保存配方");
    m_applyBtn = new QPushButton("▶️ 应用配方");

    QString actionBtnStyle = R"(
        QPushButton {
            background-color: #22c55e;
            color: white;
            border: none;
            padding: 12px 30px;
            font-size: 14px;
            font-weight: bold;
            border-radius: 5px;
        }
        QPushButton:hover {
            background-color: #16a34a;
        }
        QPushButton:pressed {
            background-color: #15803d;
        }
        QPushButton:disabled {
            background-color: #3a3a5e;
            color: #888;
        }
    )";

    // 配方ID (只读)
    QHBoxLayout *idLayout = new QHBoxLayout();
    QLabel *idLabel = new QLabel("配方ID:");
    idLabel->setStyleSheet("color: white; font-size: 13px;");
    m_recipeIdLabel = new QLabel("-");
    m_recipeIdLabel->setStyleSheet("color: #888; font-size: 13px;");
    idLayout->addWidget(idLabel);
    idLayout->addWidget(m_recipeIdLabel);
    idLayout->addStretch();

    m_saveBtn->setStyleSheet(actionBtnStyle);
    m_applyBtn->setStyleSheet(actionBtnStyle.replace("#22c55e", "#3b82f6").replace("#16a34a", "#2563eb"));

    actionLayout->addWidget(m_saveBtn);
    actionLayout->addWidget(m_applyBtn);
    actionLayout->addStretch();

    detailLayout->addLayout(nameLayout);
    detailLayout->addWidget(descLabel);
    detailLayout->addWidget(m_descriptionEdit);
    detailLayout->addLayout(timeLayout);
    detailLayout->addSpacing(10);
    detailLayout->addWidget(configLabel);
    detailLayout->addWidget(m_configTable);
    detailLayout->addLayout(actionLayout);
    detailLayout->addLayout(idLayout);
    centerLayout->addWidget(detailGroup);



    // 右侧：日志
    QWidget *rightPanel = new QWidget();
    rightPanel->setMaximumWidth(300);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    // 直接使用LogWidget替换原来的QGroupBox + QTextEdit组合
    m_logWidget = new LogWidget("操作日志", this);
    m_logWidget->setMaxLines(200);      // 设置最大行数
    m_logWidget->setShowUser(true);     // 显示用户信息
    m_logWidget->setAutoScroll(true);   // 自动滚动

    // 自定义样式以匹配原有风格
    QString customGroupStyle = R"(
        QGroupBox {
            font-size: 14px;
            font-weight: bold;
            color: #e94560;
            border: 2px solid #3a3a5e;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 20px;
            padding: 0 10px 0 10px;
        }
    )";
    m_logWidget->setGroupBoxStyle(customGroupStyle);

    rightLayout->addWidget(m_logWidget);

    // 组装布局
    splitter->addWidget(leftPanel);
    splitter->addWidget(centerPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 1);

    mainLayout->addWidget(splitter);

    // 连接信号
    connect(m_recipeList, &QListWidget::itemSelectionChanged,
            this, &RecipeManagerPage::onRecipeSelected);
    connect(m_newBtn, &QPushButton::clicked, this, &RecipeManagerPage::onNewRecipe);
    connect(m_saveBtn, &QPushButton::clicked, this, &RecipeManagerPage::onSaveRecipe);
    connect(m_deleteBtn, &QPushButton::clicked, this, &RecipeManagerPage::onDeleteRecipe);
    connect(m_applyBtn, &QPushButton::clicked, this, &RecipeManagerPage::onApplyRecipe);
    connect(m_exportBtn, &QPushButton::clicked, this, &RecipeManagerPage::onExportRecipe);
    connect(m_importBtn, &QPushButton::clicked, this, &RecipeManagerPage::onImportRecipe);
    connect(m_recipeNameEdit, &QLineEdit::textChanged,
            this, &RecipeManagerPage::onRecipeNameChanged);
    connect(m_descriptionEdit, &QTextEdit::textChanged, this,[this]() {
        m_isModified = true;
    });
}

void RecipeManagerPage::createSampleRecipes()
{
    // 创建示例配方
    Recipe recipe1;
    recipe1.name = "标准分布";
    recipe1.id = s_nextRecipeId++;
    recipe1.description = "将4个动子均匀分布在轨道上";
    recipe1.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    recipe1.modifyTime = recipe1.createTime;

    for (int i = 0; i < 4; ++i) {
        QJsonObject config;
        config["id"] = i;
        config["position"] = i * 1863.94;
        config["speed"] = 200.0;
        config["acceleration"] = 800.0;
        config["delay"] = i * 500;
        recipe1.moverConfigs.append(config);
    }
    m_recipes.append(recipe1);

    Recipe recipe2;
    recipe2.name = "集中原点";
    recipe2.id = s_nextRecipeId++; // ID: 2
    recipe2.description = "所有动子返回原点位置";
    recipe2.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    recipe2.modifyTime = recipe2.createTime;

    for (int i = 0; i < 4; ++i) {
        QJsonObject config;
        config["id"] = i;
        config["position"] = 0.0;
        config["speed"] = 150.0;
        config["acceleration"] = 600.0;
        config["delay"] = i * 1000;
        recipe2.moverConfigs.append(config);
    }
    m_recipes.append(recipe2);

    Recipe recipe3;
    recipe3.name = "双侧对称";
    recipe3.id = s_nextRecipeId++; // ID: 2
    recipe3.description = "动子分布在轨道两侧";
    recipe3.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    recipe3.modifyTime = recipe3.createTime;

    QList<double> positions = {0, 3727.88, 500, 4227.88};
    for (int i = 0; i < 4; ++i) {
        QJsonObject config;
        config["id"] = i;
        config["position"] = positions[i];
        config["speed"] = 250.0;
        config["acceleration"] = 1000.0;
        config["delay"] = 0;
        recipe3.moverConfigs.append(config);
    }
    m_recipes.append(recipe3);
}

void RecipeManagerPage::loadRecipes()
{
    m_recipeList->clear();
    const auto& recipesRef = m_recipes;
    for (const Recipe &recipe : recipesRef) {
        m_recipeList->addItem(recipe.name);
    }

    if (m_recipes.size() > 0) {
        m_recipeList->setCurrentRow(0);
    }
}

void RecipeManagerPage::addLogEntry(const QString &message, const QString &type)
{
    // 1. 添加到本页面的日志
    m_logWidget->addLogEntry(message, type, m_currentUser);

    // 2. 转发到全局日志
    if (m_mainWindow) {
        m_mainWindow->addGlobalLogEntry(QString("[配方管理] %1").arg(message), type);
    }
}

void RecipeManagerPage::saveRecipes()
{
    // 步骤1：检查是否有选中的配方，如果没有则直接返回
    if (m_currentRecipeIndex < 0 || m_currentRecipeIndex >= m_recipes.size()) {
        return;
    }

    // 步骤2：获取当前选中配方的引用
    Recipe &recipe = m_recipes[m_currentRecipeIndex];

    // 这里可以实现保存到文件的逻辑
    m_isModified = false;
    recipe.modifyTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    recipe.lastModifiedBy = m_currentUser;  // 记录最后修改者
    addLogEntry(QString("配方 '%1' 已在退出时由 %2 自动保存").arg(recipe.name,m_currentUser), "success");
}

void RecipeManagerPage::updateRecipeDetails()
{
    if (m_currentRecipeIndex < 0 || m_currentRecipeIndex >= m_recipes.size()) {
        m_recipeIdLabel->setText("-");
        m_recipeNameEdit->clear();
        m_descriptionEdit->clear();
        m_createTimeLabel->setText("创建时间: -");
        m_modifyTimeLabel->setText("修改时间: -");
        m_configTable->clearContents(); // 只清除内容，保留表头
        return;
    }

    const Recipe &recipe = m_recipes[m_currentRecipeIndex];

    m_recipeNameEdit->setText(recipe.name);
    m_recipeIdLabel->setText(QString::number(recipe.id));
    m_descriptionEdit->setText(recipe.description);
    m_createTimeLabel->setText("创建时间: " + recipe.createTime);
    m_modifyTimeLabel->setText("修改时间: " + recipe.modifyTime);

    // 更新配置表格
    for (int i = 0; i < recipe.moverConfigs.size() && i < m_configTable->rowCount(); ++i) {
        const QJsonObject &config = recipe.moverConfigs[i];

        m_configTable->item(i, 1)->setText(QString::number(config["position"].toDouble(), 'f', 1));
        m_configTable->item(i, 2)->setText(QString::number(config["speed"].toDouble(), 'f', 1));
        m_configTable->item(i, 3)->setText(QString::number(config["acceleration"].toDouble(), 'f', 0));
        m_configTable->item(i, 4)->setText(QString::number(config["delay"].toInt()));
    }
}

void RecipeManagerPage::onNewRecipe()
{
    Recipe newRecipe;
    newRecipe.name = QString("新配方_%1").arg(m_recipes.size() + 1);
    newRecipe.id = s_nextRecipeId++;
    newRecipe.description = "请输入配方描述";
    newRecipe.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    newRecipe.modifyTime = newRecipe.createTime;

    for (int i = 0; i < 4; ++i) {
        QJsonObject config;
        config["id"] = i;
        config["position"] = 0.0;
        config["speed"] = 100.0;
        config["acceleration"] = 500.0;
        config["delay"] = 0;
        newRecipe.moverConfigs.append(config);
    }

    m_recipes.append(newRecipe);
    m_recipeList->blockSignals(true);
    m_recipeList->addItem(newRecipe.name);
    m_recipeList->setCurrentRow(m_recipes.size() - 1);
    m_recipeList->blockSignals(false);
    addLogEntry(QString("创建新配方: %1").arg(newRecipe.name), "success");
    onRecipeSelected();
}

void RecipeManagerPage::onRecipeSelected()
{
    QList<QListWidgetItem*> selected = m_recipeList->selectedItems();
    if (selected.isEmpty()) return;

    m_currentRecipeIndex = m_recipeList->row(selected.first());
    updateRecipeDetails();
    addLogEntry(QString("已选择配方: %1").arg(m_recipes[m_currentRecipeIndex].name), "info");
}

void RecipeManagerPage::onSaveRecipe()
{
    if (m_currentRecipeIndex < 0 || m_currentRecipeIndex >= m_recipes.size()) {
        return;
    }

    Recipe &recipe = m_recipes[m_currentRecipeIndex];
    recipe.name = m_recipeNameEdit->text();
    recipe.description = m_descriptionEdit->toPlainText();
    recipe.modifyTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    recipe.lastModifiedBy = m_currentUser;  // 记录最后修改者
    // 保存表格数据
    recipe.moverConfigs.clear();
    for (int i = 0; i < m_configTable->rowCount(); ++i) {
        QJsonObject config;
        config["id"] = i;
        config["position"] = m_configTable->item(i, 1)->text().toDouble();
        config["speed"] = m_configTable->item(i, 2)->text().toDouble();
        config["acceleration"] = m_configTable->item(i, 3)->text().toDouble();
        config["delay"] = m_configTable->item(i, 4)->text().toInt();
        recipe.moverConfigs.append(config);
    }

    // 更新列表显示
    m_recipeList->item(m_currentRecipeIndex)->setText(recipe.name);

    m_isModified = false;
    addLogEntry(QString("配方 '%1' 已由 %2 保存").arg(recipe.name,m_currentUser), "success");
}

void RecipeManagerPage::onDeleteRecipe()
{
    if (m_currentRecipeIndex < 0 || m_currentRecipeIndex >= m_recipes.size()) {
        return;
    }

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("删除配方");
    msgBox.setText(QString("确定要删除配方 '%1' 吗？").arg(m_recipes[m_currentRecipeIndex].name));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        QString name = m_recipes[m_currentRecipeIndex].name;
        m_recipeList->blockSignals(true); // --- 开始阻塞信号 ---

        m_recipes.removeAt(m_currentRecipeIndex);
        delete m_recipeList->takeItem(m_currentRecipeIndex);

        m_recipeList->blockSignals(false); // --- 恢复信号 ---

        // 手动更新状态
        if (m_recipes.isEmpty()) {
            m_currentRecipeIndex = -1;
            updateRecipeDetails(); // 清空右侧UI
        } else {
            // 默认选中前一个，如果删的是第一个就选中新的第一个
            int newIndex = qMax(0, m_currentRecipeIndex - 1);
            m_recipeList->setCurrentRow(newIndex);
            // onRecipeSelected 会被自动调用，无需手动更新
        }

        addLogEntry(QString("配方 '%1' 已删除").arg(name), "warning");
    }
}

void RecipeManagerPage::onApplyRecipe()
{
    if (m_currentRecipeIndex < 0 || m_currentRecipeIndex >= m_recipes.size()) {
        return;
    }

    const Recipe &recipe = m_recipes[m_currentRecipeIndex];

    // 检查连接模式
    bool isPlcMode = false;
    if (m_modbusManager) {
        isPlcMode = m_modbusManager->isConnected();
    }

    QString mode = isPlcMode ? "PLC模式" : "模拟模式";

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("应用配方");
    msgBox.setText(QString("确定要应用配方 '%1' 吗？(%2)").arg(recipe.name,mode));
    msgBox.setInformativeText(isPlcMode ?
                                  "所有动子将按照配方配置通过PLC开始移动。" :
                                  "所有动子将按照配方配置在模拟模式下开始移动。");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {

        if (isPlcMode) {
            // PLC模式：发送命令到真实设备
            addLogEntry("开始向PLC发送配方数据...", "info");

            bool allCommandsSent = true;

            // 逐个发送动子配置到PLC
            for (const QJsonObject &config : recipe.moverConfigs) {
                int moverId = config["id"].toInt();
                double targetPos = config["position"].toDouble();
                double targetSpeed = config["speed"].toDouble();
                double targetAccel = config["acceleration"].toDouble();
                int delay = config["delay"].toInt();

                if (moverId >= 0 && moverId < m_movers->size()) {

                    // 1. 设置动子参数
                    bool success = true;

                    // 发送目标位置 (假设PLC有专门的目标位置寄存器)
                    int posRegister = ModbusRegisters::MoverStatus::BASE_ADDRESS +
                                      moverId * ModbusRegisters::MoverStatus::REGISTERS_PER_MOVER +
                                      ModbusRegisters::MoverStatus::TARGET_LOW;

                    qint32 positionValue = static_cast<qint32>(targetPos * 1000);
                    success &= m_modbusManager->writeHoldingRegisterDINT(posRegister, positionValue);
                    success &= m_modbusManager->setSingleAxisAutoSpeed(static_cast<qint32>(targetSpeed)); // 使用新接口

                    if (success) {
                        // 2. 更新本地数据
                        MoverData &mover = (*m_movers)[moverId];
                        mover.target = targetPos;
                        mover.commandedTarget = targetPos;
                        mover.targetSpeed = targetSpeed;
                        mover.commandedSpeed = targetSpeed;
                        mover.targetAcceleration = targetAccel;
                        mover.lastCommand = QString("配方:%1").arg(recipe.name);
                        mover.lastCommandTime = QDateTime::currentDateTime();
                        mover.status = "运行中";

                        addLogEntry(QString("PLC - 动子%1: 位置=%2mm, 速度=%3mm/s, 加速度=%4mm/s²")
                                        .arg(moverId).arg(targetPos).arg(targetSpeed).arg(targetAccel), "success");

                        // 3. 处理延时
                        if (delay > 0) {
                            QThread::msleep(delay);  // 简单延时，实际应用中建议使用QTimer
                            addLogEntry(QString("动子%1 延时 %2ms").arg(moverId).arg(delay), "info");
                        }
                    } else {
                        allCommandsSent = false;
                        addLogEntry(QString("动子%1 PLC配置发送失败").arg(moverId), "error");
                    }
                } else {
                    allCommandsSent = false;
                    addLogEntry(QString("动子%1 索引无效").arg(moverId), "error");
                }
            }

            if (allCommandsSent) {
                addLogEntry(QString("配方 '%1' 已成功发送到PLC").arg(recipe.name), "success");

                // 4. 发送启动命令 (如果PLC需要)
                if (m_modbusManager && m_modbusManager->setSingleAxisEnable(true)) {
                    addLogEntry("PLC系统启动命令已发送", "success");
                }

                QMessageBox::information(this, "应用成功",
                                         QString("配方 '%1' 已成功应用到PLC。\n动子将开始按配方运动。").arg(recipe.name));

                emit recipeApplied(recipe.id, recipe.name);
            } else {
                QMessageBox::warning(this, "应用失败",
                                     QString("配方 '%1' 应用过程中发生错误！\n请检查PLC连接和日志。").arg(recipe.name));
            }
        } else {
            // 模拟模式：直接更新本地动子状态
            addLogEntry("模拟模式 - 开始应用配方数据...", "info");

            bool allConfigsApplied = true;

            // 逐个应用动子配置
            for (const QJsonObject &config : recipe.moverConfigs) {
                int moverId = config["id"].toInt();
                double targetPos = config["position"].toDouble();
                double targetSpeed = config["speed"].toDouble();
                double targetAccel = config["acceleration"].toDouble();
                int delay = config["delay"].toInt();

                if (moverId >= 0 && moverId < m_movers->size()) {
                    try {
                        // 更新本地动子数据
                        MoverData &mover = (*m_movers)[moverId];
                        mover.target = targetPos;
                        mover.targetSpeed = targetSpeed;
                        mover.acceleration = targetAccel;
                        mover.targetAcceleration = targetAccel;
                        mover.status = "运行中";
                        mover.inPosition = false;
                        mover.hasError = false;
                        mover.speed = targetSpeed;
                        mover.lastCommand = QString("配方:%1").arg(recipe.name);
                        mover.lastCommandTime = QDateTime::currentDateTime();

                        // 重置命令状态
                        mover.commandedTarget = targetPos;
                        mover.commandedSpeed = targetSpeed;

                        // *** 添加调试输出 ***
                        qDebug() << QString("配方应用 - 动子%1: 当前位置=%2, 目标位置=%3, 速度=%4, 状态=%5")
                                        .arg(moverId).arg(mover.position, 0, 'f', 1)
                                        .arg(mover.target, 0, 'f', 1).arg(mover.targetSpeed, 0, 'f', 1).arg(mover.status);

                        addLogEntry(QString("模拟 - 动子%1: 位置=%2mm, 速度=%3mm/s, 加速度=%4mm/s²")
                                        .arg(moverId).arg(targetPos).arg(targetSpeed).arg(targetAccel), "success");

                        // 处理延时（在模拟模式中可以忽略或者用不同方式处理）
                        if (delay > 0) {
                            addLogEntry(QString("模拟 - 动子%1 配置延时 %2ms").arg(moverId).arg(delay), "info");
                            // 在模拟模式中，我们可以设置一个延迟启动时间而不是阻塞线程
                            mover.lastCommandTime = mover.lastCommandTime.addMSecs(delay);
                        }

                    } catch (const std::exception& e) {
                        allConfigsApplied = false;
                        addLogEntry(QString("动子%1 模拟配置异常: %2").arg(moverId).arg(e.what()), "error");
                    } catch (...) {
                        allConfigsApplied = false;
                        addLogEntry(QString("动子%1 模拟配置发生未知异常").arg(moverId), "error");
                    }
                } else {
                    allConfigsApplied = false;
                    addLogEntry(QString("动子%1 索引无效，配置跳过").arg(moverId), "error");
                }
            }

            if (allConfigsApplied) {
                addLogEntry(QString("模拟模式 - 配方 '%1' 已成功应用").arg(recipe.name), "success");

                QMessageBox::information(this, "应用成功",
                                         QString("配方 '%1' 已成功应用到模拟系统。\n动子将开始按配方运动。").arg(recipe.name));

                emit recipeApplied(recipe.id, recipe.name);
            } else {
                QMessageBox::warning(this, "应用失败",
                                     QString("配方 '%1' 在模拟模式下应用过程中发生错误！\n请检查日志获取详细信息。").arg(recipe.name));
            }
        }
    }
}
void RecipeManagerPage::onExportRecipe()
{
    if (m_currentRecipeIndex < 0 || m_currentRecipeIndex >= m_recipes.size()) {
        QMessageBox::warning(this, "导出失败", "请先选择要导出的配方！");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "导出配方",
                                                    m_recipes[m_currentRecipeIndex].name + ".json",
                                                    "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            const Recipe &recipe = m_recipes[m_currentRecipeIndex];

            QJsonObject recipeObj;
            recipeObj["name"] = recipe.name;
            recipeObj["description"] = recipe.description;
            recipeObj["createTime"] = recipe.createTime;
            recipeObj["modifyTime"] = recipe.modifyTime;

            QJsonArray configArray;
            for (const QJsonObject &config : recipe.moverConfigs) {
                configArray.append(config);
            }
            recipeObj["moverConfigs"] = configArray;

            QJsonDocument doc(recipeObj);
            file.write(doc.toJson());
            file.close();

            addLogEntry(QString("配方已导出到: %1").arg(fileName), "success");
        }
    }
}

void RecipeManagerPage::onImportRecipe()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "导入配方",
                                                    "",
                                                    "JSON Files (*.json)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);

            if (doc.isObject()) {
                QJsonObject recipeObj = doc.object();

                Recipe recipe;
                recipe.name = recipeObj["name"].toString();
                recipe.description = recipeObj["description"].toString();
                recipe.createTime = recipeObj["createTime"].toString();
                recipe.modifyTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

                QJsonArray configArray = recipeObj["moverConfigs"].toArray();
                for (int i = 0; i < configArray.size(); ++i) {
                    const QJsonValue &value = configArray[i];
                    recipe.moverConfigs.append(value.toObject());
                }

                m_recipes.append(recipe);
                m_recipeList->addItem(recipe.name);

                addLogEntry(QString("配方 '%1' 已导入").arg(recipe.name), "success");
            }
            file.close();
        }
    }
}

void RecipeManagerPage::onRecipeNameChanged()
{
    m_isModified = true;
}


