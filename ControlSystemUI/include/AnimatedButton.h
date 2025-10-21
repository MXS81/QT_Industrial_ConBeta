#ifndef ANIMATEDBUTTON_H
#define ANIMATEDBUTTON_H

#include <QPushButton>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class AnimatedButton : public QPushButton
{
    Q_OBJECT

public:
    explicit AnimatedButton(const QString &text, QWidget *parent = nullptr);
    ~AnimatedButton();

    // 设置动画类型
    enum AnimationType {
        None,
        Pulse,
        Blink,
        Glow
    };

    void setAnimationType(AnimationType type);
    void startAnimation();
    void stopAnimation();

private slots:
    void onAnimationFinished();

private:
    void setupPulseAnimation();
    void setupBlinkAnimation();
    void setupGlowAnimation();

    AnimationType m_animationType;
    QPropertyAnimation *m_animation;
    QGraphicsOpacityEffect *m_opacityEffect;
    QTimer *m_animationTimer;
    bool m_isAnimating;
};

#endif // ANIMATEDBUTTON_H
