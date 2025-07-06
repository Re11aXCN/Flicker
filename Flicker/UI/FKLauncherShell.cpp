#include "FKLauncherShell.h"
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

#include <NXIcon.h>
#include "FKUtils.h"
#include "FKConstant.h"
#include "Components/FKFormPannel.h"
#include "Components/FKSwitchPannel.h"
#include "Components/FKShadowWidget.h"

FKLauncherShell::FKLauncherShell(QWidget* parent /*= nullptr*/)
	: NXWidget(parent)
{
	_initUi();
	_initAnimation();
	QObject::connect(_pSwitchPannel, &FKSwitchPannel::switchClicked, this, &FKLauncherShell::_onTogglePannelButtonClicked);
}

FKLauncherShell::~FKLauncherShell()
{
	delete _pAnimationGroup;
}
void FKLauncherShell::setTopCircleX(qreal x) { _pTopCircleX = x; update(); }
void FKLauncherShell::ShowMessage(const QString& title, const QString& text, NXMessageBarType::MessageMode mode, NXMessageBarType::PositionPolicy position, int displayMsec /*= 2000*/)
{
	_pMessageButton->setBarTitle(title);
	_pMessageButton->setBarText(text);
	_pMessageButton->setMessageMode(mode);
	_pMessageButton->setPositionPolicy(position);
	_pMessageButton->setDisplayMsec(displayMsec);
	Q_EMIT _pMessageButton->showMessage();
}

