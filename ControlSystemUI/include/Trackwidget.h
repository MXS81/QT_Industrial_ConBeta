#ifndef TRACKWIDGET_H
#define TRACKWIDGET_H

#include <QWidget>
#include <QList>
#include <QWheelEvent>
#include <QShowEvent>
#include "MoverData.h"

class TrackWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrackWidget(QWidget *parent = nullptr);
    void updateMovers(const QList<MoverData> &movers);

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
};

#endif // TRACKWIDGET_H
