#include "FKSwitchPannel.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QPauseAnimation>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QStyleOption>
#include <NXTheme.h>

#include "FKConstant.h"
#include "FKUtils.h"

#include "Components/FKPushButton.h"
#include "Components/NXBoxShadowEffect.h"
#include "Components/FKShadowWidget.h"
FKSwitchPannel::FKSwitchPannel(QWidget* parent /*= nullptr*/)
	: QWidget(parent)
	, _pLoginOpacity{ 1.0 }
	, _pRegisterOpacity{ 0.0 }
{
	_initUI();
	_initAnimations();
	//NXBoxShadowEffect* shadow = new NXBoxShadowEffect(this);
	//shadow->setBlur(30.0);
	//shadow->setLightColor(QColor(0, 0x1E, 0x9A, 102));
	//shadow->setDarkColor(Constant::SWITCH_CIRCLE_DARK_SHADOW_COLOR);
	//shadow->setLightOffset({ -5,-5 });
	//shadow->setDarkOffset({ 5,5 });
	//shadow->setProjectionType(NXWidgetType::BoxShadow::ProjectionType::Outset);
	//shadow->setRotateMode(NXWidgetType::BoxShadow::RotateMode::Rotate45);
	//setGraphicsEffect(shadow);
	FKShadowWidget* pFormPanel = new FKShadowWidget(this);
	pFormPanel->setFixedSize(200, 300);
	pFormPanel->setBlur(30.0);
	pFormPanel->setLightColor(QColor(0, 0x1E, 0x9A, 102));
	pFormPanel->setDarkColor(Constant::SWITCH_CIRCLE_DARK_SHADOW_COLOR);
	pFormPanel->setLightOffset({ -5,-5 });
	pFormPanel->setDarkOffset({ 5,5 });
	pFormPanel->setProjectionType(NXWidgetType::BoxShadow::ProjectionType::Outset);
	pFormPanel->setRotateMode(NXWidgetType::BoxShadow::RotateMode::Rotate315);
	//pFormPanel->move(240,-178);
	pFormPanel->move(50, 50);
	pFormPanel->lower();

	QObject::connect(_pSwitchBtn, &FKPushButton::clicked, this, &FKSwitchPannel::toggleFormType);
	QObject::connect(_pLoginOpacityAnimation, &QPropertyAnimation::valueChanged, this, &FKSwitchPannel::_updateLoginOpacity);
	QObject::connect(_pRegisterOpacityAnimation, &QPropertyAnimation::valueChanged, this, &FKSwitchPannel::_updateRegisterOpacity);
	QObject::connect(_pAnimationGroup, &QParallelAnimationGroup::finished, this, [this]() {

		});
}

FKSwitchPannel::~FKSwitchPannel()
{
	delete _pAnimationGroup;
}

void FKSwitchPannel::setBottomCirclePos(const QPoint& pos) { _pBottomCirclePos = pos; update(); }

void FKSwitchPannel::setTopCirclePos(const QPoint& pos) { _pTopCirclePos = pos; update(); }

void FKSwitchPannel::setLoginOpacity(qreal opacity) { _pLoginOpacity = opacity; update(); }

void FKSwitchPannel::setRegisterOpacity(qreal opacity) { _pRegisterOpacity = opacity; update(); }

