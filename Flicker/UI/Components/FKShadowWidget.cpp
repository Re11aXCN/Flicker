#include "FKShadowWidget.h"

#include <QPainter>
#include <QPainterPath>
#include "FKConstant.h"
#include "Components/NXBoxShadowEffect.h"
FKShadowWidget::FKShadowWidget(QWidget* parent /*= nullptr*/)
	: QWidget(parent)
	, _pShape{ Shape::Rect }
{
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	setStyleSheet("background-color: transparent;");
	setFixedSize(200, 200);
	_pShadowEffect = new NXBoxShadowEffect(this);
	setGraphicsEffect(_pShadowEffect);
}

void FKShadowWidget::setShape(Shape shape)
{
	_pShape = shape;
	update();
}

FKShadowWidget::Shape FKShadowWidget::getShape() const
{
	return _pShape;
}


void FKShadowWidget::setDarkOffset(const QPointF& size) {
	_pShadowEffect->setDarkOffset(size);
	update();
}

QPointF FKShadowWidget::getDarkOffset() const {
	return _pShadowEffect->getDarkOffset();
}

void FKShadowWidget::setLightOffset(const QPointF& size) {
	_pShadowEffect->setLightOffset(size);
	update();
}

QPointF FKShadowWidget::getLightOffset() const {
	return _pShadowEffect->getLightOffset();
}

void FKShadowWidget::setRotateMode(const NXWidgetType::BoxShadow::RotateMode& mode) {
	_pShadowEffect->setRotateMode(mode);
	update();
}

NXWidgetType::BoxShadow::RotateMode FKShadowWidget::getRotateMode() const {
	return _pShadowEffect->getRotateMode();
}

void FKShadowWidget::setProjectionType(const NXWidgetType::BoxShadow::ProjectionType& type) {
	_pShadowEffect->setProjectionType(type);
	update();
}

NXWidgetType::BoxShadow::ProjectionType FKShadowWidget::getProjectionType() const {
	return _pShadowEffect->getProjectionType();
}

void FKShadowWidget::setBlur(const qreal& blur) {
	_pShadowEffect->setBlur(blur);
	update();
}

qreal FKShadowWidget::getBlur() const {
	return _pShadowEffect->getBlur();
}

void FKShadowWidget::setSpread(const qreal& spread) {
	_pShadowEffect->setSpread(spread);
	update();
}

qreal FKShadowWidget::getSpread() const {
	return _pShadowEffect->getSpread();
}

void FKShadowWidget::setLightColor(const QColor& color) {
	_pShadowEffect->setLightColor(color);
	update();
}

QColor FKShadowWidget::getLightColor() const {
	return _pShadowEffect->getLightColor();
}

void FKShadowWidget::setDarkColor(const QColor& color) {
	_pShadowEffect->setDarkColor(color);
	update();
}

QColor FKShadowWidget::getDarkColor() const {
	return _pShadowEffect->getDarkColor();
}

void FKShadowWidget::paintEvent(QPaintEvent* event)
{
	if (_pShadowEffect->getProjectionType() == NXWidgetType::BoxShadow::ProjectionType::Outset) {
		QPainter painter(this);
		painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
		painter.setPen(Qt::NoPen);
		painter.setBrush(Constant::LIGHT_MAIN_BG_COLOR);
		_pShape == Shape::Rect 
			? painter.drawRect(rect())
			: painter.drawEllipse(rect());
	}
	else {
		if (_pShape == Shape::Ellipse) {
			// 将坐标系原点移动到窗口中心附近，方便观察
			//painter.translate(100, 400);

			//QPainterPath rectPath;
			//rectPath.addRect(rect());

			//// 创建半圆路径（直径100，半径50，开口向上）
			//QPainterPath circlePath;
			//// 半圆定位：圆心在(50,120)，覆盖区域(0,70,100,100)
			//circlePath.moveTo(1, 151);          // 起点：左下角
			//circlePath.arcTo(QRectF(QPoint{1, 1}, QSize{300, 300}), 180, -360); // 从180°逆时针绘制到0°
			//circlePath.closeSubpath();          // 闭合路径形成半圆区域

			//// 从矩形中减去半圆形成凹槽
			//QPainterPath finalPath = rectPath.subtracted(circlePath);

			//// 绘制最终路径
			//painter.drawPath(finalPath);
		}
	}
	
	QWidget::paintEvent(event);
}

