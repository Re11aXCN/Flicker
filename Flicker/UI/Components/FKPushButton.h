#ifndef FK_PUSHBUTTON_H_
#define FK_PUSHBUTTON_H_
#include <QPropertyAnimation>
#include <NXPushButton.h>

class FKPushButton : public NXPushButton {
	Q_OBJECT
		Q_PROPERTY_CREATE_BASIC_H(int, WaveRadius)
		Q_PROPERTY_CREATE_BASIC_H(int, WaveOpacity)
public:
	explicit FKPushButton(QWidget* parent = nullptr);
	explicit FKPushButton(const QString& text, QWidget* parent = nullptr);
	~FKPushButton();


protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	QPoint _pMousePressPos;
	QPropertyAnimation* _pWaveRadiusAnimation;
	QPropertyAnimation* _pWaveOpacityAnimation;
};
#endif // !FK_PUSHBUTTON_H_