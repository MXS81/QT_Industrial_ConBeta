#ifndef MOVERWIDGET_H
#define MOVERWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include "MoverData.h"

class MoverWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MoverWidget(const MoverData &mover, QWidget *parent = nullptr);
    void updateMover(const MoverData &mover);

signals:
    void moverSelected(int id);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    MoverData m_moverData;
    QLabel *m_idLabel;
    QLabel *m_positionLabel;
    QLabel *m_speedLabel;
};

#endif // MOVERWIDGET_H
