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
	Q_PROPERTY_CREATE_BASIC_H(qreal, LoginOpacity)
	Q_PROPERTY_CREATE_BASIC_H(qreal, RegisterOpacity)

public:
	explicit FKSwitchPannel(QWidget *parent = nullptr);
	~FKSwitchPannel();

	// 切换状态
	Q_SLOT void toggleFormType();
	Q_SLOT void clickSwitchBtn();
Q_SIGNALS:
	void switchClicked(); // 当切换按钮被点击时发出信号

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent *event)  override;
private:
	void _initUI();
	void _initAnimations();
	void _updateChildGeometry();

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
	QPropertyAnimation* _pLoginOpacityAnimation{ nullptr };
	QPropertyAnimation* _pRegisterOpacityAnimation{ nullptr };

	bool _pIsLoginState{ true };
};

#endif //!FK_SWITCH_PANNEL_H_