void FKSwitchPannel::toggleFormType()
{
	_pAnimationGroup->stop();

	if (_pIsLoginState) {
		_pBottomCircleAnimation->setStartValue(_pBottomCirclePos);
		_pBottomCircleAnimation->setEndValue(rect().bottomRight());

		_pTopCircleAnimation->setStartValue(_pTopCirclePos);
		_pTopCircleAnimation->setEndValue(rect().topLeft());

		_pLoginOpacityAnimation->setEasingCurve(QEasingCurve::InOutQuad);
		_pLoginOpacityAnimation->setStartValue(1.0);
		_pLoginOpacityAnimation->setEndValue(0.0);
		_pRegisterOpacityAnimation->setEasingCurve(QEasingCurve::Linear);
		_pRegisterOpacityAnimation->setStartValue(0.3);
		_pRegisterOpacityAnimation->setEndValue(1.0);
	}
	else {
		_pBottomCircleAnimation->setStartValue(_pBottomCirclePos);
		_pBottomCircleAnimation->setEndValue(rect().bottomLeft());

		_pTopCircleAnimation->setStartValue(_pTopCirclePos);
		_pTopCircleAnimation->setEndValue(rect().topRight());

		_pRegisterOpacityAnimation->setEasingCurve(QEasingCurve::InOutQuad);
		_pRegisterOpacityAnimation->setStartValue(1.0);
		_pRegisterOpacityAnimation->setEndValue(0.0);
		_pLoginOpacityAnimation->setEasingCurve(QEasingCurve::Linear);
		_pLoginOpacityAnimation->setStartValue(0.3);
		_pLoginOpacityAnimation->setEndValue(1.0);
	}
	_pIsLoginState = !_pIsLoginState;
	_pAnimationGroup->start();
	_pSwitchBtn->setText(_pIsLoginState ? "REGISTER" : "LOGIN");

	Q_EMIT switchClicked();
}

void FKSwitchPannel::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	const int width = this->width();
	const int height = this->height();

	QRect opacityRect{ rect().topLeft(), rect().bottomRight()  };
	// 背景色设置，为了遮挡FormPanel
	painter.fillRect(opacityRect, Constant::LIGHT_MAIN_BG_COLOR);

	QRect bottomCircleRect(_pBottomCirclePos - QPoint(242, 138), _pBottomCirclePos + QPoint(258, 362));

	QRadialGradient bottomGradient(bottomCircleRect.center(), bottomCircleRect.width() / 2);
	bottomGradient.setColorAt(0.0, Constant::LIGHT_MAIN_BG_COLOR);
	bottomGradient.setColorAt(0.95, Constant::LIGHT_MAIN_BG_COLOR);
	//bottomGradient.setColorAt(0.9, Qt::white);
	bottomGradient.setColorAt(1.0, Constant::SWITCH_CIRCLE_DARK_SHADOW_COLOR);
	painter.setBrush(bottomGradient);
	painter.setPen(Qt::NoPen);
	painter.drawEllipse(bottomCircleRect);

	QRect topCircleRect(_pTopCirclePos - QPoint(150, 180), _pTopCirclePos + QPoint(150, 120));

	QRadialGradient topGradient(topCircleRect.center(), topCircleRect.width() / 2);
	topGradient.setColorAt(0.0, Constant::LIGHT_MAIN_BG_COLOR);
	topGradient.setColorAt(0.85, Constant::LIGHT_MAIN_BG_COLOR);
	topGradient.setColorAt(0.9, Qt::white);
	topGradient.setColorAt(1.0, Constant::SWITCH_CIRCLE_DARK_SHADOW_COLOR);
	painter.setBrush(topGradient);
	painter.drawEllipse(topCircleRect);

	// 1. 定义同心圆
	const int centerX = width / 2;
	const int centerY = height / 2;
	const int outerRadius = 115;  // 大圆半径
	const int innerRadius = 100;  // 小圆半径
	QPointF center(centerX, centerY);

	// 2. 创建锥形渐变
	QConicalGradient gradient(center, 180);  // 180度起点在负x轴
	gradient.setColorAt(0.0, Constant::SWITCH_CIRCLE_DARK_SHADOW_COLOR);     // 起点黑色
	gradient.setColorAt(0.5, Qt::white);     // 180度后变为白色
	gradient.setColorAt(1.0, Constant::SWITCH_CIRCLE_DARK_SHADOW_COLOR);     // 回到起点黑色

	// 3. 创建圆环路径
	QPainterPath ringPath;
	ringPath.addEllipse(center, outerRadius, outerRadius);  // 大圆
	ringPath.addEllipse(center, innerRadius, innerRadius);  // 减去小圆
	ringPath.setFillRule(Qt::OddEvenFill);                 // 设置填充规则

	// 4. 绘制圆环
	painter.fillPath(ringPath, gradient);
	QWidget::paintEvent(event);
}

