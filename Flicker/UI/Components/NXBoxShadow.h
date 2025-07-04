#ifndef NX_BOXSHADOW_H_
#define NX_BOXSHADOW_H_

#include <QGraphicsEffect>
#include "NXDef.h"
class NXBoxShadowPrivate;
class NXBoxShadow : public QGraphicsEffect
{
	Q_OBJECT
	Q_Q_CREATE(NXBoxShadow)
	Q_PRIVATE_CREATE_COMPLEX_H(QColor, LightColor)
	Q_PRIVATE_CREATE_COMPLEX_H(QColor, DarkColor)
	Q_PRIVATE_CREATE_Q_H(NXWidgetType::BoxShadow::RotateMode, RotateMode)
	Q_PRIVATE_CREATE_Q_H(NXWidgetType::BoxShadow::ProjectionType, ProjectionType)
	Q_PRIVATE_CREATE_Q_H(qreal, Blur)
	Q_PRIVATE_CREATE_Q_H(qreal, Spread)
	Q_PRIVATE_CREATE_Q_H(bool, CircleShadow)
public:
	explicit NXBoxShadow(QObject* parent = nullptr);
	~NXBoxShadow();
protected:
	QRectF boundingRectFor(const QRectF& rect) const override;
	void draw(QPainter* painter) override;
};
#include <QObject>

class NXBoxShadow;
class NXBoxShadowPrivate : public QObject
{
	Q_OBJECT
	Q_D_CREATE(NXBoxShadow)
	Q_PROPERTY_CREATE_D(qreal, Blur)
	Q_PROPERTY_CREATE_D(qreal, Spread)
	Q_PROPERTY_CREATE_D(QColor, LightColor)
	Q_PROPERTY_CREATE_D(QColor, DarkColor)
	Q_PROPERTY_CREATE_D(NXWidgetType::BoxShadow::RotateMode, RotateMode)
	Q_PROPERTY_CREATE_D(NXWidgetType::BoxShadow::ProjectionType, ProjectionType)
	Q_PROPERTY_CREATE_D(bool, CircleShadow)
public:
	explicit NXBoxShadowPrivate(QObject* parent = nullptr);
	~NXBoxShadowPrivate();
private:
	void _drawInsetShadow(QPainter* painter, const QPixmap& pixmap,
		const QPoint& pos, const QPoint& offset);
	void _drawOutsetShadow(QPainter* painter, const QPixmap& pixmap,
		const QPoint& pos, const QPoint& offset);
	void _drawCircleInsetShadow(QPainter* painter, const QPixmap& pixmap,
		const QPoint& pos, qreal spread, qreal blur);
	void _drawCircleOutsetShadow(QPainter* painter, const QPixmap& pixmap,
		const QPoint& pos, qreal spread, qreal blur);

	QImage _createBaseShadowImage(const QPixmap& pixmap, const QColor& color);
	QImage _applyBlur(const QImage& source, qreal blur, bool alphaOnly);
	QImage _applyColor(const QImage& image, const QColor& color);
	NXThemeType::ThemeMode _themeMode{ NXThemeType::ThemeMode::Light };
	qreal _radian{ 0.0 };
};

#endif // !NX_BOXSHADOW_H_
