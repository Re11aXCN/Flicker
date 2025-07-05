#ifndef FK_SHADOWWIDGET_H_
#define FK_SHADOWWIDGET_H_
#include <QWidget>
#include "NXDef.h"

class NXBoxShadowEffect;
class FKShadowWidget : public QWidget {
	Q_OBJECT
	Q_PRIVATE_CREATE_COMPLEX_H(QColor, LightColor)
	Q_PRIVATE_CREATE_COMPLEX_H(QColor, DarkColor)
	Q_PRIVATE_CREATE_COMPLEX_H(NXWidgetType::BoxShadow::RotateMode, RotateMode)
	Q_PRIVATE_CREATE_COMPLEX_H(NXWidgetType::BoxShadow::ProjectionType, ProjectionType)
	Q_PRIVATE_CREATE_COMPLEX_H(qreal, Blur)
	Q_PRIVATE_CREATE_COMPLEX_H(qreal, Spread)
	Q_PRIVATE_CREATE_COMPLEX_H(QPointF, LightOffset)
	Q_PRIVATE_CREATE_COMPLEX_H(QPointF, DarkOffset)
public:
	enum Shape{
		Rect,
		Ellipse
	};
	explicit FKShadowWidget(QWidget* parent = nullptr);
	~FKShadowWidget() override = default;
	void setShape(FKShadowWidget::Shape shape);
	FKShadowWidget::Shape getShape() const;
protected:
	void paintEvent(QPaintEvent* event) override;
private:
	FKShadowWidget::Shape _pShape;
	NXBoxShadowEffect* _pShadowEffect;
};

#endif // !FK_SHADOWWIDGET_H_
