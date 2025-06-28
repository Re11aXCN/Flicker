#include "FKLineEdit.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <QStyleOption>
#include <QRegion>

#include <NXTheme.h>
#include "FKConstant.h"

FKLineEdit::FKLineEdit(const ButtonConfig& buttonConfig, QWidget* parent)
    : NXLineEdit(parent)
    , _pButtonText(buttonConfig.ButtonText)
    , _pButtonWidth(buttonConfig.ButtonWidth)
    , _pButtonEnabled(buttonConfig.ButtonEnabled)
    , _pButtonVisible(buttonConfig.ButtonVisible)
{
    //_pButtonWidth = this->fontMetrics().horizontalAdvance(_pButtonText);
	// 设置文本边距，为右侧按钮区域留出空间
	setTextMargins(0, 0, _pButtonWidth + 5, 0);
	setCursor(Qt::IBeamCursor);
}

FKLineEdit::~FKLineEdit()
{
}

void FKLineEdit::paintEvent(QPaintEvent* event)
{
    NXLineEdit::paintEvent(event);

    // 如果按钮不可见，则不绘制
    if (!_pButtonVisible) return;

    QPainter painter(this);
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    QRect btnRect = _getButtonRect();

    // 设置按钮颜色
    QColor btnColor;
    if (!_pButtonEnabled || !isInputValid()) {
        btnColor = Constant::DESCRIPTION_TEXT_COLOR;
    } else if (_pButtonPressed) {
        btnColor = NXThemeColor(NXThemeType::Light, PrimaryPress);
    } else if (_pButtonHovered) {
        btnColor = NXThemeColor(NXThemeType::Light, PrimaryHover);
    } else {
        btnColor = NXThemeColor(NXThemeType::Light, PrimaryNormal);
    }

	painter.setPen(btnColor);
	painter.drawText(btnRect, Qt::AlignCenter, _pButtonText);

	painter.setPen(Qt::NoPen);
	painter.setBrush(NXThemeColor(NXThemeType::Light, PrimaryNormal));
    painter.drawRoundedRect(QRectF(QPointF{ btnRect.x() - (qreal)getContentsPaddings().left(), btnRect.y() + btnRect.height() * 0.2 },
        QSizeF{ 3.0, btnRect.height() - btnRect.height() * 0.4 }), 2, 2);

    painter.restore();
}

void FKLineEdit::mouseMoveEvent(QMouseEvent* event)
{
    if (_pButtonVisible && _isMousePosInButtonRect(event->pos())) {
        if (_pButtonEnabled && isInputValid()) {
            _pButtonHovered = true;
            setCursor(Qt::PointingHandCursor);
        }
        else {
            setCursor(Qt::ForbiddenCursor);
        }
    } else {
        _pButtonHovered = false;
        setCursor(Qt::IBeamCursor);
    }
    
    update();
    NXLineEdit::mouseMoveEvent(event);
}

void FKLineEdit::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && _pButtonVisible && 
        _isMousePosInButtonRect(event->pos()) && _pButtonEnabled && isInputValid())
    {
        _pButtonPressed = true;
        update();
        event->accept();
    } else {
        NXLineEdit::mousePressEvent(event);
    }
}

void FKLineEdit::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && _pButtonPressed)
    {
        _pButtonPressed = false;
		if (_isMousePosInButtonRect(event->pos()) && _pButtonEnabled && isInputValid())
		{
			Q_EMIT buttonClicked();
		}
        
        update();
        event->accept();
    } else {
        NXLineEdit::mouseReleaseEvent(event);
    }
}

void FKLineEdit::leaveEvent(QEvent* event)
{
	_pButtonHovered = false;
	_pButtonPressed = false;
    setCursor(Qt::IBeamCursor);
    update();
    NXLineEdit::leaveEvent(event);
}

QRect FKLineEdit::_getButtonRect() const
{
    return QRect(width() - _pButtonWidth - getContentsPaddings().left(), 0, _pButtonWidth, height());
}

bool FKLineEdit::_isMousePosInButtonRect(const QPoint& point) const
{
    return _getButtonRect().contains(point);
}

void FKLineEdit::setButtonText(const QString& text)
{
    if (_pButtonText != text) {
        _pButtonText = text;
        update();
    }
}
void FKLineEdit::setButtonWidth(int width)
{
    if (_pButtonWidth != width && width > 0) {
        _pButtonWidth = width;
        setTextMargins(0, 0, _pButtonWidth + 5, 0);
        update();
    }
}

void FKLineEdit::setButtonEnabled(bool enabled)
{
    if (_pButtonEnabled != enabled) {
        _pButtonEnabled = enabled;
        update();
    }
}

void FKLineEdit::setButtonVisible(bool visible)
{
    if (_pButtonVisible != visible) {
        _pButtonVisible = visible;
        // 更新文本边距
        if (visible) {
            setTextMargins(0, 0, _pButtonWidth + 5, 0);
        } else {
            setTextMargins(0, 0, 0, 0);
        }
        update();
    }
}

void FKLineEdit::setValidationRegExp(const QRegularExpression& regExp)
{
    _pValidationRegExp = regExp;
    update(); // 更新按钮状态
}

bool FKLineEdit::isInputValid() const
{
    // 如果没有设置验证正则表达式，则认为输入始终有效
    if (!_pValidationRegExp.isValid())
        return true;
        
    // 使用正则表达式验证输入
    QRegularExpressionMatch match = _pValidationRegExp.match(text());
    return match.hasMatch();
}
