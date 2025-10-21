// AnimatedButton.cpp
#include "AnimatedButton.h"
#include <QGraphicsOpacityEffect>

AnimatedButton::AnimatedButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
    , m_animationType(None)
    , m_animation(nullptr)
    , m_opacityEffect(nullptr)
    , m_animationTimer(new QTimer(this))
    , m_isAnimating(false)
{
    // 设置动画定时器
    m_animationTimer->setSingleShot(false);
    connect(m_animationTimer, &QTimer::timeout, this, &AnimatedButton::startAnimation);
}

AnimatedButton::~AnimatedButton()
{
    stopAnimation();
}

void AnimatedButton::setAnimationType(AnimationType type)
{
    if (m_animationType == type) return;

    // 安全地停止当前动画
    stopAnimation();
    m_animationType = type;

    switch (type) {
    case Pulse:
        setupPulseAnimation();
        break;
    case Blink:
        setupBlinkAnimation();
        break;
    case Glow:
        setupGlowAnimation();
        break;
    case None:
    default:
        // 确保完全停止并清理
        stopAnimation();
        break;
    }
}

void AnimatedButton::startAnimation()
{
    if (m_animationType == None || !m_animation) return;

    if (!m_isAnimating) {
        m_isAnimating = true;
        m_animation->start();
    }
}

void AnimatedButton::stopAnimation()
{
    m_isAnimating = false;

    // 停止定时器
    if (m_animationTimer) {
        m_animationTimer->stop();
    }

    // 停止并清理动画
    if (m_animation) {
        m_animation->stop();
        m_animation->deleteLater();
        m_animation = nullptr;
    }

    // 清理透明效果
    if (m_opacityEffect) {
        setGraphicsEffect(nullptr);
        m_opacityEffect = nullptr;  // 不需要手动删除，setGraphicsEffect(nullptr)会处理
    }
}

void AnimatedButton::setupPulseAnimation()
{
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);

    m_animation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_animation->setDuration(1000); // 1秒
    m_animation->setStartValue(1.0);
    m_animation->setEndValue(0.5);
    m_animation->setLoopCount(-1); // 无限循环
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);

    // 来回动画
    connect(m_animation, &QPropertyAnimation::finished, this, &AnimatedButton::onAnimationFinished);
}

void AnimatedButton::setupBlinkAnimation()
{
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);

    m_animation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_animation->setDuration(500); // 0.5秒
    m_animation->setStartValue(1.0);
    m_animation->setEndValue(0.0);
    m_animation->setLoopCount(-1);

    connect(m_animation, &QPropertyAnimation::finished, this, &AnimatedButton::onAnimationFinished);
}

void AnimatedButton::setupGlowAnimation()
{
    // 发光效果可以通过改变样式表实现
    m_animationTimer->setInterval(1000);
    connect(m_animationTimer, &QTimer::timeout, this, [this]() {
        static bool glowing = false;
        glowing = !glowing;

        if (glowing) {
            setStyleSheet("QPushButton { background-color: #ff1744; border: 2px solid #fff; }");
        } else {
            setStyleSheet("QPushButton { background-color: #e94560; border: none; }");
        }
    });

    m_animationTimer->start();
}

void AnimatedButton::onAnimationFinished()
{
    if (m_isAnimating && m_animation) {
        // 反向播放动画
        QPropertyAnimation::Direction direction = m_animation->direction();
        if (direction == QPropertyAnimation::Forward) {
            m_animation->setDirection(QPropertyAnimation::Backward);
        } else {
            m_animation->setDirection(QPropertyAnimation::Forward);
        }
        m_animation->start();
    }
}
