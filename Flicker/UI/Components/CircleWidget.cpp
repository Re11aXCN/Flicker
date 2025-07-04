#include "CircleWidget.h"

#include <QPainter>
#include <QPainterPath>
#include "FKConstant.h"
#include "Components/NXBoxShadow.h"
CircleWidget::CircleWidget(QWidget* parent /*= nullptr*/)
	: QWidget(parent)
{
	// 设置窗口标志以实现透明背景
	//setAttribute(Qt::WA_TranslucentBackground);
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	setStyleSheet("background-color: gray;");
	maskRegion = QRegion(0, 0, 200, 200, QRegion::Ellipse);
	setFixedSize(200, 200);
	NXBoxShadow* shadow = new NXBoxShadow(this);
	shadow->setBlur(15.0);
	shadow->setSpread(10.0);
	shadow->setLightColor(QColor{ 255, 0, 0, 100 });
	shadow->setDarkColor(QColor{ 0, 0, 0, 100 });
	shadow->setProjectionType(NXWidgetType::BoxShadow::Inset);
	// 启用圆形阴影
	shadow->setCircleShadow(true);

	setGraphicsEffect(shadow);
}
void CircleWidget::resizeEvent(QResizeEvent* event)
{
	// 更新圆形遮罩区域
	maskRegion = QRegion(0, 0, width(), height(), QRegion::Ellipse);
	//setMask(maskRegion);
	QWidget::resizeEvent(event);
}

void CircleWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
	painter.setPen(Qt::NoPen);
	painter.setBrush(Constant::LIGHT_MAIN_BG_COLOR);
	painter.drawEllipse(rect());
	QWidget::paintEvent(event);
}

