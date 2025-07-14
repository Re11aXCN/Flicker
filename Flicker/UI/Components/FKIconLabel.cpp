#include "FKIconLabel.h"
#include <QPainter>
#include <QEvent>
#include <QStyleOption>

#include "FKConstant.h"
FKIconLabel::FKIconLabel(const QString& iconString, QWidget* parent /*= nullptr*/)
    : NXText(parent)
    , _pIconString(iconString)
{
}

FKIconLabel::~FKIconLabel()
{

}

void FKIconLabel::setBorderRadius(int radius)
{

}

void FKIconLabel::enterEvent(QEnterEvent* event)
{
    Q_UNUSED(event)
    _pMouseHover = true;
    setCursor(Qt::PointingHandCursor);
    update();
    NXText::enterEvent(event);
}

void FKIconLabel::leaveEvent(QEvent* event)
{
    Q_UNUSED(event)
    _pMouseHover = false;
    setCursor(Qt::ArrowCursor);
    update();
    NXText::leaveEvent(event);
}

void FKIconLabel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.save();
    // 绘制背景
    QStyleOptionFrame option;
    initStyleOption(&option);
    style()->drawPrimitive(QStyle::PE_PanelButtonCommand, &option, &painter, this);

    // 绘制图标
    QFont font = QFont("iconfont");
    
    font.setPixelSize(25); // 25px 大小
    painter.setFont(font);

    QColor currColor = _pMouseHover ? Constant::LAUNCHER_ICON_HOVER_COLOR : Constant::LAUNCHER_ICON_NORMAL_COLOR;
    // 根据悬停状态设置颜色
    painter.setPen(currColor);
    painter.drawText(rect(), Qt::AlignCenter, _pIconString);
    painter.setPen(QPen(currColor, 2));

    QRect circleRect{ rect().adjusted(1, 1, -1, -1) };
    painter.drawRoundedRect(circleRect, circleRect.width() / 2, circleRect.height() / 2); // 绘制圆形边框
    painter.restore();
    NXText::paintEvent(event);
}
