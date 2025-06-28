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
	, _pMoveOpacity{ 1.0 }
{

	_initUI();
	_initAnimations();

	QObject::connect(_pSwitchBtn, &FKPushButton::clicked, this, [this]() {
		toggleState();
		Q_EMIT switchClicked();
		});
	QObject::connect(_pAnimationGroup, &QParallelAnimationGroup::finished, this, [this]() {
		_updateUI();
		});
}

FKSwitchPannel::~FKSwitchPannel()
{
	delete _pAnimationGroup;
}

void FKSwitchPannel::setBottomCirclePos(const QPoint& pos) { _pBottomCirclePos = pos; update(); }

void FKSwitchPannel::setTopCirclePos(const QPoint& pos) { _pTopCirclePos = pos; update(); }

void FKSwitchPannel::setMoveOpacity(qreal opacity) { _pMoveOpacity = opacity; update(); }

void FKSwitchPannel::toggleState()
{
	// 停止正在进行的动画
	_pAnimationGroup->stop();

	// 设置动画起始值和结束值
	_pOpacityAnimation->setStartValue(_pMoveOpacity);
	_pOpacityAnimation->setEndValue(_pMoveOpacity);
	_pOpacityAnimation->setKeyValueAt(0.5, 0.0); // 中间点完全透明

	if (_isLoginState) {
		// 从登录状态切换到注册状态
		_pBottomCircleAnimation->setStartValue(_pBottomCirclePos);
		_pBottomCircleAnimation->setEndValue(rect().bottomRight());

		_pTopCircleAnimation->setStartValue(_pTopCirclePos);
		_pTopCircleAnimation->setEndValue(rect().topLeft());
	}
	else {
		// 从注册状态切换到登录状态
		_pBottomCircleAnimation->setStartValue(_pBottomCirclePos);
		_pBottomCircleAnimation->setEndValue(rect().bottomLeft());

		_pTopCircleAnimation->setStartValue(_pTopCirclePos);
		_pTopCircleAnimation->setEndValue(rect().topRight());
	}

	// 切换状态标志
	_isLoginState = !_isLoginState;

	// 启动动画
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

	// 绘制底部圆形（使用动画属性控制位置）
	QRect bottomCircleRect(_pBottomCirclePos - QPoint(250, 170), _pBottomCirclePos + QPoint(250, 330));

	// 创建径向渐变实现内阴影效果
	QRadialGradient bottomGradient(bottomCircleRect.center(), bottomCircleRect.width() / 2);
	bottomGradient.setColorAt(0.0, Constant::LIGHT_MAIN_BG_COLOR); // 圆形背景色
	bottomGradient.setColorAt(0.85, Constant::LIGHT_MAIN_BG_COLOR); // 圆形背景色
	bottomGradient.setColorAt(0.9, Qt::white); // 内阴影亮部
	bottomGradient.setColorAt(1.0, Constant::SWITCH_CIRCLE_DARK_SHADOW_COLOR); // 内阴影暗部
	painter.setBrush(bottomGradient);
	painter.setPen(Qt::NoPen);
	painter.drawEllipse(bottomCircleRect);

	// 绘制顶部圆形（使用动画属性控制位置）
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
	_pTitleText = new NXText(FKUtils::concat(
		"<font color='",
		FKUtils::colorToCssString(Constant::DARK_TEXT_COLOR),
		"'>Hello Friend！</font>"
	), this);
	_pDescriptionText = new NXText(FKUtils::concat(
		"<font color='",
		FKUtils::colorToCssString(Constant::DESCRIPTION_TEXT_COLOR),
		"'>去注册一个账号，成为尊贵的粉丝会员，让我们踏入奇妙的旅途！</font>"
	), this);
	_pSwitchBtn = new FKPushButton("SIGN UP", this);

	_pTitleText->setTextStyle(NXTextType::CustomStyle, 28, QFont::Weight::Bold);
	_pDescriptionText->setFixedWidth(280);
	_pDescriptionText->setAlignment(Qt::AlignCenter);
	_pDescriptionText->setTextStyle(NXTextType::CustomStyle, 14, QFont::Weight::Normal);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(50, 55, 50, 85);
	mainLayout->setSpacing(0);
	mainLayout->addStretch();
	mainLayout->addWidget(_pTitleText, 0, Qt::AlignCenter);
	mainLayout->addSpacing(40);
	mainLayout->addWidget(_pDescriptionText, 0, Qt::AlignCenter);
	mainLayout->addSpacing(50);
	mainLayout->addWidget(_pSwitchBtn, 0, Qt::AlignCenter);
	mainLayout->addStretch();
}

void FKSwitchPannel::_initAnimations()
{
	// 创建动画组
	_pAnimationGroup = new QParallelAnimationGroup(this);

	// 创建底部圆形动画
	_pBottomCircleAnimation = new QPropertyAnimation(this, "pBottomCirclePos");
	_pBottomCircleAnimation->setDuration(1250); // 1.25秒
	_pBottomCircleAnimation->setEasingCurve(QEasingCurve::OutCubic);

	// 创建顶部圆形动画
	_pTopCircleAnimation = new QPropertyAnimation(this, "pTopCirclePos");
	_pTopCircleAnimation->setDuration(1250); // 1.25秒
	_pTopCircleAnimation->setEasingCurve(QEasingCurve::OutCubic);

	// 创建不透明度动画
	_pOpacityAnimation = new QPropertyAnimation(this, "pMoveOpacity");
	_pOpacityAnimation->setDuration(600); // 0.6秒
	_pOpacityAnimation->setEasingCurve(QEasingCurve::InOutQuad);
	_pOpacityAnimation->setStartValue(1.0);
	_pOpacityAnimation->setEndValue(0.0);
	_pOpacityAnimation->setKeyValueAt(0.5, 0.0); // 中间点完全透明

	// 添加到动画组
	_pAnimationGroup->addAnimation(_pBottomCircleAnimation);
	_pAnimationGroup->addAnimation(_pTopCircleAnimation);
	_pAnimationGroup->addAnimation(_pOpacityAnimation);
}

void FKSwitchPannel::_updateUI()
{
	// 根据当前状态更新文本
	if (_isLoginState) {
		// 当前是登录状态，显示注册相关文本
		_pTitleText->setText(FKUtils::concat(
			"<font color='",
			FKUtils::colorToCssString(Constant::DARK_TEXT_COLOR),
			"'>Hello Friend！</font>"
		));
		_pDescriptionText->setText(FKUtils::concat(
			"<font color='",
			FKUtils::colorToCssString(Constant::DESCRIPTION_TEXT_COLOR),
			"'>去注册一个账号，成为尊贵的粉丝会员，让我们踏入奇妙的旅途！</font>"
		));
		_pSwitchBtn->setText("SIGN UP");
	}
	else {
		// 当前是注册状态，显示登录相关文本
		_pTitleText->setText(FKUtils::concat(
			"<font color='",
			FKUtils::colorToCssString(Constant::DARK_TEXT_COLOR),
			"'>Welcome Back！</font>"
		));
		_pDescriptionText->setText(FKUtils::concat(
			"<font color='",
			FKUtils::colorToCssString(Constant::DESCRIPTION_TEXT_COLOR),
			"'>已经有账号了嘛，去登入账号来进入奇妙世界吧！！！</font>"
		));
		_pSwitchBtn->setText("SIGN IN");
	}
}