void FKSwitchPannel::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	_updateChildGeometry();
}

void FKSwitchPannel::_initUI()
{
	QFont SitkaHeadingFont("Sitka Heading");
	QFont SimSunFont("SimSun");
	_pLoginTitleEffect = new QGraphicsOpacityEffect(this);
	_pLoginDescriptionEffect = new QGraphicsOpacityEffect(this);
	_pRegisterTitleEffect = new QGraphicsOpacityEffect(this);
	_pRegisterDescriptionEffect = new QGraphicsOpacityEffect(this);
	_pSwitchBtnEffect = new QGraphicsOpacityEffect(this);

	_pLoginTitleText = new NXText(FKUtils::concat(
		"<font color='",
		FKUtils::colorToCssString(Constant::DARK_TEXT_COLOR),
		"'>Hello Friend！</font>"
	), this);
	_pLoginDescriptionText = new NXText(FKUtils::concat(
		"<font color='",
		FKUtils::colorToCssString(Constant::DESCRIPTION_TEXT_COLOR),
		"'>去注册一个账号，成为尊贵的粉丝会员，让我们踏入奇妙的旅途！</font>"
	), this);

	_pRegisterTitleText = new NXText(FKUtils::concat(
		"<font color='",
		FKUtils::colorToCssString(Constant::DARK_TEXT_COLOR),
		"'>Welcome Back！</font>"
	), this);
	_pRegisterDescriptionText = new NXText(FKUtils::concat(
		"<font color='",
		FKUtils::colorToCssString(Constant::DESCRIPTION_TEXT_COLOR),
		"'>已经有账号了嘛，去登入账号来进入奇妙世界吧！！！</font>"
	), this);

	_pSwitchBtn = new FKPushButton("REGISTER", this);

	_pLoginTitleEffect->setOpacity(1.0);
	_pLoginDescriptionEffect->setOpacity(1.0);
	_pRegisterTitleEffect->setOpacity(0.0);
	_pRegisterDescriptionEffect->setOpacity(0.0);
	_pSwitchBtnEffect->setOpacity(1.0);

	_pLoginTitleText->setFont(SitkaHeadingFont);
	_pLoginTitleText->setFixedWidth(280);
	_pLoginTitleText->setAlignment(Qt::AlignCenter);
	_pLoginTitleText->setTextStyle(NXTextType::CustomStyle, 32, QFont::Weight::Black);
	_pLoginTitleText->setGraphicsEffect(_pLoginTitleEffect);

	_pLoginDescriptionText->setFont(SimSunFont);
	_pLoginDescriptionText->setFixedWidth(300);
	_pLoginDescriptionText->setAlignment(Qt::AlignCenter);
	_pLoginDescriptionText->setTextStyle(NXTextType::CustomStyle, 15, QFont::Weight::Medium);
	_pLoginDescriptionText->setGraphicsEffect(_pLoginDescriptionEffect);

	_pRegisterTitleText->setFont(SitkaHeadingFont);
	_pRegisterTitleText->setFixedWidth(280);
	_pRegisterTitleText->setAlignment(Qt::AlignCenter);
	_pRegisterTitleText->setTextStyle(NXTextType::CustomStyle, 32, QFont::Weight::Black);
	_pRegisterTitleText->setGraphicsEffect(_pRegisterTitleEffect);

	_pRegisterDescriptionText->setFont(SimSunFont);
	_pRegisterDescriptionText->setFixedWidth(300);
	_pRegisterDescriptionText->setAlignment(Qt::AlignCenter);
	_pRegisterDescriptionText->setTextStyle(NXTextType::CustomStyle, 15, QFont::Weight::Medium);
	_pRegisterDescriptionText->setGraphicsEffect(_pRegisterDescriptionEffect);

	_pSwitchBtn->setGraphicsEffect(_pSwitchBtnEffect);

	constexpr int XOffset = (Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT) / 2;
	constexpr int YOffset = Constant::WIDGET_HEIGHT / 2;
	const QSize loginTitleSize = _pLoginTitleText->size();
	const QSize loginDescriptionSize = _pLoginDescriptionText->size();
	const QSize switchBtnSize = _pSwitchBtn->size();

	const int half = _pLoginDescriptionText->height() / 2;
	_pLoginTitleText->move(XOffset - loginTitleSize.width() / 2, YOffset - loginTitleSize.height() / 2 - half - 70);
	_pLoginDescriptionText->move(XOffset - loginDescriptionSize.width() / 2, YOffset - half);
	_pRegisterTitleText->move(_pLoginTitleText->pos());
	_pRegisterDescriptionText->move(_pLoginDescriptionText->pos());

	_pSwitchBtn->move(XOffset - switchBtnSize.width() / 2, YOffset + switchBtnSize.height() / 2 + half + 40);
}

