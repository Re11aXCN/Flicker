#include "FKSwitchPannel.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include "FKConstant.h"
#include "FKUtils.h"

#include "Components/FKPushButton.h"

FKSwitchPannel::FKSwitchPannel(QWidget* parent /*= nullptr*/)
	: QWidget(parent)
	, _pLoginOpacity{ 1.0 }
	, _pRegisterOpacity{ 0.0 }
{

	_initUI();
	_initAnimations();

	QObject::connect(_pLoginSwitchBtn, &FKPushButton::clicked, this, [this]() {
		_pLoginOpacityAnimation->setDuration(1000);
		_pLoginOpacityAnimation->setEasingCurve(QEasingCurve::InOutQuad);
		_pLoginOpacityAnimation->setStartValue(1.0);
		_pLoginOpacityAnimation->setEndValue(0.0);
		_pLoginOpacityAnimation->setKeyValueAt(0.1, 1.0);
		_pLoginOpacityAnimation->setKeyValueAt(0.9, 0.0);

		_pRegisterOpacityAnimation->setDuration(700);
		_pRegisterOpacityAnimation->setEasingCurve(QEasingCurve::Linear);
		_pRegisterOpacityAnimation->setStartValue(0.3);
		_pRegisterOpacityAnimation->setEndValue(1.0);
		_pLoginSwitchBtn->setEnabled(false);
		_pRegisterSwitchBtn->setEnabled(true);
		_pRegisterSwitchBtn->raise();
		toggleFormType();
		Q_EMIT switchClicked();
		});
	QObject::connect(_pRegisterSwitchBtn, &FKPushButton::clicked, this, [this]() {
		_pRegisterOpacityAnimation->setDuration(1000);
		_pRegisterOpacityAnimation->setEasingCurve(QEasingCurve::InOutQuad);
		_pRegisterOpacityAnimation->setStartValue(1.0);
		_pRegisterOpacityAnimation->setEndValue(0.0);
		_pRegisterOpacityAnimation->setKeyValueAt(0.1, 1.0);
		_pRegisterOpacityAnimation->setKeyValueAt(0.9, 0.0);

		_pLoginOpacityAnimation->setDuration(700);
		_pLoginOpacityAnimation->setEasingCurve(QEasingCurve::Linear);
		_pLoginOpacityAnimation->setStartValue(0.3);
		_pLoginOpacityAnimation->setEndValue(1.0);
		_pLoginSwitchBtn->setEnabled(true);
		_pRegisterSwitchBtn->setEnabled(false);
		_pLoginSwitchBtn->raise();
		toggleFormType();
		Q_EMIT switchClicked();
		});
	QObject::connect(_pAnimationGroup, &QParallelAnimationGroup::finished, this, [this]() {
		_updateUI();
		});
	QObject::connect(_pLoginOpacityAnimation, &QPropertyAnimation::valueChanged, this, [this](const QVariant& value) {
		//qreal opacity = value.toReal();
		_pLoginTitleEffect->setOpacity(_pLoginOpacity);
		_pLoginDescriptionEffect->setOpacity(_pLoginOpacity);
		_pLoginSwitchBtnEffect->setOpacity(_pLoginOpacity);
		});
	QObject::connect(_pRegisterOpacityAnimation, &QPropertyAnimation::valueChanged, this, [this](const QVariant& value) {
		_pRegisterTitleEffect->setOpacity(_pRegisterOpacity);
		_pRegisterDescriptionEffect->setOpacity(_pRegisterOpacity);
		_pRegisterSwitchBtnEffect->setOpacity(_pRegisterOpacity);
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

	if (_isLoginState) {
		_pBottomCircleAnimation->setStartValue(_pBottomCirclePos);
		_pBottomCircleAnimation->setEndValue(rect().bottomRight());

		_pTopCircleAnimation->setStartValue(_pTopCirclePos);
		_pTopCircleAnimation->setEndValue(rect().topLeft());
	}
	else {
		_pBottomCircleAnimation->setStartValue(_pBottomCirclePos);
		_pBottomCircleAnimation->setEndValue(rect().bottomLeft());

		_pTopCircleAnimation->setStartValue(_pTopCirclePos);
		_pTopCircleAnimation->setEndValue(rect().topRight());
	}

	_isLoginState = !_isLoginState;

	_pAnimationGroup->start();
}

void FKSwitchPannel::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	const int width = this->width();
	const int height = this->height();

	QRect opacityRect{ rect().topLeft() + QPoint{ 0, 30 }, rect().bottomRight()  };
	// 背景色设置，为了遮挡FormPanel
	painter.fillRect(opacityRect, Constant::LIGHT_MAIN_BG_COLOR);

	QRect bottomCircleRect(_pBottomCirclePos - QPoint(250, 170), _pBottomCirclePos + QPoint(250, 330));

	QRadialGradient bottomGradient(bottomCircleRect.center(), bottomCircleRect.width() / 2);
	bottomGradient.setColorAt(0.0, Constant::LIGHT_MAIN_BG_COLOR);
	bottomGradient.setColorAt(0.85, Constant::LIGHT_MAIN_BG_COLOR);
	bottomGradient.setColorAt(0.9, Qt::white);
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

void FKSwitchPannel::_initUI()
{
	QFont SitkaHeadingFont("Sitka Heading");
	QFont SimSunFont("SimSun");
	_pLoginTitleEffect = new QGraphicsOpacityEffect(this);
	_pLoginDescriptionEffect = new QGraphicsOpacityEffect(this);
	_pLoginSwitchBtnEffect = new QGraphicsOpacityEffect(this);
	_pRegisterTitleEffect = new QGraphicsOpacityEffect(this);
	_pRegisterDescriptionEffect = new QGraphicsOpacityEffect(this);
	_pRegisterSwitchBtnEffect = new QGraphicsOpacityEffect(this);

	_pLoginTitleEffect->setOpacity(1.0);
	_pLoginDescriptionEffect->setOpacity(1.0);
	_pLoginSwitchBtnEffect->setOpacity(1.0);

	_pRegisterTitleEffect->setOpacity(0.0);
	_pRegisterDescriptionEffect->setOpacity(0.0);
	_pRegisterSwitchBtnEffect->setOpacity(0.0);

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
	_pLoginSwitchBtn = new FKPushButton("SIGN UP", this);

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

	_pLoginSwitchBtn->setGraphicsEffect(_pLoginSwitchBtnEffect);

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
	_pRegisterSwitchBtn = new FKPushButton("SIGN IN", this);

	_pRegisterTitleText->setFont(SitkaHeadingFont);
	_pRegisterTitleText->setFixedWidth(280);
	_pRegisterTitleText->setAlignment(Qt::AlignCenter);
	_pRegisterTitleText->setTextStyle(NXTextType::CustomStyle, 28, QFont::Weight::ExtraBold);
	_pRegisterTitleText->setGraphicsEffect(_pRegisterTitleEffect);

	_pRegisterDescriptionText->setFont(SimSunFont);
	_pRegisterDescriptionText->setFixedWidth(300);
	_pRegisterDescriptionText->setAlignment(Qt::AlignCenter);
	_pRegisterDescriptionText->setTextStyle(NXTextType::CustomStyle, 15, QFont::Weight::Medium);
	_pRegisterDescriptionText->setGraphicsEffect(_pRegisterDescriptionEffect);

	_pRegisterSwitchBtn->setGraphicsEffect(_pRegisterSwitchBtnEffect);
	_pRegisterSwitchBtn->setEnabled(false);

	_pLoginSwitchBtn->raise();
	/*QVBoxLayout* loginLayout = new QVBoxLayout();
	loginLayout->setContentsMargins(50, 55, 50, 85);
	loginLayout->setSpacing(0);
	loginLayout->addStretch();
	loginLayout->addWidget(_pLoginTitleText, 0, Qt::AlignCenter);
	loginLayout->addSpacing(40);
	loginLayout->addWidget(_pLoginDescriptionText, 0, Qt::AlignCenter);
	loginLayout->addSpacing(50);
	loginLayout->addWidget(_pLoginSwitchBtn, 0, Qt::AlignCenter);
	loginLayout->addStretch();

	QVBoxLayout* registerLayout = new QVBoxLayout();
	registerLayout->setContentsMargins(50, 55, 50, 85);
	registerLayout->setSpacing(0);
	registerLayout->addStretch();
	registerLayout->addWidget(_pRegisterTitleText, 0, Qt::AlignCenter);
	registerLayout->addSpacing(40);
	registerLayout->addWidget(_pRegisterDescriptionText, 0, Qt::AlignCenter);
	registerLayout->addSpacing(50);
	registerLayout->addWidget(_pRegisterSwitchBtn, 0, Qt::AlignCenter);
	registerLayout->addStretch();*/
	constexpr int XOffset = (Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT) / 2;
	constexpr int YOffset = Constant::WIDGET_HEIGHT / 2;
	const QSize loginTitleSize = _pLoginTitleText->size();
	const QSize loginDescriptionSize = _pLoginDescriptionText->size();
	const QSize loginSwitchBtnSize = _pLoginSwitchBtn->size();

	const int half = _pLoginDescriptionText->height() / 2;
	_pLoginTitleText->move(XOffset - loginTitleSize.width() / 2, YOffset - loginTitleSize.height() / 2 - half - 70);
	_pLoginDescriptionText->move(XOffset - loginDescriptionSize.width() / 2, YOffset - half);
	_pLoginSwitchBtn->move(XOffset - loginSwitchBtnSize.width() / 2, YOffset + loginSwitchBtnSize.height() / 2 + half + 40);

	_pRegisterTitleText->move(_pLoginTitleText->pos());
	_pRegisterDescriptionText->move(_pLoginDescriptionText->pos());
	_pRegisterSwitchBtn->move(_pLoginSwitchBtn->pos());
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
	_pLoginOpacityAnimation->setDuration(1000);
	_pLoginOpacityAnimation->setEasingCurve(QEasingCurve::InOutQuad);
	_pLoginOpacityAnimation->setStartValue(1.0);
	_pLoginOpacityAnimation->setEndValue(0.0);
	_pLoginOpacityAnimation->setKeyValueAt(0.1, 1.0);
	_pLoginOpacityAnimation->setKeyValueAt(0.9, 0.0);

	_pRegisterOpacityAnimation = new QPropertyAnimation(this, "pRegisterOpacity");
	_pRegisterOpacityAnimation->setDuration(700);
	_pRegisterOpacityAnimation->setEasingCurve(QEasingCurve::Linear);
	_pRegisterOpacityAnimation->setStartValue(0.3);
	_pRegisterOpacityAnimation->setEndValue(1.0);

	_pAnimationGroup->addAnimation(_pBottomCircleAnimation);
	_pAnimationGroup->addAnimation(_pTopCircleAnimation);
	_pAnimationGroup->addAnimation(_pLoginOpacityAnimation);
	_pAnimationGroup->addAnimation(_pRegisterOpacityAnimation);
}

void FKSwitchPannel::_updateUI()
{
	
}