void FKLauncherShell::_initUi()
{
	setAppBarHeight(30);
	setIsFixedSize(true);
	setFixedSize(Constant::WIDGET_WIDTH, Constant::WIDGET_HEIGHT);
	setWindowTitle("Flicker");
	setWindowIcon(QIcon(":/Resource/ico/Weixin_IDI_ICON1.ico"));
	setCustomBackgroundColor(Constant::LIGHT_MAIN_BG_COLOR, Constant::DARK_MAIN_BG_COLOR);
	setWindowButtonFlags(NXAppBarType::MinimizeButtonHint | NXAppBarType::CloseButtonHint);
	_pMessageButton = new NXMessageButton(this);
	_pFormPannel = new FKFormPannel(this);
	_pSwitchPannel = new FKSwitchPannel(this);
	_pShadowWidget = new FKShadowWidget(this);

	_pMessageButton->setFixedSize(1, 1);
	_pMessageButton->setVisible(false);

	_pFormPannel->setFixedSize(Constant::WIDGET_HEIGHT, Constant::WIDGET_HEIGHT);

	_pSwitchPannel->setFixedSize(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, Constant::WIDGET_HEIGHT);
	_pSwitchPannel->setTopCirclePos(_pSwitchPannel->rect().topRight());
	_pSwitchPannel->setBottomCirclePos(_pSwitchPannel->rect().bottomLeft());

	_pShadowWidget->setFixedSize(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, Constant::WIDGET_HEIGHT);
	_pShadowWidget->setAttribute(Qt::WA_TranslucentBackground, true);
	_pShadowWidget->setBlur(30.0);
	_pShadowWidget->setLightColor(Constant::SWITCH_BOX_SHADOW_COLOR);
	_pShadowWidget->setDarkColor(Constant::SWITCH_BOX_SHADOW_COLOR);
	_pShadowWidget->setLightOffset({ -5,-5 });
	_pShadowWidget->setDarkOffset({ 5,5 });
	_pShadowWidget->setProjectionType(NXWidgetType::BoxShadow::ProjectionType::Outset);
	_pShadowWidget->setRotateMode(NXWidgetType::BoxShadow::RotateMode::Rotate45);

	_pShadowWidget->setCustomDraw([&](QPainter* painter) {
		painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
		painter->setPen(Qt::NoPen);
		painter->setBrush(Constant::LIGHT_MAIN_BG_COLOR);
		QPainterPath rectPath;
		rectPath.addRect(_pShadowWidget->rect());
		// 创建凹槽路径
		QPainterPath notchPath;

		//// 顶部凹槽 (只需要绘制下半圆弧,180°到360°)
		//{
		//	QRectF topCircleRect(QPoint{ 250, -180 }, QSize{ 300, 300 });
		//	notchPath.moveTo(topCircleRect.left(), topCircleRect.center().y());
		//	notchPath.arcTo(topCircleRect, 180, 180);
		//	notchPath.closeSubpath();
		//}

		//// 底部凹槽 (只需要绘制上半圆弧,0°到180°)
		//{
		//	QRectF bottomCircleRect(QPoint{ -250, 460 }, QSize{ 500, 500 });
		//	notchPath.moveTo(bottomCircleRect.left(), bottomCircleRect.center().y());
		//	notchPath.arcTo(bottomCircleRect, 0, 180);
		//	notchPath.closeSubpath();
		//}
		notchPath.moveTo(400, 120);
		notchPath.lineTo(400, 0);
		notchPath.lineTo(250, 0);
		notchPath.arcTo(QRectF{ QPointF{ 250 - _pTopCircleX, -180 }, QSizeF{ 300, 300 } }, 180, 90); // 从B点画弧线到O点
		notchPath.closeSubpath();
		// 从矩形中减去凹槽形成最终路径
		QPainterPath finalPath = rectPath.subtracted(notchPath);
		painter->drawPath(finalPath);
		});
	_pFormPannel->move(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, 0);
	_pSwitchPannel->move(0, 0);
	_pShadowWidget->move(0, 0);
	
	_pFormPannel->lower();
	_pShadowWidget->raise();
	_pSwitchPannel->raise();

	// 防止 Pannel 遮挡 最小化 和 关闭按钮的点击事件
	NXAppBar* appBar = this->appBar();
	QWidget* appBarWindow = appBar->window();
	appBarWindow->setWindowFlags((appBarWindow->windowFlags()) | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
	appBar->raise();
}

void FKLauncherShell::_initAnimation()
{
	_pAnimationGroup = new QParallelAnimationGroup(this);

	_pSwitchPannelAnimation = new QPropertyAnimation(_pSwitchPannel, "pos");
	_pSwitchPannelAnimation->setDuration(1250); 
	_pSwitchPannelAnimation->setEasingCurve(QEasingCurve::OutCubic);

	_pShadowWidgetAnimation = new QPropertyAnimation(_pShadowWidget, "pos");
	_pShadowWidgetAnimation->setDuration(1250); 
	_pShadowWidgetAnimation->setEasingCurve(QEasingCurve::OutCubic);

	_pShadowWidgetWidthAnimation = new QPropertyAnimation(_pShadowWidget, "minimumWidth");
	_pShadowWidgetWidthAnimation->setDuration(1250);
	_pShadowWidgetWidthAnimation->setEasingCurve(QEasingCurve::OutCubic);
	_pShadowWidgetWidthAnimation->setKeyValues({ {0.0, 400}, {0.5, 500}, {1.0, 400} });

	_pSwitchPannelWidthAnimation = new QPropertyAnimation(_pSwitchPannel, "minimumWidth");
	_pSwitchPannelWidthAnimation->setDuration(1250);
	_pSwitchPannelWidthAnimation->setEasingCurve(QEasingCurve::OutCubic);
	_pSwitchPannelWidthAnimation->setKeyValues({ {0.0, 400}, {0.5, 500}, {1.0, 400} });

	_pFormPannelAnimation = new QPropertyAnimation(_pFormPannel, "pos");
	_pFormPannelAnimation->setDuration(1250); 
	_pFormPannelAnimation->setEasingCurve(QEasingCurve::OutCubic);

	_pSwitchTopCircleXAnimation = new QPropertyAnimation(_pFormPannel, "pTopCircleX");
	_pSwitchTopCircleXAnimation->setDuration(1250);
	_pSwitchTopCircleXAnimation->setEasingCurve(QEasingCurve::OutCubic);

	_pAnimationGroup->addAnimation(_pSwitchPannelAnimation);
	_pAnimationGroup->addAnimation(_pShadowWidgetAnimation);
	_pAnimationGroup->addAnimation(_pFormPannelAnimation);
	_pAnimationGroup->addAnimation(_pSwitchPannelWidthAnimation);
	_pAnimationGroup->addAnimation(_pShadowWidgetWidthAnimation);
	_pAnimationGroup->addAnimation(_pSwitchTopCircleXAnimation);
}

void FKLauncherShell::_onTogglePannelButtonClicked()
{
	_pAnimationGroup->stop();
	
	QPoint switchPos = _pSwitchPannel->pos();
	QPoint shadowPos = _pShadowWidget->pos();
	QPoint formPos = _pFormPannel->pos();

	// 设置动画起始值和结束值
	_pSwitchPannelAnimation->setStartValue(switchPos);
	_pShadowWidgetAnimation->setStartValue(shadowPos);
	_pFormPannelAnimation->setStartValue(formPos);
	_pSwitchTopCircleXAnimation->setStartValue(0.0);
	if (switchPos.x() == 0) {
		_pSwitchPannelAnimation->setEndValue(QPoint(Constant::WIDGET_HEIGHT, 0));
		_pShadowWidgetAnimation->setEndValue(QPoint(Constant::WIDGET_HEIGHT, 0));
		_pFormPannelAnimation->setEndValue(QPoint(0, 0));
		_pSwitchTopCircleXAnimation->setEndValue(250.0);
	} else {
		_pSwitchPannelAnimation->setEndValue(QPoint(0, 0));
		_pShadowWidgetAnimation->setEndValue(QPoint(0, 0));
		_pFormPannelAnimation->setEndValue(QPoint(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, 0));
		_pSwitchTopCircleXAnimation->setEndValue(0.0);
	}
	
	_pFormPannel->toggleFormType();
	_pAnimationGroup->start();
}
