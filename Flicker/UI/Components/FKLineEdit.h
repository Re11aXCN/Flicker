#ifndef FK_LINEEDIT_H_
#define FK_LINEEDIT_H_

#include <NXLineEdit.h>
#include <QRegularExpression>

class FKLineEdit : public NXLineEdit
{
    Q_OBJECT
		Q_PROPERTY_CREATE_COMPLEX_H(QRegularExpression, ValidationRegExp)
		Q_PROPERTY_CREATE_COMPLEX_H(QString, ButtonText)
		Q_PROPERTY_CREATE_BASIC_H(int, ButtonWidth)
        Q_PROPERTY_CREATE_BASIC_H(bool, ButtonEnabled)
		Q_PROPERTY_CREATE_BASIC_H(bool, ButtonVisible)
		Q_PROPERTY_CREATE_D(bool, ButtonHovered)
		Q_PROPERTY_CREATE_D(bool, ButtonPressed)
public:
    struct ButtonConfig {
        QString ButtonText{ "..." };
        int ButtonWidth { 20 };
        bool ButtonEnabled { true };
        bool ButtonVisible { true };
    };
	explicit FKLineEdit(const ButtonConfig& buttonConfig, QWidget* parent = nullptr);

    ~FKLineEdit();

    bool isInputValid() const;

    Q_SIGNAL void buttonClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QRect _getButtonRect() const;
    bool _isMousePosInButtonRect(const QPoint& point) const;
};

#endif // !FK_LINEEDIT_H_

