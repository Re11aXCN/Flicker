#ifndef FK_BOXSHADOWEFFECT_H_
#define FK_BOXSHADOWEFFECT_H_

#include <QGraphicsEffect>
#include "NXDef.h"
class NXBoxShadowEffectPrivate;
class NXBoxShadowEffect : public QGraphicsEffect
{
    Q_OBJECT
    Q_Q_CREATE(NXBoxShadowEffect)
    Q_PRIVATE_CREATE_COMPLEX_H(QColor, LightColor)
    Q_PRIVATE_CREATE_COMPLEX_H(QColor, DarkColor)
    Q_PRIVATE_CREATE_COMPLEX_H(NXWidgetType::BoxShadow::RotateMode, RotateMode)
    Q_PRIVATE_CREATE_COMPLEX_H(NXWidgetType::BoxShadow::ProjectionType, ProjectionType)
    Q_PRIVATE_CREATE_COMPLEX_H(qreal, Blur)
    Q_PRIVATE_CREATE_COMPLEX_H(qreal, Spread)
    Q_PRIVATE_CREATE_COMPLEX_H(QPointF, LightOffset)
    Q_PRIVATE_CREATE_COMPLEX_H(QPointF, DarkOffset)
public:
    explicit NXBoxShadowEffect(QObject* parent = nullptr);
    ~NXBoxShadowEffect();
protected:
    QRectF boundingRectFor(const QRectF& rect) const override;
    void draw(QPainter* painter) override;
};
#include <QObject>

class NXBoxShadowEffect;
class NXBoxShadowEffectPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(NXBoxShadowEffect)
    Q_PROPERTY_CREATE_D(qreal, Blur)
    Q_PROPERTY_CREATE_D(qreal, Spread)
    Q_PROPERTY_CREATE_D(QColor, LightColor)
    Q_PROPERTY_CREATE_D(QColor, DarkColor)
    Q_PROPERTY_CREATE_D(QPointF, LightOffset)
    Q_PROPERTY_CREATE_D(QPointF, DarkOffset)
    Q_PROPERTY_CREATE_D(NXWidgetType::BoxShadow::RotateMode, RotateMode)
    Q_PROPERTY_CREATE_D(NXWidgetType::BoxShadow::ProjectionType, ProjectionType)
public:
    explicit NXBoxShadowEffectPrivate(QObject* parent = nullptr);
    ~NXBoxShadowEffectPrivate();
private:
    void _drawInsetShadow(QPainter* painter, const QPixmap& pixmap, const QPoint& pos);
    void _drawOutsetShadow(QPainter* painter, const QPixmap& pixmap, const QPoint& pos);

    NXThemeType::ThemeMode _themeMode{ NXThemeType::ThemeMode::Light };
};

#endif // !FK_BOXSHADOWEFFECT_H_
