#include "MoverWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>

MoverWidget::MoverWidget(const MoverData &mover, QWidget *parent)
    : QWidget(parent)
    , m_moverData(mover)
{
    setFixedSize(150, 100);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    // 标题
    QHBoxLayout *titleLayout = new QHBoxLayout;
    m_idLabel = new QLabel(QString("动子 %1").arg(mover.id));
    m_idLabel->setStyleSheet("font-weight: bold; color: white;");

    titleLayout->addWidget(m_idLabel);
    titleLayout->addStretch();
    layout->addLayout(titleLayout);

    // 位置信息
    QHBoxLayout *posLayout = new QHBoxLayout;
    posLayout->addWidget(new QLabel("位置:"));
    m_positionLabel = new QLabel(QString("%1").arg(mover.position, 0, 'f', 0));
    m_positionLabel->setStyleSheet("color: white; font-family: monospace;");
    posLayout->addStretch();
    posLayout->addWidget(m_positionLabel);
    layout->addLayout(posLayout);

    // 速度信息
    QHBoxLayout *speedLayout = new QHBoxLayout;
    speedLayout->addWidget(new QLabel("速度:"));
    m_speedLabel = new QLabel(QString("%1").arg(mover.speed, 0, 'f', 1));
    m_speedLabel->setStyleSheet("color: white; font-family: monospace;");
    speedLayout->addStretch();
    speedLayout->addWidget(m_speedLabel);
    layout->addLayout(speedLayout);

    updateMover(mover);
}

void MoverWidget::updateMover(const MoverData &mover)
{
    m_moverData = mover;

    // 更新显示内容
    m_positionLabel->setText(QString("%1").arg(mover.position, 0, 'f', 0));
    m_speedLabel->setText(QString("%1").arg(mover.speed, 0, 'f', 1));

    // 更新选中状态
    if (mover.isSelected) {
        m_idLabel->setText(QString("动子 %1 ●").arg(mover.id));
        setStyleSheet("border: 2px solid #3B82F6; border-radius: 6px; background-color: rgba(59, 130, 246, 0.1);");
    } else {
        m_idLabel->setText(QString("动子 %1").arg(mover.id));
        setStyleSheet("border: 2px solid #4B5563; border-radius: 6px; background-color: #374151;");
    }

    // 状态颜色
    QString statusColor = "#6B7280";
    if (mover.status == "运行中") {
        statusColor = "#3B82F6";
    } else if (mover.status == "紧急停止") {
        statusColor = "#EF4444";
    }
    update();  // 触发重绘
}

void MoverWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit moverSelected(m_moverData.id);
    }
    QWidget::mousePressEvent(event);
}

void MoverWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    // 选中时绘制额外的边框效果
    if (m_moverData.isSelected) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(QColor(59, 130, 246), 1);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
    }
}
