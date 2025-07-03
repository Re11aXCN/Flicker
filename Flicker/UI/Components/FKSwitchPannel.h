#ifndef FK_SWITCH_PANNEL_H_
#define FK_SWITCH_PANNEL_H_

#include <QWidget>
#include <NXText.h>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>

class FKPushButton;
class FKSwitchPannel : public QWidget
{
	Q_OBJECT
		Q_PROPERTY_CREATE_COMPLEX_H(QPoint, BottomCirclePos)
		Q_PROPERTY_CREATE_COMPLEX_H(QPoint, TopCirclePos)
		Q_PROPERTY_CREATE_BASIC_H(qreal, LoginOpacity)
		Q_PROPERTY_CREATE_BASIC_H(qreal, RegisterOpacity)

public:
	explicit FKSwitchPannel(QWidget *parent = nullptr);
	~FKSwitchPannel();

	// 切换状态
	void toggleFormType();

Q_SIGNALS:
	void switchClicked(); // 当切换按钮被点击时发出信号

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	void _initUI();
	void _initAnimations();
	void _updateUI(); // 更新UI显示
	NXText* _pLoginTitleText{ nullptr };
	NXText* _pLoginDescriptionText{ nullptr };
	FKPushButton* _pLoginSwitchBtn{ nullptr };
	QGraphicsOpacityEffect* _pLoginTitleEffect{ nullptr };
	QGraphicsOpacityEffect* _pLoginDescriptionEffect{ nullptr };
	QGraphicsOpacityEffect* _pLoginSwitchBtnEffect{ nullptr };

	NXText* _pRegisterTitleText{ nullptr };
	NXText* _pRegisterDescriptionText{ nullptr };
	FKPushButton* _pRegisterSwitchBtn{ nullptr };
	QGraphicsOpacityEffect* _pRegisterTitleEffect{ nullptr };
	QGraphicsOpacityEffect* _pRegisterDescriptionEffect{ nullptr };
	QGraphicsOpacityEffect* _pRegisterSwitchBtnEffect{ nullptr };

	QParallelAnimationGroup* _pAnimationGroup{ nullptr };
	QPropertyAnimation* _pBottomCircleAnimation{ nullptr };
	QPropertyAnimation* _pTopCircleAnimation{ nullptr };
	QPropertyAnimation* _pLoginOpacityAnimation{ nullptr };
	QPropertyAnimation* _pRegisterOpacityAnimation{ nullptr };

	// 状态标志
	bool _isLoginState{ true }; // true表示登录状态，false表示注册状态
};

#endif //!FK_SWITCH_PANNEL_H_


