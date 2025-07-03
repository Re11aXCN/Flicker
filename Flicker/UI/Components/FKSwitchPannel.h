#ifndef FK_SWITCH_PANNEL_H_
#define FK_SWITCH_PANNEL_H_

#include <QWidget>
#include <QGraphicsOpacityEffect>
#include <NXText.h>

class QPropertyAnimation;
class QSequentialAnimationGroup;
class QParallelAnimationGroup;

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
	Q_SLOT void toggleFormType();

Q_SIGNALS:
	void switchClicked(); // 当切换按钮被点击时发出信号

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	void _initUI();
	void _initAnimations();
	Q_SLOT void _updateLoginOpacity(const QVariant& value);
	Q_SLOT void _updateRegisterOpacity(const QVariant& value);
	NXText* _pLoginTitleText{ nullptr };
	NXText* _pLoginDescriptionText{ nullptr };
	QGraphicsOpacityEffect* _pLoginTitleEffect{ nullptr };
	QGraphicsOpacityEffect* _pLoginDescriptionEffect{ nullptr };

	NXText* _pRegisterTitleText{ nullptr };
	NXText* _pRegisterDescriptionText{ nullptr };
	QGraphicsOpacityEffect* _pRegisterTitleEffect{ nullptr };
	QGraphicsOpacityEffect* _pRegisterDescriptionEffect{ nullptr };

	QGraphicsOpacityEffect* _pSwitchBtnEffect{ nullptr };
	FKPushButton* _pSwitchBtn{ nullptr };

	QParallelAnimationGroup* _pAnimationGroup{ nullptr };
	QPropertyAnimation* _pBottomCircleAnimation{ nullptr };
	QPropertyAnimation* _pTopCircleAnimation{ nullptr };
	QPropertyAnimation* _pLoginOpacityAnimation{ nullptr };
	QPropertyAnimation* _pRegisterOpacityAnimation{ nullptr };

	// 状态标志
	bool _isLoginState{ true }; // true表示登录状态，false表示注册状态
};

#endif //!FK_SWITCH_PANNEL_H_


