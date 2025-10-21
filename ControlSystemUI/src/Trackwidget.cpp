// TrackWidget.cpp 轨道视图绘制
#include "TrackWidget.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <qmath.h>
#include <QScrollArea>

// 实际轨道尺寸 (mm)
const double TrackWidget::TRACK_LENGTH = 7455.75;  // 2段直线(2000*2) + 2段半圆(π*550*2) ≈ 7455.75mm
const double TrackWidget::STRAIGHT_LENGTH = 2000.0;  // 2米 = 2000mm
const double TrackWidget::RADIUS = 550.0;           // 直径1.1米 = 半径550mm

TrackWidget::TrackWidget(QWidget *parent)
    : QWidget(parent)
    , m_zoomFactor(1.0)
{
    setMinimumSize(700, 450);
    setStyleSheet("background-color: #1F2937; border: 2px solid #374151; border-radius: 8px;");
    //m_originalSize = size();
}

void TrackWidget::updateMovers(const QList<MoverData> &movers)
{
    m_movers = movers;
    // 添加调试信息
    for (int i = 0; i < m_movers.size(); ++i) {
        auto& mover = m_movers[i];
        if (mover.status == "紧急停止") {
            qDebug() << QString("TrackWidget收到动子%1急停状态").arg(mover.id);
        }
    }
    update();  // 触发重绘
}

void TrackWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 计算绘制区域
    QRect drawRect = rect().adjusted(50, 50, -50, -50);
    double centerX = drawRect.center().x();
    double centerY = drawRect.center().y();

    // 缩放比例 - 适应实际轨道尺寸
    double baseSize = qMin(width(), height());
    double scale = (baseSize / 2800.0) * m_zoomFactor;

    // 缩放后的轨道参数
    double scaledLength = STRAIGHT_LENGTH * scale;
    double scaledRadius = RADIUS * scale;

    // 绘制轨道背景网格
    painter.setPen(QPen(QColor(55, 65, 81, 60), 1));
    for (int i = 0; i < drawRect.width(); i += 30) {
        painter.drawLine(drawRect.left() + i, drawRect.top(),
                         drawRect.left() + i, drawRect.bottom());
    }
    for (int i = 0; i < drawRect.height(); i += 30) {
        painter.drawLine(drawRect.left(), drawRect.top() + i,
                         drawRect.right(), drawRect.top() + i);
    }

    // 绘制轨道阴影
    QPen shadowPen(QColor(0, 0, 0, 100), 8 * scale);
    painter.setPen(shadowPen);
    double shadowOffset = 2 * scale;

    painter.drawLine(centerX - scaledLength/2 + shadowOffset, centerY - scaledRadius + shadowOffset,
                     centerX + scaledLength/2 + shadowOffset, centerY - scaledRadius + shadowOffset);
    painter.drawLine(centerX - scaledLength/2 + shadowOffset, centerY + scaledRadius + shadowOffset,
                     centerX + scaledLength/2 + shadowOffset, centerY + scaledRadius + shadowOffset);

    // 绘制轨道背景（虚线）
    QPen bgPen(QColor(75, 85, 99, 150), 10 * scale);
    bgPen.setStyle(Qt::DashLine);
    painter.setPen(bgPen);

    // 上下直线段背景
    painter.drawLine(centerX - scaledLength/2, centerY - scaledRadius,
                     centerX + scaledLength/2, centerY - scaledRadius);
    painter.drawLine(centerX - scaledLength/2, centerY + scaledRadius,
                     centerX + scaledLength/2, centerY + scaledRadius);

    // 左右半圆背景
    QRectF leftBgCircle(centerX - scaledLength/2 - scaledRadius,
                        centerY - scaledRadius,
                        scaledRadius * 2, scaledRadius * 2);
    painter.drawArc(leftBgCircle, 90 * 16, 180 * 16);

    QRectF rightBgCircle(centerX + scaledLength/2 - scaledRadius,
                         centerY - scaledRadius,
                         scaledRadius * 2, scaledRadius * 2);
    painter.drawArc(rightBgCircle, 270 * 16, 180 * 16);

    // 绘制主轨道（实线）
    QPen trackPen(QColor(156, 163, 175), 6 * scale);
    painter.setPen(trackPen);

    painter.drawLine(centerX - scaledLength/2, centerY - scaledRadius,
                     centerX + scaledLength/2, centerY - scaledRadius);
    painter.drawLine(centerX - scaledLength/2, centerY + scaledRadius,
                     centerX + scaledLength/2, centerY + scaledRadius);

    QRectF leftCircle(centerX - scaledLength/2 - scaledRadius,
                      centerY - scaledRadius,
                      scaledRadius * 2, scaledRadius * 2);
    painter.drawArc(leftCircle, 90 * 16, 180 * 16);

    QRectF rightCircle(centerX + scaledLength/2 - scaledRadius,
                       centerY - scaledRadius,
                       scaledRadius * 2, scaledRadius * 2);
    painter.drawArc(rightCircle, 270 * 16, 180 * 16);

    // 绘制距离刻度标记（优化位置避免重叠）
    painter.setPen(QPen(QColor(107, 114, 128), 2));
    painter.setBrush(QBrush(QColor(107, 114, 128)));

    QList<double> majorMarks = {0, 1000, 2000, 3000, 4000, 5000, 6000, 7000};

    for (double pos : majorMarks) {
        if (pos >= TRACK_LENGTH) continue;


        // 1. 获取以(0,0)为中心的物理坐标
        QPointF point = getTrackPosition(pos);

        // 2. 将其缩放并平移到屏幕中心 (替换旧的坐标转换)
        point = point * scale + QPointF(centerX, centerY);
        // 刻度标记点
        painter.drawEllipse(point, 6 * scale, 6 * scale);

        // 距离标签 - 放在轨道外侧
        QString distText = QString("%1").arg((int)pos);
        QFont markFont = painter.font();
        markFont.setPointSize(10);
        markFont.setBold(true);
        painter.setFont(markFont);

        QFontMetrics fm(markFont);
        QRect textRect = fm.boundingRect(distText);

        // 根据位置决定标签位置（避开动子区域）
        QPointF labelPos;
        if (pos == 3000) {
            // 处理 "3000"，将其移动到标记点的左侧
            labelPos = QPointF(point.x() - 180 * scale, point.y());
        } else if (pos <= 2000 || pos > 3000) {
            // 处理 0, 1000,2000, 4000 (轨道下方的标签)
            labelPos = QPointF(point.x(), point.y() + 70 * scale);
        }

        textRect.moveCenter(labelPos.toPoint());

        // 标签背景
        painter.setBrush(QBrush(QColor(55, 65, 81, 200)));
        painter.setPen(QPen(QColor(107, 114, 128), 1));
        painter.drawRoundedRect(textRect.adjusted(-4, -2, 4, 2), 4, 4);

        painter.setPen(QPen(QColor(96, 165, 250)));
        painter.drawText(textRect, Qt::AlignCenter, distText);
    }

    // 绘制动子
    for (int i = 0; i < m_movers.size(); ++i) {
        const MoverData &mover = m_movers[i];
        const double moverRadius = 20.0; //动子半径

        QPointF currentPos = getTrackPosition(mover.position);
        currentPos = currentPos * scale + QPointF(centerX, centerY);

        // 绘制目标位置（如果不同于当前位置）
        if (qAbs(mover.target - mover.position) > 5.0) {
            QPointF targetPos = getTrackPosition(mover.target);
            targetPos = targetPos * scale + QPointF(centerX, centerY);
            // 目标位置指示圆
            painter.setBrush(QBrush(QColor(59, 130, 246, 100)));
            painter.setPen(QPen(QColor(59, 130, 246), 2 * scale, Qt::DashLine));
            painter.drawEllipse(targetPos, 12 * scale, 12 * scale);

            // 目标位置标签
            painter.setPen(QPen(QColor(59, 130, 246)));
            QFont targetFont = painter.font();
            targetFont.setPointSize(7);
            targetFont.setBold(true);
            painter.setFont(targetFont);
            painter.drawText(targetPos.x() - 15 * scale, targetPos.y() - 15 * scale, "目标");

            // 绘制从当前位置到目标位置的连接线
            painter.setPen(QPen(QColor(59, 130, 246, 150), 2 * scale, Qt::DotLine));
            painter.drawLine(currentPos, targetPos);
        }

        // 动子颜色
        QColor moverColor;
        if (mover.status == "紧急停止") {
            moverColor = QColor(239, 68, 68);  // 红色 - 紧急停止优先
        } else if (mover.isSelected) {
            moverColor = QColor(59, 130, 246);  // 蓝色 - 选中状态
        } else if (mover.status == "运行中") {
            moverColor = QColor(16, 185, 129);  // 绿色 - 运行中
        } else {
            moverColor = QColor(107, 114, 128);  // 灰色 - 其他状态
        }
        // 动子阴影
        painter.setBrush(QBrush(QColor(0, 0, 0, 80)));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(currentPos + QPointF(1.5 * scale, 1.5 * scale), 14 * scale, 14 * scale);

        // 动子主体
        painter.setBrush(QBrush(moverColor));
        painter.setPen(QPen(Qt::white, 2 * scale));
        painter.drawEllipse(currentPos, 25 * scale, 25 * scale);

        // 如果是紧急停止状态，添加闪烁的红色边框
        if (mover.status == "紧急停止") {
            // 创建一个更粗的红色边框
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(239, 68, 68), 4 * scale));
            painter.drawEllipse(currentPos, 28 * scale, 28 * scale);

            // 可选：添加警告图标或文字
            painter.setPen(QPen(Qt::white));
            QFont warningFont = painter.font();
            warningFont.setBold(true);
            warningFont.setPointSize(10);
            painter.setFont(warningFont);
            painter.drawText(currentPos.x() - 5 * scale, currentPos.y() + 3 * scale, "!");
        }

        // 动子内圈
        painter.setBrush(QBrush(moverColor.lighter(130)));
        painter.setPen(QPen(moverColor.darker(120), 1));
        painter.drawEllipse(currentPos, 8 * scale, 8 * scale);

        // 动子编号
        painter.setPen(QPen(Qt::white));
        QFont moverFont = painter.font();
        moverFont.setBold(true);
        moverFont.setPointSize(9);
        painter.setFont(moverFont);

        // 1. 定义编号文字
        QString idText = QString("M%1").arg(mover.id);

        // 2. 计算文字的边界矩形
        QFontMetrics idFm(moverFont);
        QRect idRect = idFm.boundingRect(idText);

        // 3. 将矩形中心移动到动子正上方
        //    下方位置标签的Y坐标是 currentPos.y() + 22 * scale
        //    用对称的负值将其移动到上方
        idRect.moveCenter(QPoint(currentPos.x(), currentPos.y() - 80 * scale));

        // 4. 为编号添加一个与下方位置标签类似的背景，以提高可读性
        painter.setBrush(QBrush(QColor(0, 0, 0, 180)));
        painter.setPen(QPen(moverColor, 1));
        painter.drawRoundedRect(idRect.adjusted(-4, -2, 4, 2), 3, 3);

        // 5. 在计算好的矩形内居中绘制文字
        painter.setPen(QPen(QColor(255, 255, 255)));
        painter.drawText(idRect, Qt::AlignCenter, idText);

        // 优化的位置显示
        QString posText = QString("%1mm").arg((int)mover.position);
        QFont posFont = painter.font();
        posFont.setBold(false);
        posFont.setPointSize(8);
        painter.setFont(posFont);

        QFontMetrics fm(posFont);
        QRect posRect = fm.boundingRect(posText);
        posRect.moveCenter(QPoint(currentPos.x(), currentPos.y() - 200 * scale));

        // 位置标签背景
        painter.setBrush(QBrush(QColor(0, 0, 0, 180)));
        painter.setPen(QPen(moverColor, 1));
        painter.drawRoundedRect(posRect.adjusted(-4, -2, 4, 2), 3, 3);

        painter.setPen(QPen(QColor(255, 255, 255)));
        painter.drawText(posRect, Qt::AlignCenter, posText);

        // 优化的选中效果（更小的外圈）
        if (mover.isSelected) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(59, 130, 246), 2 * scale, Qt::DashLine));
            painter.drawEllipse(currentPos, 18 * scale, 18 * scale);  // 减小选中圈
        }

        // 速度指示器
        if (mover.speed > 0) {
            painter.setPen(QPen(moverColor, 3 * scale));

            // 计算运动方向
            QPointF targetPoint = getTrackPosition(mover.target);
            targetPoint.setX(centerX + (targetPoint.x() - 1200) * scale);
            targetPoint.setY(centerY + (targetPoint.y() - 200) * scale);

            QPointF direction = targetPoint - currentPos;
            double length = sqrt(direction.x() * direction.x() + direction.y() * direction.y());
            if (length > 0) {
                direction = direction / length;

                // 速度比例指示（箭头长度）
                double speedRatio = mover.speed / mover.targetSpeed;
                double arrowLength = 15 * scale * speedRatio;

                QPointF arrowStart = currentPos + direction * 10 * scale;
                QPointF arrowEnd = arrowStart + direction * arrowLength;

                painter.drawLine(arrowStart, arrowEnd);

                // 箭头头部
                QPointF perpendicular(-direction.y(), direction.x());
                QPointF head1 = arrowEnd - direction * 5 * scale + perpendicular * 3 * scale;
                QPointF head2 = arrowEnd - direction * 5 * scale - perpendicular * 3 * scale;
                painter.drawLine(arrowEnd, head1);
                painter.drawLine(arrowEnd, head2);
            }
        }
    }

    // 绘制轨道信息面板
    int infoX = 20;
    int infoY = 30;
    int padding = 10; // 面板文字的内边距

    // 1. 准备要显示的文字和字体
    QFont titleFont = painter.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);

    QFont infoFont = painter.font();
    infoFont.setBold(false);
    infoFont.setPointSize(9);

    QStringList infoLines;
    infoLines << QString("总长度: %1 mm").arg(TRACK_LENGTH, 0, 'f', 1);
    infoLines << QString("直线段: %1 mm").arg(STRAIGHT_LENGTH, 0, 'f', 0);
    infoLines << QString("半圆直径: %1 mm").arg(RADIUS * 2, 0, 'f', 0);
    infoLines << QString("缩放比例: 1:%1").arg(1000.0 / scale, 0, 'f', 0);

    // 2. 动态计算所需的最大宽度
    QFontMetrics titleFm(titleFont);
    QFontMetrics infoFm(infoFont);
    int maxTextWidth = titleFm.horizontalAdvance("轨道参数");
    for (int i = 0; i < infoLines.size(); ++i) {
        const QString &line = infoLines[i];
        maxTextWidth = qMax(maxTextWidth, infoFm.horizontalAdvance(line));
    }

    // 3. 动态计算面板的尺寸
    int panelWidth = maxTextWidth + padding * 2;
    int panelHeight = titleFm.height() + (infoFm.height() * infoLines.size()) + padding * 2;

    // 4. 使用计算出的新尺寸来绘制面板背景
    painter.setBrush(QBrush(QColor(55, 65, 81, 220)));
    painter.setPen(QPen(QColor(107, 114, 128), 1));
    painter.drawRoundedRect(infoX - padding, infoY - padding, panelWidth, panelHeight, 5, 5);

    // 5. 绘制标题
    painter.setPen(QPen(QColor(96, 165, 250)));
    painter.setFont(titleFont);
    painter.drawText(infoX, infoY + titleFm.ascent() - padding/2, "轨道参数");

    // 6. 绘制参数信息
    painter.setPen(QPen(QColor(229, 231, 235)));
    painter.setFont(infoFont);
    int currentY = infoY + titleFm.height();
    for (int i = 0; i < infoLines.size(); ++i) {
        const QString &line = infoLines[i];
        painter.drawText(infoX, currentY + infoFm.ascent(), line);
        currentY += infoFm.height();
    }

    // 方向指示
    painter.setPen(QPen(QColor(16, 185, 129), 8 * scale));
    double arrowY = centerY + scaledRadius + 200 * scale;
    double arrowStartX = centerX - 50 * scale;
    double arrowEndX = centerX + 50 * scale;

    painter.drawLine(arrowStartX, arrowY, arrowEndX, arrowY);
    painter.drawLine(arrowEndX - 6 * scale, arrowY - 4 * scale, arrowEndX, arrowY);
    painter.drawLine(arrowEndX - 6 * scale, arrowY + 4 * scale, arrowEndX, arrowY);

    painter.setPen(QPen(QColor(16, 185, 129)));
    QFont arrowFont = painter.font();
    arrowFont.setPointSize(12);
    arrowFont.setBold(true);
    painter.setFont(arrowFont);
    painter.drawText(arrowStartX - 60 * scale, arrowY + 150 * scale, "运动方向");
}

