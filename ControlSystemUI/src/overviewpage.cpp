// src/overviewpage.cpp

#include "OverviewPage.h"
#include "TrackWidget.h"
#include "MoverWidget.h"
#include "LogWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QHeaderView>
#include <QScrollArea>
#include <QScrollBar>
#include <QDebug>

// 构造函数接收一个指向动子数据列表的指针
OverviewPage::OverviewPage(QList<MoverData>* movers, const QString& currentUser, QWidget *parent)
    : QWidget(parent)
    , m_movers(movers) // 初始化指针
    , m_currentUser(currentUser)
    , m_selectedMover(0) // 默认选中第一个动子
{
    setupUI();
    addLogEntry(QString("系统概览页面已加载 - 用户: %1").arg(m_currentUser), "success");
}

OverviewPage::~OverviewPage()
{
}

void OverviewPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // 顶部用户信息栏
    QWidget *topBar = new QWidget();
    topBar->setStyleSheet(R"(
        QWidget {
            background-color: #0f3460;
            border-radius: 5px;
            padding: 5px;
        }
    )");
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);

    m_userInfoLabel = new QLabel(QString("👤 当前操作员: %1").arg(m_currentUser));
    m_userInfoLabel->setStyleSheet("color: #00ff41; font-size: 14px; font-weight: bold;");
    m_recipeInfoLabel = new QLabel("当前配方: 无");
    m_recipeInfoLabel->setStyleSheet("color: white; font-size: 14px;");

    QLabel *titleLabel = new QLabel("系统概览");
    titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);

    topLayout->addWidget(m_userInfoLabel);
    topLayout->addStretch();
    topLayout->addWidget(m_recipeInfoLabel);
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    // 创建分割器
    QHBoxLayout *contentLayout = new QHBoxLayout();

    // 左侧：轨道可视化和状态表格
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    // 轨道可视化
    QGroupBox *trackGroup = new QGroupBox("轨道实时监控");
    trackGroup->setStyleSheet(R"(
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
    QVBoxLayout *trackLayout = new QVBoxLayout(trackGroup);

    m_trackWidget = new TrackWidget();
    m_trackScrollArea = new QScrollArea();
    m_trackScrollArea->setWidget(m_trackWidget);
    m_trackScrollArea->setWidgetResizable(false);
    m_trackScrollArea->setAlignment(Qt::AlignCenter);
    trackLayout->addWidget(m_trackScrollArea);

    QHBoxLayout *zoomLayout = new QHBoxLayout();
    QPushButton *zoomInBtn = new QPushButton("放大 (+)");
    QPushButton *zoomOutBtn = new QPushButton("缩小 (-)");
    QPushButton *resetZoomBtn = new QPushButton("重置缩放");

    QString zoomBtnStyle = "QPushButton { padding: 5px 10px; font-size: 11px; }";
    zoomInBtn->setStyleSheet(zoomBtnStyle);
    zoomOutBtn->setStyleSheet(zoomBtnStyle);
    resetZoomBtn->setStyleSheet(zoomBtnStyle);

    zoomLayout->addStretch();
    zoomLayout->addWidget(zoomInBtn);
    zoomLayout->addWidget(zoomOutBtn);
    zoomLayout->addWidget(resetZoomBtn);

    trackLayout->addLayout(zoomLayout);

    connect(zoomInBtn, &QPushButton::clicked, m_trackWidget, &TrackWidget::zoomIn);
    connect(zoomOutBtn, &QPushButton::clicked, m_trackWidget, &TrackWidget::zoomOut);
    connect(resetZoomBtn, &QPushButton::clicked, m_trackWidget, &TrackWidget::resetZoom);

    // 状态表格
    QGroupBox *tableGroup = new QGroupBox("动子状态详情");
    tableGroup->setStyleSheet(trackGroup->styleSheet());
    QVBoxLayout *tableLayout = new QVBoxLayout(tableGroup);

    m_statusTable = new QTableWidget(4, 4);
    m_statusTable->setHorizontalHeaderLabels({
        "ID", "位置(mm)", "目标(mm)", "速度(mm/s)",
    });
    m_statusTable->horizontalHeader()->setStretchLastSection(true);
    m_statusTable->setAlternatingRowColors(true);
    m_statusTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #16213e;
            color: white;
            gridline-color: #3a3a5e;
            border: 1px solid #3a3a5e;
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
    m_statusTable->setMaximumHeight(200);

    connect(m_statusTable, &QTableWidget::cellClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        onMoverSelected(row);
    });

    tableLayout->addWidget(m_statusTable);
    leftLayout->addWidget(trackGroup, 2);
    leftLayout->addWidget(tableGroup, 1);

    // 右侧：动子控件和日志
    QWidget *rightPanel = new QWidget();
    rightPanel->setMaximumWidth(400);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    QGroupBox *moverGroup = new QGroupBox("动子快速控制");
    moverGroup->setStyleSheet(trackGroup->styleSheet());
    m_moverGridLayout = new QGridLayout(moverGroup);

    createMoverWidgets();

    m_logWidget = new LogWidget("系统日志", this);
    m_logWidget->setMaxLines(500);
    m_logWidget->setShowUser(true);
    m_logWidget->setAutoScroll(true);
    rightLayout->addWidget(moverGroup);
    rightLayout->addWidget(m_logWidget);
    rightLayout->addStretch();

    contentLayout->addWidget(leftPanel, 2);
    contentLayout->addWidget(rightPanel, 1);

    connect(m_trackWidget, &TrackWidget::viewReset, this, [this]() {
        m_trackScrollArea->horizontalScrollBar()->setValue(0);
        m_trackScrollArea->verticalScrollBar()->setValue(0);
    });

    mainLayout->addWidget(topBar);
    mainLayout->addLayout(contentLayout);
}

