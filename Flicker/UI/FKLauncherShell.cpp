#include "FKLauncherShell.h"
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

#include <NXIcon.h>
#include "Common/utils/utils.h"

#include "Self/FKDef.h"
#include "Components/FKFormPannel.h"
#include "Components/FKSwitchPannel.h"

FKLauncherShell::FKLauncherShell(QWidget* parent /*= nullptr*/)
    : NXWidget(parent)
    , _pCircleX{ 0.0 }
{
    _initUi();
    _initAnimation();
    QObject::connect(_pSwitchPannel, &FKSwitchPannel::switchClicked, this, &FKLauncherShell::_onSwitchPannelButtonClicked);
    QObject::connect(_pFormPannel, &FKFormPannel::switchClicked, this, &FKLauncherShell::_onSwitchPannelButtonClicked);

}

FKLauncherShell::~FKLauncherShell()
{
    delete _pAnimationGroup;
}
void FKLauncherShell::setCircleX(qreal x) { 
    _pCircleX = x;
    update();
    Q_EMIT pCircleXChanged();
}
void FKLauncherShell::ShowMessage(const QString& title, const QString& text, NXMessageBarType::MessageMode mode, NXMessageBarType::PositionPolicy position, int displayMsec /*= 2000*/)
{
    _pMessageButton->setBarTitle(title);
    _pMessageButton->setBarText(text);
    _pMessageButton->setMessageMode(mode);
    _pMessageButton->setPositionPolicy(position);
    _pMessageButton->setDisplayMsec(displayMsec);
    Q_EMIT _pMessageButton->showMessage();
}

void FKLauncherShell::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    NXWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    _drawEdgeShadow(painter, _pSwitchPannel->rect(), Constant::SWITCH_BOX_SHADOW_COLOR, Constant::SWITCH_BOX_SHADOW_WIDTH);
}