QPointF TrackWidget::getTrackPosition(double position)
{
    // 归一化位置
    double normalizedPos = position;
    while (normalizedPos < 0) normalizedPos += TRACK_LENGTH;
    while (normalizedPos >= TRACK_LENGTH) normalizedPos -= TRACK_LENGTH;

    // 轨道段长度定义
    double segment1_End = STRAIGHT_LENGTH;                   // 下方直线段
    double segment2_End = segment1_End + M_PI * RADIUS;      // 右半圆
    double segment3_End = segment2_End + STRAIGHT_LENGTH;    // 上方直线段

    if (normalizedPos <= segment1_End) {
        // 下方直线段: y固定在+R, x从-L/2变化到+L/2
        double progress = normalizedPos / STRAIGHT_LENGTH;
        return QPointF(-STRAIGHT_LENGTH / 2 + progress * STRAIGHT_LENGTH,
                       RADIUS);
    } else if (normalizedPos <= segment2_End) {
        // 右半圆: 中心在(+L/2, 0)
        double arcProgress = (normalizedPos - segment1_End) / (M_PI * RADIUS);
        double angle = M_PI / 2 - M_PI * arcProgress; // 从90度到-90度
        return QPointF(STRAIGHT_LENGTH / 2 + RADIUS * cos(angle),
                       0 + RADIUS * sin(angle));
    } else if (normalizedPos <= segment3_End) {
        // 上方直线段: y固定在-R, x从+L/2变化到-L/2
        double progress = (normalizedPos - segment2_End) / STRAIGHT_LENGTH;
        return QPointF(STRAIGHT_LENGTH / 2 - progress * STRAIGHT_LENGTH,
                       -RADIUS);
    } else {
        // 左半圆: 中心在(-L/2, 0)
        double arcProgress = (normalizedPos - segment3_End) / (M_PI * RADIUS);
        double angle = -M_PI / 2 - M_PI * arcProgress; // 从-90度到-270度
        return QPointF(-STRAIGHT_LENGTH / 2 + RADIUS * cos(angle),
                       0 + RADIUS * sin(angle));
    }
}