void OverviewPage::createMoverWidgets()
{
    // 安全检查，确保指针有效
    if (!m_movers) {
        qWarning() << "createMoverWidgets: 动子列表指针无效!";
        return;
    }

    // 根据 MainWindow 传递过来的数据创建 MoverWidget
    for (int i = 0; i < m_movers->size(); ++i) {
        // 直接从指针获取数据
        MoverData& mover = (*m_movers)[i];
        mover.isSelected = (i == m_selectedMover); // 设置初始选中状态

        MoverWidget *widget = new MoverWidget(mover, this);
        connect(widget, &MoverWidget::moverSelected, this, &OverviewPage::onMoverSelected);
        m_moverWidgets.append(widget);
        m_moverGridLayout->addWidget(widget, i / 2, i % 2);
    }

    // 使用同样的数据初始化表格
    initializeTableWithData();
    qDebug() << "OverviewPage::createMoverWidgets 完成";
}

void OverviewPage::initializeTableWithData()
{
    // 安全检查
    if (!m_movers) {
        qWarning() << "initializeTableWithData: 动子列表指针无效!";
        return;
    }

    // 设置表格行数与动子数量一致
    m_statusTable->setRowCount(m_movers->size());

    for (int i = 0; i < m_movers->size(); ++i) {
        const MoverData& mover = (*m_movers)[i];

        // 确保表格单元格存在
        for (int j = 0; j < 7; ++j) {
            if (!m_statusTable->item(i, j)) {
                m_statusTable->setItem(i, j, new QTableWidgetItem());
            }
        }

        // 填充数据
        m_statusTable->item(i, 0)->setText(QString::number(mover.id));
        m_statusTable->item(i, 1)->setText(QString::number(mover.position, 'f', 1));
        m_statusTable->item(i, 2)->setText(QString::number(mover.target, 'f', 1));
        m_statusTable->item(i, 3)->setText(QString::number(mover.speed, 'f', 1));
    }
    // 默认选中第一行
    if (m_statusTable->rowCount() > 0) {
        m_statusTable->selectRow(0);
    }
}

void OverviewPage::updateMovers(const QList<MoverData> &movers)
{
    // 此函数的参数 movers 是由信号传递过来的，但我们现在直接使用指针 m_movers
    // 以确保数据源唯一。

    // 安全检查
    if (!m_movers || m_movers->isEmpty()) {
        qWarning() << "OverviewPage::updateMovers - 动子列表指针无效或列表为空。";
        return;
    }

    // 直接使用指针指向的最新数据来无条件更新UI
    if (m_trackWidget) {
        m_trackWidget->updateMovers(*m_movers);
    }
    if (m_statusTable) {
        updateTableData(*m_movers);
    }

    int moverCount = qMin(m_movers->size(), m_moverWidgets.size());
    for (int i = 0; i < moverCount; ++i) {
        if (m_moverWidgets[i]) {
            MoverData moverData = (*m_movers)[i];
            moverData.isSelected = (i == m_selectedMover);
            m_moverWidgets[i]->updateMover(moverData);
        }
    }
}

void OverviewPage::updateTableData(const QList<MoverData> &data)
{
    // 这个函数现在由 updateMovers 调用，参数 data 是 *m_movers
    m_statusTable->setRowCount(data.size());
    m_statusTable->blockSignals(true); // 更新期间禁止信号

    for (int i = 0; i < data.size(); ++i) {
        const MoverData &mover = data[i];

        // 确保单元格存在
        for (int j = 0; j < 4; ++j) {
            if (!m_statusTable->item(i, j)) {
                m_statusTable->setItem(i, j, new QTableWidgetItem());
            }
        }

        // 更新数据
        m_statusTable->item(i, 0)->setText(QString::number(mover.id));
        m_statusTable->item(i, 1)->setText(QString::number(mover.position, 'f', 1));
        m_statusTable->item(i, 2)->setText(QString::number(mover.target, 'f', 1));
        m_statusTable->item(i, 3)->setText(QString::number(mover.speed, 'f', 1));

        // 更新颜色和背景
        QColor bgColor = (i == m_selectedMover) ? QColor(83, 52, 131, 80) : QColor();
        for (int j = 0; j < 4; ++j) {
            m_statusTable->item(i, j)->setBackground(bgColor);
        }
    }
    m_statusTable->blockSignals(false); // 恢复信号
}

void OverviewPage::onMoverSelected(int id)
{
    if (!m_movers || id < 0 || id >= m_movers->size()) {
        qWarning() << "onMoverSelected: 无效的动子ID:" << id;
        return;
    }

    m_selectedMover = id;

    // 强制刷新所有UI以反映新的选中状态
    updateMovers(*m_movers);

    addLogEntry(QString("[%1] 选择了动子 %2").arg(m_currentUser).arg(id), "info");
}

void OverviewPage::onUserChanged(const QString& username)
{
    m_currentUser = username;
    if (m_userInfoLabel) {
        m_userInfoLabel->setText(QString("👤 当前操作员: %1").arg(m_currentUser));
    }
    addLogEntry(QString("用户切换为: %1").arg(username), "warning");
}

void OverviewPage::addLogEntry(const QString &message, const QString &type)
{
    if (m_logWidget) {
        m_logWidget->addLogEntry(message, type, m_currentUser);
    }
}