void FKSwitchPannel::_initAnimations()
{
	_pAnimationGroup = new QParallelAnimationGroup(this);

	_pBottomCircleAnimation = new QPropertyAnimation(this, "pBottomCirclePos");
	_pBottomCircleAnimation->setDuration(1250);
	_pBottomCircleAnimation->setEasingCurve(QEasingCurve::OutCubic);

	_pTopCircleAnimation = new QPropertyAnimation(this, "pTopCirclePos");
	_pTopCircleAnimation->setDuration(1250);
	_pTopCircleAnimation->setEasingCurve(QEasingCurve::OutCubic);

	_pLoginOpacityAnimation = new QPropertyAnimation(this, "pLoginOpacity");
	_pLoginOpacityAnimation->setDuration(1250);

	_pRegisterOpacityAnimation = new QPropertyAnimation(this, "pRegisterOpacity");
	_pRegisterOpacityAnimation->setDuration(1250);

	_pAnimationGroup->addAnimation(_pBottomCircleAnimation);
	_pAnimationGroup->addAnimation(_pTopCircleAnimation);
	_pAnimationGroup->addAnimation(_pLoginOpacityAnimation);
	_pAnimationGroup->addAnimation(_pRegisterOpacityAnimation);
}

void FKSwitchPannel::_updateChildGeometry()
{
	const int squareSize = height();
	const int squareLeft = (width() - squareSize) / 2;
	const int squareCenterX = squareLeft + squareSize / 2;
	const int squareCenterY = squareSize / 2;

	// 更新所有子控件位置（保持居中）
	const int half = _pLoginDescriptionText->height() / 2;

	// 登录标题和描述
	_pLoginTitleText->move(
		squareCenterX - _pLoginTitleText->width() / 2,
		squareCenterY - _pLoginTitleText->height() / 2 - half - 70
	);
	_pLoginDescriptionText->move(
		squareCenterX - _pLoginDescriptionText->width() / 2,
		squareCenterY - half
	);

	// 注册标题和描述
	_pRegisterTitleText->move(
		squareCenterX - _pRegisterTitleText->width() / 2,
		squareCenterY - _pRegisterTitleText->height() / 2 - half - 70
	);
	_pRegisterDescriptionText->move(
		squareCenterX - _pRegisterDescriptionText->width() / 2,
		squareCenterY - half
	);

	// 切换按钮
	_pSwitchBtn->move(
		squareCenterX - _pSwitchBtn->width() / 2,
		squareCenterY + _pSwitchBtn->height() / 2 + half + 40
	);
}

void FKSwitchPannel::_updateLoginOpacity(const QVariant& value)
{
	_pLoginTitleEffect->setOpacity(value.toReal());
	_pLoginDescriptionEffect->setOpacity(value.toReal());
}

void FKSwitchPannel::_updateRegisterOpacity(const QVariant& value)
{
	_pRegisterTitleEffect->setOpacity(value.toReal());
	_pRegisterDescriptionEffect->setOpacity(value.toReal());
}