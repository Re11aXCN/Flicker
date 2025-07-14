#ifndef FK_PUSHBUTTON_H_
#define FK_PUSHBUTTON_H_
#include <QPropertyAnimation>
#include <NXPushButton.h>

class FKPushButton : public NXPushButton {
    Q_OBJECT
    Q_PROPERTY_CREATE(qreal, PressRadius)
    Q_PROPERTY_CREATE(qreal, HoverOpacity)
    Q_PROPERTY_CREATE(qreal, PressOpacity)
public:
    explicit FKPushButton(QWidget* parent = nullptr);
    explicit FKPushButton(const QString& text, QWidget* parent = nullptr);
    ~FKPushButton();

protected:
    virtual bool event(QEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;
private:
    qreal _getLongestDistance(QPoint point);
    qreal _distance(QPoint point1, QPoint point2);
    void _startHoverOpacityAnimation(bool isVisible);

    int _pShadowBorderWidth{ 6 };
    bool _pIsPressAnimationFinished{ true };
    QRadialGradient* _pHoverGradient{ nullptr };
    QRadialGradient* _pPressGradient{ nullptr };
};
#endif // !FK_PUSHBUTTON_H_