void TrackWidget::zoomIn()
{
    // 找到父滚动区域并禁用其自动缩放
    if (auto scrollArea = qobject_cast<QScrollArea*>(parentWidget()->parentWidget())) {
        scrollArea->setWidgetResizable(false); // 禁用自动缩放
    }
    m_zoomFactor *= 1.25;
    updateGeometry(); // 通知布局系统尺寸提示已改变
    update();
}

void TrackWidget::zoomOut()
{
    if (auto scrollArea = qobject_cast<QScrollArea*>(parentWidget()->parentWidget())) {
        scrollArea->setWidgetResizable(false);
    }
    m_zoomFactor /= 1.25;
    if (m_zoomFactor < 0.2) m_zoomFactor = 0.2;
    updateGeometry();
    update();
}

void TrackWidget::resetZoom()
{
    m_zoomFactor = 1.0;
    if (auto scrollArea = qobject_cast<QScrollArea*>(parentWidget()->parentWidget())) {
        scrollArea->setWidgetResizable(false);
    }
    emit viewReset(); // 发送信号，让父控件处理后续
    updateGeometry();
    update();
}


// --- 实现滚轮事件处理函数，用于鼠标滚轮缩放 ---

void TrackWidget::wheelEvent(QWheelEvent *event)
{
    // 根据滚轮滚动的方向来决定是放大还是缩小
    if (event->angleDelta().y() > 0) {
        zoomIn();
    } else {
        zoomOut();
    }
    event->accept();
}
