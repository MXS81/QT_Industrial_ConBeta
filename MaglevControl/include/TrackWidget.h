/********************************************************************************
* Copyright (h) 2025, 沈阳盛科祝融科技有限公司
* Date: 2025-8-12
* Description: 磁浮环线仿真
*********************************************************************************/
#ifndef TRACKWIDGET_H
#define TRACKWIDGET_H

#include <QWidget>
#include <QList>
#include <QWheelEvent>
#include <QShowEvent>
#include <QString>
#include <QColor>

// 动子数据结构
struct MoverData {
    int id;                    // 动子ID
    double position;           // 当前位置 (mm)
    double target;             // 目标位置 (mm)
    QString status;            // 状态（如"紧急停止"、"运行中"、"停止"等）
    bool isSelected;           // 是否选中
    double speed;              // 当前速度
    double targetSpeed;        // 目标速度
    
    // 构造函数
    MoverData() : id(0), position(0.0), target(0.0), status("停止"), 
                  isSelected(false), speed(0.0), targetSpeed(0.0) {}
    
    MoverData(int _id, double _pos, double _target, const QString& _status = "停止", 
              bool _selected = false, double _speed = 0.0, double _targetSpeed = 0.0)
        : id(_id), position(_pos), target(_target), status(_status), 
          isSelected(_selected), speed(_speed), targetSpeed(_targetSpeed) {}
};

class TrackWidget : public QWidget
{
    Q_OBJECT

    // 主题自适应：通过 QSS 的 qproperty 在不同主题下注入配色，
    // 若未设置则在运行时根据 QPalette 推导，保证双通道适配（QSS / Palette）。
    Q_PROPERTY(QColor panelBackgroundColor READ panelBackgroundColor WRITE setPanelBackgroundColor)
    Q_PROPERTY(QColor panelBorderColor READ panelBorderColor WRITE setPanelBorderColor)
    Q_PROPERTY(QColor panelTitleColor READ panelTitleColor WRITE setPanelTitleColor)
    Q_PROPERTY(QColor panelTextColor READ panelTextColor WRITE setPanelTextColor)

public:
    explicit TrackWidget(QWidget *parent = nullptr);
    void updateMovers(const QList<MoverData> &movers);

    /**
     * 轨道信息面板颜色属性（可由 QSS 通过 qproperty 注入）。
     * 若未设置（QColor::isValid()==false），绘制时将回退到 QPalette 派生的颜色。
     */
    QColor panelBackgroundColor() const { return m_panelBackgroundColor; }
    void setPanelBackgroundColor(const QColor& c) { m_panelBackgroundColor = c; }

    QColor panelBorderColor() const { return m_panelBorderColor; }
    void setPanelBorderColor(const QColor& c) { m_panelBorderColor = c; }

    QColor panelTitleColor() const { return m_panelTitleColor; }
    void setPanelTitleColor(const QColor& c) { m_panelTitleColor = c; }

    QColor panelTextColor() const { return m_panelTextColor; }
    void setPanelTextColor(const QColor& c) { m_panelTextColor = c; }

public slots: // 公共槽函数，用于从外部控制缩放
    void zoomIn();
    void zoomOut();
    void resetZoom();

signals:
    void viewReset();

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    //void showEvent(QShowEvent *event) override;

private:
    QPointF getTrackPosition(double position);
    QList<MoverData> m_movers;
    //QSize m_originalSize;
    //bool m_isInitialShow = true;
    double m_zoomFactor = 2.3;
    static const double TRACK_LENGTH;
    static const double STRAIGHT_LENGTH;
    static const double RADIUS;

    // 可由 QSS 注入的主题色；默认 invalid，表示使用 QPalette 推导。
    QColor m_panelBackgroundColor;
    QColor m_panelBorderColor;
    QColor m_panelTitleColor;
    QColor m_panelTextColor;

};

#endif // TRACKWIDGET_H
