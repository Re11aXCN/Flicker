#ifndef FK_SWITCH_PANNEL_H_
#define FK_SWITCH_PANNEL_H_

#include <QWidget>
#include <NXText.h>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

class FKPushButton;
class FKSwitchPannel : public QWidget
{
	Q_OBJECT
		Q_PROPERTY_CREATE_COMPLEX_H(QPoint, BottomCirclePos)
		Q_PROPERTY_CREATE_COMPLEX_H(QPoint, TopCirclePos)
		Q_PROPERTY_CREATE_BASIC_H(qreal, MoveOpacity)

public:
	explicit FKSwitchPannel(QWidget *parent = nullptr);
	~FKSwitchPannel();

	// 切换状态
	void toggleState();

Q_SIGNALS:
	void switchClicked(); // 当切换按钮被点击时发出信号

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	void _initUI();
	void _initAnimations();
	void _updateUI(); // 更新UI显示
	NXText* _pTitleText{ nullptr };
	NXText* _pDescriptionText{ nullptr };
	FKPushButton* _pSwitchBtn{ nullptr };

	QParallelAnimationGroup* _pAnimationGroup{ nullptr };
	QPropertyAnimation* _pBottomCircleAnimation{ nullptr };
	QPropertyAnimation* _pTopCircleAnimation{ nullptr };
	QPropertyAnimation* _pOpacityAnimation{ nullptr };

	// 状态标志
	bool _isLoginState{ true }; // true表示登录状态，false表示注册状态
};

#endif //!FK_SWITCH_PANNEL_H_


