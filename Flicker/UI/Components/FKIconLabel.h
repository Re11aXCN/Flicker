#ifndef FK_ICONLABEL_H_
#define FK_ICONLABEL_H_

#include <NXText.h>

class FKIconLabel : public NXText
{
	Q_OBJECT
public:
	explicit FKIconLabel(const QString& iconString, QWidget* parent = nullptr);
	~FKIconLabel() override;
	void setBorderRadius(int radius);

protected:
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
private:
	QString  _pIconString;
	bool _pMouseHover{ false };
};

#endif // !FK_ICONLABEL_H_