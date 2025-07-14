#include "FKShadowWidget.h"

#include <QPainter>
#include <QPainterPath>
#include "FKConstant.h"
#include "Components/NXBoxShadowEffect.h"
FKShadowWidget::FKShadowWidget(QWidget* parent /*= nullptr*/)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setStyleSheet("background-color: transparent;");
    setFixedSize(200, 200);
    _pShadowEffect = new NXBoxShadowEffect(this);
    setGraphicsEffect(_pShadowEffect);
}

void FKShadowWidget::setCustomDraw(std::function<void(QPainter*, QWidget*)> customDraw)
{
    _pCustomDraw = customDraw;
}

void FKShadowWidget::setDarkOffset(const QPointF& size) {
    _pShadowEffect->setDarkOffset(size);
    update();
}

QPointF FKShadowWidget::getDarkOffset() const {
    return _pShadowEffect->getDarkOffset();
}

void FKShadowWidget::setLightOffset(const QPointF& size) {
    _pShadowEffect->setLightOffset(size);
    update();
}

QPointF FKShadowWidget::getLightOffset() const {
    return _pShadowEffect->getLightOffset();
}

void FKShadowWidget::setRotateMode(const NXWidgetType::BoxShadow::RotateMode& mode) {
    _pShadowEffect->setRotateMode(mode);
    update();
}

NXWidgetType::BoxShadow::RotateMode FKShadowWidget::getRotateMode() const {
    return _pShadowEffect->getRotateMode();
}

void FKShadowWidget::setProjectionType(const NXWidgetType::BoxShadow::ProjectionType& type) {
    _pShadowEffect->setProjectionType(type);
    update();
}

NXWidgetType::BoxShadow::ProjectionType FKShadowWidget::getProjectionType() const {
    return _pShadowEffect->getProjectionType();
}

void FKShadowWidget::setBlur(const qreal& blur) {
    _pShadowEffect->setBlur(blur);
    update();
}

qreal FKShadowWidget::getBlur() const {
    return _pShadowEffect->getBlur();
}

void FKShadowWidget::setSpread(const qreal& spread) {
    _pShadowEffect->setSpread(spread);
    update();
}

qreal FKShadowWidget::getSpread() const {
    return _pShadowEffect->getSpread();
}

void FKShadowWidget::setLightColor(const QColor& color) {
    _pShadowEffect->setLightColor(color);
    update();
}

QColor FKShadowWidget::getLightColor() const {
    return _pShadowEffect->getLightColor();
}

void FKShadowWidget::setDarkColor(const QColor& color) {
    _pShadowEffect->setDarkColor(color);
    update();
}

QColor FKShadowWidget::getDarkColor() const {
    return _pShadowEffect->getDarkColor();
}

void FKShadowWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    if (_pShadowEffect->getProjectionType() == NXWidgetType::BoxShadow::ProjectionType::Outset) {
        if (_pCustomDraw) {
            _pCustomDraw(&painter, this->parentWidget());
        }
        else {
            painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Constant::LIGHT_MAIN_BG_COLOR);
            painter.drawRect(rect());
        }
    }
    QWidget::paintEvent(event);
}

