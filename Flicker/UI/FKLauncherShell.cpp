#include "FKLauncherShell.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

#include <NXIcon.h>
#include "FKUtils.h"
#include "FKConstant.h"
#include "Components/FKFormPannel.h"
#include "Components/FKSwitchPannel.h"
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

	// 移除布局，使用绝对定位以便实现动画
	setLayout(nullptr);
	_pMessageButton = new NXMessageButton(this);
	_pMessageButton->setFixedSize(1, 1);
	_pMessageButton->setVisible(false);

	_pFormPannel = new FKFormPannel(this);
	_pFormPannel->setFixedSize(Constant::WIDGET_HEIGHT, Constant::WIDGET_HEIGHT);

	_pSwitchPannel = new FKSwitchPannel(this);
	_pSwitchPannel->setFixedSize(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, Constant::WIDGET_HEIGHT);
	_pSwitchPannel->setTopCirclePos(_pSwitchPannel->rect().topRight());
	_pSwitchPannel->setBottomCirclePos(_pSwitchPannel->rect().bottomLeft());

	// 设置初始位置
	_pSwitchPannel->move(0, 0);
	_pFormPannel->move(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, 0);

	// 防止 Pannel 遮挡 最小化 和 关闭按钮的点击事件
	NXAppBar* appBar = this->appBar();
	QWidget* appBarWindow = appBar->window();
	appBarWindow->setWindowFlags((appBarWindow->windowFlags()) | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
	appBar->raise();
}

void FKLauncherShell::_initAnimation()
{
	// 初始化动画
	_pAnimationGroup = new QParallelAnimationGroup(this);

	// 创建SwitchPannel位置动画
	_pSwitchPannelAnimation = new QPropertyAnimation(_pSwitchPannel, "pos");
	_pSwitchPannelAnimation->setDuration(1250); // 1.25秒
	_pSwitchPannelAnimation->setEasingCurve(QEasingCurve::OutCubic);

	// 创建FormPannel位置动画
	_pFormPannelAnimation = new QPropertyAnimation(_pFormPannel, "pos");
	_pFormPannelAnimation->setDuration(1250); // 1.25秒
	_pFormPannelAnimation->setEasingCurve(QEasingCurve::OutCubic);

	// 添加到动画组
	_pAnimationGroup->addAnimation(_pSwitchPannelAnimation);
	_pAnimationGroup->addAnimation(_pFormPannelAnimation);
}

void FKLauncherShell::_onTogglePannelButtonClicked()
{
	// 停止正在进行的动画
	_pAnimationGroup->stop();
	
	QPoint switchPos = _pSwitchPannel->pos();
	QPoint formPos = _pFormPannel->pos();

	// 设置动画起始值和结束值
	_pSwitchPannelAnimation->setStartValue(switchPos);
	_pFormPannelAnimation->setStartValue(formPos);
	
	if (switchPos.x() == 0) {
		// 当前SwitchPannel在左侧，移动到右侧
		_pSwitchPannelAnimation->setEndValue(QPoint(Constant::WIDGET_HEIGHT, 0));
		_pFormPannelAnimation->setEndValue(QPoint(0, 0));
	} else {
		// 当前SwitchPannel在右侧，移动到左侧
		_pSwitchPannelAnimation->setEndValue(QPoint(0, 0));
		_pFormPannelAnimation->setEndValue(QPoint(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, 0));
	}
	
	// 切换FormPannel的状态
	_pFormPannel->toggleState();
	
	// 启动动画
	_pAnimationGroup->start();
}
