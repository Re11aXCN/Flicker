#include "FKPushButton.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <NXTheme.h>

#include "FKConstant.h"

FKPushButton::FKPushButton(QWidget* parent /*= nullptr*/)
	: NXPushButton(parent)
{
	setCursor(Qt::PointingHandCursor);
	setFixedSize(180, 56);
	setBorderRadius(28);
	setLightDefaultColor(NXThemeColor(NXThemeType::Light, PrimaryNormal));
	setLightHoverColor(NXThemeColor(NXThemeType::Light, PrimaryHover));
	setLightPressColor(NXThemeColor(NXThemeType::Light, PrimaryPress));
	setLightTextColor(Constant::LIGHT_TEXT_COLOR);

	setDarkDefaultColor(NXThemeColor(NXThemeType::Dark, PrimaryNormal));
	setDarkHoverColor(NXThemeColor(NXThemeType::Dark, PrimaryHover));
	setDarkPressColor(NXThemeColor(NXThemeType::Dark, PrimaryPress));
	setDarkTextColor(Constant::LIGHT_TEXT_COLOR);

	_pPressRadius = 0;
	_pHoverOpacity = 0;
	_pHoverGradient = new QRadialGradient();
	_pHoverGradient->setRadius(170);
	_pHoverGradient->setColorAt(0, QColor(0xFF, 0xFF, 0xFF, 40));
	_pHoverGradient->setColorAt(1, QColor(0xFF, 0xFF, 0xFF, 0));

	_pPressGradient = new QRadialGradient();
	_pPressGradient->setRadius(170);
	_pPressGradient->setColorAt(0, QColor(0xFF, 0xFF, 0xFF, 0));
	_pPressGradient->setColorAt(1, QColor(0xFF, 0xFF, 0xFF, 100));
}

FKPushButton::FKPushButton(const QString& text, QWidget* parent /*= nullptr*/)
	: FKPushButton(parent)
{
	setText(text);
}

FKPushButton::~FKPushButton()
{

}

bool FKPushButton::event(QEvent* event) {
	switch (event->type())
	{
	case QEvent::MouseButtonPress:
	{
		break;
	}
	case QEvent::MouseButtonRelease:
	{
		QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
		QPropertyAnimation* opacityAnimation = new QPropertyAnimation(this, "pPressOpacity");
		QObject::connect(opacityAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
			update();
			});
		QObject::connect(opacityAnimation, &QPropertyAnimation::finished, this, [=]() {
			_pIsPressAnimationFinished = true;
			});
		opacityAnimation->setDuration(1250);
		opacityAnimation->setEasingCurve(QEasingCurve::InQuad);
		opacityAnimation->setStartValue(1);
		opacityAnimation->setEndValue(0);
		opacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);

		QPropertyAnimation* pressAnimation = new QPropertyAnimation(this, "pPressRadius");
		QObject::connect(pressAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
			_pPressGradient->setRadius(value.toReal());
			});
		pressAnimation->setDuration(1250);
		pressAnimation->setEasingCurve(QEasingCurve::InQuad);
		pressAnimation->setStartValue(30);
		pressAnimation->setEndValue(_getLongestDistance(mouseEvent->pos()) * 1.8);
		pressAnimation->start(QAbstractAnimation::DeleteWhenStopped);
		_pIsPressAnimationFinished = false;
		_pPressGradient->setFocalPoint(mouseEvent->pos());
		_pPressGradient->setCenter(mouseEvent->pos());
		//点击后隐藏Hover效果
		_startHoverOpacityAnimation(false);
		break;
	}
	case QEvent::MouseMove:
	{
		QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
		if (_pHoverOpacity < 1 && _pIsPressAnimationFinished)
		{
			_startHoverOpacityAnimation(true);
		}
		if (_pIsPressAnimationFinished)
		{
			_pHoverGradient->setCenter(mouseEvent->pos());
			_pHoverGradient->setFocalPoint(mouseEvent->pos());
		}
		update();
		break;
	}
	case QEvent::Enter:
	{
		_startHoverOpacityAnimation(true);
		break;
	}
	case QEvent::Leave:
	{
		_startHoverOpacityAnimation(false);
		break;
	}
	default:
	{
		break;
	}
	}
	return QWidget::event(event);
}

void FKPushButton::paintEvent(QPaintEvent* event)
{
	NXPushButton::paintEvent(event);
	QPainter painter(this);
	painter.save();
	painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
	painter.setPen(Qt::NoPen);
	//效果阴影绘制
	if (_pIsPressAnimationFinished)
	{
		//覆盖阴影绘制
		painter.setOpacity(_pHoverOpacity);
		painter.setBrush(*_pHoverGradient);
		painter.drawEllipse(_pHoverGradient->center(), _pHoverGradient->radius(), _pHoverGradient->radius());
	}
	else
	{
		//点击阴影绘制
		painter.setOpacity(_pPressOpacity);
		painter.setBrush(*_pPressGradient);
		painter.drawEllipse(_pPressGradient->center(), _pPressRadius, _pPressRadius / 1.1);
	}
	painter.restore();
}

qreal FKPushButton::_getLongestDistance(QPoint point)
{
	qreal topLeftDis = _distance(point, QPoint(0, 0));
	qreal topRightDis = _distance(point, QPoint(width(), 0));
	qreal bottomLeftDis = _distance(point, QPoint(0, height()));
	qreal bottomRightDis = _distance(point, QPoint(width(), height()));
	QList<qreal> disList{ topLeftDis, topRightDis, bottomLeftDis, bottomRightDis };
	std::sort(disList.begin(), disList.end());
	return disList[3];
}

qreal FKPushButton::_distance(QPoint point1, QPoint point2)
{
	return std::sqrt((point1.x() - point2.x()) * (point1.x() - point2.x()) + (point1.y() - point2.y()) * (point1.y() - point2.y()));
}

void FKPushButton::_startHoverOpacityAnimation(bool isVisible)
{
	QPropertyAnimation* opacityAnimation = new QPropertyAnimation(this, "pHoverOpacity");
	QObject::connect(opacityAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
		update();
		});
	opacityAnimation->setDuration(250);
	opacityAnimation->setStartValue(_pHoverOpacity);
	opacityAnimation->setEndValue(isVisible ? 1 : 0);
	opacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}