void FKLauncherShell::_initUi()
{
    setAppBarHeight(30);
    setIsFixedSize(true);
    setFixedSize(Constant::WIDGET_WIDTH, Constant::WIDGET_HEIGHT);
    setWindowTitle("Flicker");
    setWindowIcon(QIcon(":/Resource/ico/Weixin_IDI_ICON1.ico"));
    setCustomBackgroundColor(Constant::LIGHT_MAIN_BG_COLOR, Constant::DARK_MAIN_BG_COLOR);
    setWindowButtonFlags(NXAppBarType::MinimizeButtonHint | NXAppBarType::CloseButtonHint);
    _pMessageButton = new NXMessageButton(this);
    _pFormPannel = new FKFormPannel(this);
    _pSwitchPannel = new FKSwitchPannel(this);
    _pShadowWidget = new NXShadowWidget(this);

    _pMessageButton->setFixedSize(1, 1);
    _pMessageButton->setVisible(false);

    _pFormPannel->setFixedSize(Constant::WIDGET_HEIGHT, Constant::WIDGET_HEIGHT);
    _pSwitchPannel->setFixedSize(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, Constant::WIDGET_HEIGHT);

    _pShadowWidget->setFixedSize(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, Constant::WIDGET_HEIGHT);
    _pShadowWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    _pShadowWidget->setBlur(30.0);
    _pShadowWidget->setLightColor(Constant::SWITCH_CIRCLE_DARK_SHADOW_COLOR);
    _pShadowWidget->setDarkColor(Constant::SWITCH_CIRCLE_LIGHT_SHADOW_COLOR);
    _pShadowWidget->setLightOffset({ -5,-5 });
    _pShadowWidget->setDarkOffset({ 5,5 });
    _pShadowWidget->setProjectionType(NXWidgetType::BoxShadow::ProjectionType::Outset);
    _pShadowWidget->setRotateMode(NXWidgetType::BoxShadow::RotateMode::Rotate45);
    _pShadowWidget->setCustomDraw([this](QPainter* painter, QWidget* p) {
        painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        painter->setPen(Qt::NoPen);
        painter->setBrush(Constant::LIGHT_MAIN_BG_COLOR);
        QPainterPath rectPath;
        rectPath.addRect(_pShadowWidget->rect());
        // 创建凹槽路径
        QPainterPath notchPath;
        qreal X = qobject_cast<FKLauncherShell*>(p)->getCircleX();

        // 顶部凹槽 (只需要绘制下半圆弧,180°到360°)
        {
            QRectF topCircleRect(QPointF{ 250 - X, -180 }, QSizeF{ 300, 300 });
            notchPath.moveTo(topCircleRect.left(), topCircleRect.center().y());
            notchPath.arcTo(topCircleRect, 180, 180);
            notchPath.closeSubpath();
        }

        // 底部凹槽 (只需要绘制上半圆弧,0°到180°)
        {
            QRectF bottomCircleRect(QPointF{ -250 + X, 460 }, QSizeF{ 500, 500 });
            notchPath.moveTo(bottomCircleRect.left(), bottomCircleRect.center().y());
            notchPath.arcTo(bottomCircleRect, 0, 180);
            notchPath.closeSubpath();
        }
        //qreal x = qobject_cast<FKLauncherShell*>(p)->getTopCircleX();
        //notchPath.moveTo(400 - x, 120);
        //notchPath.lineTo(400 - x, 0);
        //notchPath.lineTo(250 - x, 0);
        //notchPath.arcTo(QRectF{ QPointF{ 250 - x, -180 }, QSizeF{ 300, 300 } }, 180, 90); // 从B点画弧线到O点
        //notchPath.closeSubpath();
        // 从矩形中减去凹槽形成最终路径
        QPainterPath finalPath = rectPath.subtracted(notchPath);
        painter->drawPath(finalPath);
        });
    _pFormPannel->move(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, 0);
    _pSwitchPannel->move(0, 0);
    _pShadowWidget->move(0, 0);
    
    _pFormPannel->lower();
    _pShadowWidget->raise();
    _pSwitchPannel->raise();

    // 防止 Pannel 遮挡 最小化 和 关闭按钮的点击事件
    NXAppBar* appBar = this->appBar();
    QWidget* appBarWindow = appBar->window();
    appBarWindow->setWindowFlags((appBarWindow->windowFlags()) | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    appBar->raise();
}

void FKLauncherShell::_initAnimation()
{
    _pAnimationGroup = new QParallelAnimationGroup(this);

    _pSwitchPannelAnimation = new QPropertyAnimation(_pSwitchPannel, "pos");
    _pSwitchPannelAnimation->setDuration(1250); 
    _pSwitchPannelAnimation->setEasingCurve(QEasingCurve::OutCubic);

    _pShadowWidgetAnimation = new QPropertyAnimation(_pShadowWidget, "pos");
    _pShadowWidgetAnimation->setDuration(1250); 
    _pShadowWidgetAnimation->setEasingCurve(QEasingCurve::OutCubic);

    _pShadowWidgetWidthAnimation = new QPropertyAnimation(_pShadowWidget, "minimumWidth");
    _pShadowWidgetWidthAnimation->setDuration(1250);
    _pShadowWidgetWidthAnimation->setEasingCurve(QEasingCurve::OutCubic);
    _pShadowWidgetWidthAnimation->setKeyValues({ {0.0, 400}, {0.5, 500}, {1.0, 400} });

    _pSwitchPannelWidthAnimation = new QPropertyAnimation(_pSwitchPannel, "minimumWidth");
    _pSwitchPannelWidthAnimation->setDuration(1250);
    _pSwitchPannelWidthAnimation->setEasingCurve(QEasingCurve::OutCubic);
    _pSwitchPannelWidthAnimation->setKeyValues({ {0.0, 400}, {0.5, 500}, {1.0, 400} });

    _pFormPannelAnimation = new QPropertyAnimation(_pFormPannel, "pos");
    _pFormPannelAnimation->setDuration(1250); 
    _pFormPannelAnimation->setEasingCurve(QEasingCurve::OutCubic);

    _pSwitchCircleXAnimation = new QPropertyAnimation(this, "pCircleX");
    _pSwitchCircleXAnimation->setDuration(1250);
    _pSwitchCircleXAnimation->setEasingCurve(QEasingCurve::OutCubic);

    _pAnimationGroup->addAnimation(_pSwitchPannelAnimation);
    _pAnimationGroup->addAnimation(_pShadowWidgetAnimation);
    _pAnimationGroup->addAnimation(_pFormPannelAnimation);
    _pAnimationGroup->addAnimation(_pSwitchPannelWidthAnimation);
    _pAnimationGroup->addAnimation(_pShadowWidgetWidthAnimation);
    _pAnimationGroup->addAnimation(_pSwitchCircleXAnimation);
}

void FKLauncherShell::_onSwitchPannelButtonClicked()
{
    _pAnimationGroup->stop();
    
    QPoint switchPos = _pSwitchPannel->pos();
    QPoint shadowPos = _pShadowWidget->pos();
    QPoint formPos = _pFormPannel->pos();

    // 设置动画起始值和结束值
    _pSwitchPannelAnimation->setStartValue(switchPos);
    _pShadowWidgetAnimation->setStartValue(shadowPos);
    _pFormPannelAnimation->setStartValue(formPos);
    _pSwitchCircleXAnimation->setStartValue(_pCircleX);
    if (switchPos.x() == 0) {
        _pSwitchPannelAnimation->setEndValue(QPoint(Constant::WIDGET_HEIGHT, 0));
        _pShadowWidgetAnimation->setEndValue(QPoint(Constant::WIDGET_HEIGHT, 0));
        _pFormPannelAnimation->setEndValue(QPoint(0, 0));
        _pSwitchCircleXAnimation->setEndValue(250.0);
    } else {
        _pSwitchPannelAnimation->setEndValue(QPoint(0, 0));
        _pShadowWidgetAnimation->setEndValue(QPoint(0, 0));
        _pFormPannelAnimation->setEndValue(QPoint(Constant::WIDGET_WIDTH - Constant::WIDGET_HEIGHT, 0));
        _pSwitchCircleXAnimation->setEndValue(0.0);
    }
    
    _pFormPannel->toggleFormType();
    _pAnimationGroup->start();
}

void FKLauncherShell::_drawEdgeShadow(QPainter& painter, const QRect& rect, const QColor& color, int shadowWidth) const noexcept
{
    QPoint shadowRectTopLeft, shadowRectBottomRight;
    QLinearGradient gradient;
    if (_pSwitchPannel->pos().x() < 160) {
        shadowRectTopLeft = _pSwitchPannel->pos() + QPoint{ rect.width(), 0 };
        shadowRectBottomRight = _pSwitchPannel->pos() + QPoint{ rect.width() + shadowWidth, rect.height() };
        gradient = { shadowRectTopLeft, shadowRectTopLeft + QPoint{ shadowWidth, 0 } };
        
    }
    else {
        shadowRectTopLeft = _pSwitchPannel->pos() - QPoint{ shadowWidth, 0 };
        shadowRectBottomRight = _pSwitchPannel->pos() + QPoint{ 0, rect.height() };
        gradient = { _pSwitchPannel->pos(), shadowRectTopLeft };
    }
    gradient.setColorAt(0, color);
    gradient.setColorAt(1, Qt::transparent);
    painter.fillRect(QRect{ shadowRectTopLeft, shadowRectBottomRight }, gradient);
//#define DrawEdgeShadow(Left, Right)\
//    QLinearGradient gradient(rect.top##Left(), contentRect.top##Left());\
//    gradient.setColorAt(0, color);\
//    gradient.setColorAt(1, Qt::transparent);\
//    painter.fillRect(QRect{ rect.top##Left(), contentRect.bottom##Right() }, gradient)
//
//    if (_pFormType == Launcher::FormType::Login || _pFormType == Launcher::FormType::Authentication) {
//        QRect contentRect = rect.adjusted(shadowWidth, 0, 0, 0);
//        DrawEdgeShadow(Left, Right);
//    }
//    else {
//        QRect contentRect = rect.adjusted(0, 0, -shadowWidth, 0);
//        DrawEdgeShadow(Right, Left);
//    }
//#undef DrawEdgeShadow
}
