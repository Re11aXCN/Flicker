#include "FKPushButton.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <NXTheme.h>

#include "FKConstant.h"

FKPushButton::FKPushButton(QWidget* parent /*= nullptr*/)
	: NXPushButton(parent)
	, _pWaveRadius{ 0 }
	, _pWaveOpacity{ 0 } // 初始值改为0（动画从255->0）
{
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
	_pWaveRadiusAnimation = new QPropertyAnimation(this, "pWaveRadius");
	_pWaveRadiusAnimation->setDuration(400);
	_pWaveRadiusAnimation->setStartValue(0); // 从0开始
	_pWaveRadiusAnimation->setEndValue(width());
	_pWaveRadiusAnimation->setEasingCurve(QEasingCurve::Linear);

	_pWaveOpacityAnimation = new QPropertyAnimation(this, "pWaveOpacity");
	_pWaveOpacityAnimation->setDuration(400);
	_pWaveOpacityAnimation->setStartValue(255); // 完全可见
	_pWaveOpacityAnimation->setEndValue(0);     // 渐变到透明
	_pWaveOpacityAnimation->setEasingCurve(QEasingCurve::Linear);
}

FKPushButton::FKPushButton(const QString& text, QWidget* parent /*= nullptr*/)
	: FKPushButton(parent)
{
	setText(text);
}

FKPushButton::~FKPushButton()
{

}

void FKPushButton::setWaveRadius(int radius) { _pWaveRadius = radius; update(); }
void FKPushButton::setWaveOpacity(int opacity) { _pWaveOpacity = opacity; update(); }

void FKPushButton::paintEvent(QPaintEvent* event)
{
	NXPushButton::paintEvent(event);

	if (_pWaveRadius > 0 && _pWaveOpacity > 0) {
		int height = this->height();
		qreal radius = height / 4.0;
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setViewport(0, 0, width(), height);
		painter.setWindow(0, 0, width(), height);

		// 创建圆角矩形裁剪路径
		QPainterPath path;
		path.addRoundedRect(rect(), radius, radius);
		painter.setClipPath(path);

		// 设置波纹样式
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(255, 255, 255, _pWaveOpacity)); // 使用动态透明度
		painter.drawEllipse(_pMousePressPos, _pWaveRadius, _pWaveRadius); // 使用动态半径
	}
}

void FKPushButton::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		_pMousePressPos = event->pos();
		_pWaveRadius = 0;       // 重置半径
		_pWaveOpacity = 255;    // 重置透明度
		_pWaveRadiusAnimation->start();
		_pWaveOpacityAnimation->start();
	}
	NXPushButton::mousePressEvent(event);
}

void FKPushButton::mouseReleaseEvent(QMouseEvent* event)
{
	NXPushButton::mouseReleaseEvent(event);
}

