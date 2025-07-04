#pragma once
#include <QWidget>
#include <QPainter>
#include <QPainterPath>

class CircleWidget : public QWidget {
public:
	explicit CircleWidget(QWidget* parent = nullptr);
	~CircleWidget() override = default;
protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override; // 新增重设大小事件
private:
	QRegion maskRegion; // 用于存储圆形遮罩区域
};