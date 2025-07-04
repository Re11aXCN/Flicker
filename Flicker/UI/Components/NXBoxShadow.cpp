#include "NXBoxShadow.h"

#include <QPainter>
#include "NXTheme.h"

QT_BEGIN_NAMESPACE
extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter* p, QImage& blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

constexpr qreal Radian45 = 45.0 * M_PI / 180.0;
constexpr qreal Radian135 = 135.0 * M_PI / 180.0;
constexpr qreal Radian225 = 225.0 * M_PI / 180.0;
constexpr qreal Radian315 = 315.0 * M_PI / 180.0;

NXBoxShadowPrivate::NXBoxShadowPrivate(QObject* parent)
	: QObject{ parent }
{
}
NXBoxShadowPrivate::~NXBoxShadowPrivate()
{

}

void NXBoxShadowPrivate::_drawInsetShadow(QPainter* painter, const QPixmap& pixmap, const QPoint& pos, const QPoint& offset)
{
	QImage lightShadowImg = _createBaseShadowImage(pixmap, _pLightColor);
	QImage darkShadowImg = _createBaseShadowImage(pixmap, _pDarkColor);
	// 组合两种阴影
	{
		QPainter shadowPainter(&lightShadowImg);
		shadowPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		shadowPainter.drawImage(offset, darkShadowImg);
		shadowPainter.end();
	}
	// 重叠区域清除
	{
		const int X = qAbs(offset.x()), Y = qAbs(offset.y());
		QRect innerRect{ QPoint{}, pixmap.size() / pixmap.devicePixelRatioF() };
		innerRect.adjust(X, Y, -X, -Y);

		QPainter shadowPainter(&lightShadowImg);
		shadowPainter.setCompositionMode(QPainter::CompositionMode_Clear);
		shadowPainter.fillRect(innerRect, Qt::transparent);
		shadowPainter.end();
	}

	// 应用模糊效果
	QImage blurred = _applyBlur(lightShadowImg, _pBlur, false);

	// 组合最终图像
	QImage resultImage = pixmap.toImage();
	{
		QPainter resultPainter(&resultImage);
		resultPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		resultPainter.drawImage(blurred.rect(), blurred);
		resultPainter.end();
	}

	painter->drawImage(pos, resultImage);
}

void NXBoxShadowPrivate::_drawOutsetShadow(QPainter* painter, const QPixmap& pixmap, const QPoint& pos, const QPoint& offset)
{
	// 创建基础阴影图像
	QImage baseShadow = _createBaseShadowImage(pixmap, _pLightColor);

	// 应用模糊效果
	QImage blurred = _applyBlur(baseShadow, _pBlur, true);

	QImage lightShadow = _applyColor(blurred, _pLightColor);
	QImage darkShadow = _applyColor(blurred, _pDarkColor);
	painter->drawImage(pos + offset, lightShadow);
	painter->drawImage(pos - offset, darkShadow);

	painter->drawPixmap(pos, pixmap);
}

QImage NXBoxShadowPrivate::_createBaseShadowImage(const QPixmap& pixmap, const QColor& color)
{
	QImage image(pixmap.size(), QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(pixmap.devicePixelRatioF());
	image.fill(0);

	QPainter painter(&image);
	painter.drawPixmap(0, 0, pixmap);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	painter.fillRect(image.rect(), color);
	painter.end();

	return image;
}

QImage NXBoxShadowPrivate::_applyBlur(const QImage& source, qreal blur, bool alphaOnly)
{
	QImage blurred(source.size(), QImage::Format_ARGB32_Premultiplied);
	blurred.setDevicePixelRatio(source.devicePixelRatioF());
	blurred.fill(0);

	QPainter painter(&blurred);
	qt_blurImage(&painter, const_cast<QImage&>(source), blur, false, alphaOnly);
	painter.end();

	return blurred;
}

QImage NXBoxShadowPrivate::_applyColor(const QImage& image, const QColor& color)
{
	QImage result = image;
	QPainter painter(&result);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	painter.fillRect(result.rect(), color);
	return result;
}

void NXBoxShadowPrivate::_drawCircleInsetShadow(QPainter* painter, const QPixmap& pixmap, const QPoint& pos, qreal spread, qreal blur)
{
	// 获取圆形区域
	QRect rect = pixmap.rect();
	int diameter = qMin(rect.width(), rect.height());
	QRect circleRect(0, 0, diameter, diameter);
	circleRect.moveCenter(rect.center());
	
	// 创建两个阴影图像
	QImage shadowImage(pixmap.size(), QImage::Format_ARGB32_Premultiplied);
	shadowImage.setDevicePixelRatio(pixmap.devicePixelRatioF());
	shadowImage.fill(0);
	
	// 绘制原始圆形
	QPainter shadowPainter(&shadowImage);
	shadowPainter.setRenderHints(QPainter::Antialiasing);
	shadowPainter.setPen(Qt::NoPen);
	shadowPainter.setBrush(Qt::white);
	shadowPainter.drawEllipse(circleRect);
	shadowPainter.end();
	
	// 创建内阴影图像
	QImage innerShadow = shadowImage;
	QRect innerCircleRect = circleRect.adjusted(spread, spread, -spread, -spread);
	
	// 清除内部区域
	QPainter innerPainter(&innerShadow);
	innerPainter.setRenderHints(QPainter::Antialiasing);
	innerPainter.setCompositionMode(QPainter::CompositionMode_Clear);
	innerPainter.setPen(Qt::NoPen);
	innerPainter.setBrush(Qt::black);
	innerPainter.drawEllipse(innerCircleRect);
	innerPainter.end();
	
	// 应用模糊效果
	QImage blurred = _applyBlur(innerShadow, blur, false);
	
	// 应用颜色
	QImage lightShadow = _applyColor(blurred, _pLightColor);
	QImage darkShadow = _applyColor(blurred, _pDarkColor);
	
	// 创建最终图像
	QImage resultImage = pixmap.toImage();
	QPainter resultPainter(&resultImage);
	resultPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	
	// 绘制阴影
	resultPainter.drawImage(0, 0, lightShadow);
	resultPainter.drawImage(0, 0, darkShadow);
	resultPainter.end();
	
	// 绘制最终图像
	painter->drawImage(pos, resultImage);
}

void NXBoxShadowPrivate::_drawCircleOutsetShadow(QPainter* painter, const QPixmap& pixmap, const QPoint& pos, qreal spread, qreal blur)
{
	// 获取圆形区域
	QRect rect = pixmap.rect();
	int diameter = qMin(rect.width(), rect.height());
	QRect circleRect(0, 0, diameter, diameter);
	circleRect.moveCenter(rect.center());
	
	// 创建阴影图像
	QImage shadowImage(pixmap.size(), QImage::Format_ARGB32_Premultiplied);
	shadowImage.setDevicePixelRatio(pixmap.devicePixelRatioF());
	shadowImage.fill(0);
	
	// 绘制原始圆形
	QPainter shadowPainter(&shadowImage);
	shadowPainter.setRenderHints(QPainter::Antialiasing);
	shadowPainter.setPen(Qt::NoPen);
	shadowPainter.setBrush(Qt::white);
	shadowPainter.drawEllipse(circleRect);
	shadowPainter.end();
	
	// 应用模糊效果
	QImage blurred = _applyBlur(shadowImage, blur, false);
	
	// 创建扩展的阴影图像
	QImage expandedShadow(pixmap.size().width() + 2 * spread, pixmap.size().height() + 2 * spread, QImage::Format_ARGB32_Premultiplied);
	expandedShadow.setDevicePixelRatio(pixmap.devicePixelRatioF());
	expandedShadow.fill(0);
	
	// 将模糊后的图像绘制到扩展图像中心
	QPainter expandedPainter(&expandedShadow);
	expandedPainter.drawImage(spread, spread, blurred);
	expandedPainter.end();
	
	// 应用颜色
	QImage lightShadow = _applyColor(expandedShadow, _pLightColor);
	QImage darkShadow = _applyColor(expandedShadow, _pDarkColor);
	
	// 绘制阴影
	painter->drawImage(pos.x() - spread, pos.y() - spread, lightShadow);
	
	// 绘制原始图像
	painter->drawPixmap(pos, pixmap);
}

NXBoxShadow::NXBoxShadow(QObject* parent /*= nullptr*/)
	: QGraphicsEffect{ parent }, d_ptr(new NXBoxShadowPrivate())
{
	Q_D(NXBoxShadow);
	d->q_ptr = this;
	d->_radian = Radian45;
	d->_pBlur = 12.0;
	d->_pSpread = 8.0;
	d->_pLightColor = NXThemeColor(d->_themeMode, BasicBorder);
	d->_pDarkColor = NXThemeColor(d->_themeMode, BasicBorder);
	d->_pRotateMode = NXWidgetType::BoxShadow::RotateMode::Rotate45;
	d->_pProjectionType = NXWidgetType::BoxShadow::ProjectionType::Inset;
	d->_pCircleShadow = false;
}

NXBoxShadow::~NXBoxShadow()
{

}

void NXBoxShadow::setRotateMode(NXWidgetType::BoxShadow::RotateMode mode) {
	Q_D(NXBoxShadow);
	switch (mode) {
	case NXWidgetType::BoxShadow::RotateMode::Rotate45:
		d->_radian = Radian45;
		break;
	case NXWidgetType::BoxShadow::RotateMode::Rotate135:
		d->_radian = Radian135;
		break;
	case NXWidgetType::BoxShadow::RotateMode::Rotate225:
		d->_radian = Radian225;
		break;
	case NXWidgetType::BoxShadow::RotateMode::Rotate315:
		d->_radian = Radian315;
		break;
	default:
		break;
	}
	d->_pRotateMode = mode;
	update();
}

NXWidgetType::BoxShadow::RotateMode NXBoxShadow::getRotateMode() const {
	Q_D(const NXBoxShadow);
	return d->_pRotateMode;
}

void NXBoxShadow::setProjectionType(NXWidgetType::BoxShadow::ProjectionType type) {
	Q_D(NXBoxShadow);
	d->_pProjectionType = type;
	update();
}

NXWidgetType::BoxShadow::ProjectionType NXBoxShadow::getProjectionType() const {
	Q_D(const NXBoxShadow);
	return d->_pProjectionType;
}

void NXBoxShadow::setBlur(qreal blur) {
	Q_D(NXBoxShadow);
	d->_pBlur = blur;
	update();
}

qreal NXBoxShadow::getBlur() const {
	Q_D(const NXBoxShadow);
	return d->_pBlur;
}

void NXBoxShadow::setSpread(qreal spread) {
	Q_D(NXBoxShadow);
	d->_pSpread = spread;
	update();
}

qreal NXBoxShadow::getSpread() const {
	Q_D(const NXBoxShadow);
	return d->_pSpread;
}

void NXBoxShadow::setLightColor(const QColor& color) {
	Q_D(NXBoxShadow);
	d->_pLightColor = color;
	update();
}

QColor NXBoxShadow::getLightColor() const {
	Q_D(const NXBoxShadow);
	return d->_pLightColor;
}

void NXBoxShadow::setDarkColor(const QColor& color) {
	Q_D(NXBoxShadow);
	d->_pDarkColor = color;
	update();
}

QColor NXBoxShadow::getDarkColor() const {
	Q_D(const NXBoxShadow);
	return d->_pDarkColor;
}

void NXBoxShadow::setCircleShadow(bool isCircle) {
	Q_D(NXBoxShadow);
	d->_pCircleShadow = isCircle;
	update();
}

bool NXBoxShadow::getCircleShadow() const {
	Q_D(const NXBoxShadow);
	return d->_pCircleShadow;
}

QRectF NXBoxShadow::boundingRectFor(const QRectF& rect) const
{
	Q_D(const NXBoxShadow);
	QRectF boundingRect = rect.united(rect.translated(0, 0));
	
	// 对于内阴影，不需要扩展边界
	if (d->_pProjectionType == NXWidgetType::BoxShadow::ProjectionType::Inset) {
		return boundingRect;
	}
	
	// 对于外阴影，需要扩展边界以容纳阴影
	qreal offset = d->_pBlur + d->_pSpread;
	
	// 无论是矩形还是圆形阴影，都需要在所有方向上扩展边界
	return boundingRect.united(rect.translated(0, 0).adjusted(-offset, -offset, offset, offset));
}

void NXBoxShadow::draw(QPainter* painter)
{
	Q_D(NXBoxShadow);
	QPoint pos;
	const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &pos, PadToEffectiveBoundingRect);
	if (pixmap.isNull()) return;

	QTransform restoreTransform = painter->worldTransform();
	painter->setWorldTransform(QTransform());

	if (d->_pCircleShadow) {
		// 圆形阴影绘制
		if (d->_pProjectionType == NXWidgetType::BoxShadow::ProjectionType::Inset) {
			d->_drawCircleInsetShadow(painter, pixmap, pos, d->_pSpread, d->_pBlur);
		}
		else {
			d->_drawCircleOutsetShadow(painter, pixmap, pos, d->_pSpread, d->_pBlur);
		}
	}
	else {
		// 矩形阴影绘制
		// 计算阴影偏移量
		const QPoint offset(
			static_cast<int>(d->_pSpread * qCos(d->_radian)),
			static_cast<int>(d->_pSpread * qSin(d->_radian))
		);

		if (d->_pProjectionType == NXWidgetType::BoxShadow::ProjectionType::Inset) {
			d->_drawInsetShadow(painter, pixmap, pos, offset);
		}
		else {
			d->_drawOutsetShadow(painter, pixmap, pos, offset);
		}
	}

	painter->setWorldTransform(restoreTransform);
	/*
	QImage lightShadow(pixmap.size(), QImage::Format_ARGB32_Premultiplied);
	lightShadow.setDevicePixelRatio(pixmap.devicePixelRatioF());
	lightShadow.fill(0);

	QImage darkShadow(pixmap.size(), QImage::Format_ARGB32_Premultiplied);
	darkShadow.setDevicePixelRatio(pixmap.devicePixelRatioF());
	darkShadow.fill(0);

	QPoint offset;
	offset.setX(d->_pSpread * qCos(d->_radian));
	offset.setY(d->_pSpread * qSin(d->_radian));

	if (d->_pProjectionType == NXWidgetType::BoxShadow::ProjectionType::Inset) {
		QPainter lightShadowPainter(&lightShadow);
		lightShadowPainter.drawPixmap(0, 0, pixmap);
		lightShadowPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		lightShadowPainter.fillRect(lightShadow.rect(), d->_pLightColor);
		lightShadowPainter.end();

		QPainter darkShadowPainter(&darkShadow);
		darkShadowPainter.drawPixmap(0, 0, pixmap);
		darkShadowPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		darkShadowPainter.fillRect(lightShadow.rect(), d->_pDarkColor);
		darkShadowPainter.end();

		// CompositionMode_SourceIn 源图颜色有透明度
		// CompositionMode_Source 源图颜色无透明度
		// CompositionMode_SourceAtop + CompositionMode_Clear 目标图像和源图像进行应用透明度混合

		QPainter shadowPainter(&lightShadow);
		shadowPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		shadowPainter.drawImage(offset, darkShadow);
		//shadowPainter.setCompositionMode(QPainter::CompositionMode_Clear);
		offset.setX(qAbs(offset.x()));
		offset.setY(qAbs(offset.y()));

		QRect rect;
		rect.setWidth(lightShadow.width() / pixmap.devicePixelRatioF() / pixmap.devicePixelRatioF());
		rect.setHeight(lightShadow.height() / pixmap.devicePixelRatioF() / pixmap.devicePixelRatioF());
		rect.adjust(offset.x(), offset.y(), -offset.x(), -offset.y());
		shadowPainter.fillRect(rect, Qt::transparent);
		shadowPainter.end();

		QImage blurred(pixmap.size(), QImage::Format_ARGB32);
		blurred.setDevicePixelRatio(pixmap.devicePixelRatioF());
		blurred.fill(0);
		QPainter blurPainter(&blurred);
		qt_blurImage(&blurPainter, lightShadow, d->_pBlur, false, false);
		blurPainter.end();

		QImage image = pixmap.toImage();
		QPainter lastPainter(&image);
		lastPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		lastPainter.drawImage(blurred.rect(), blurred);
		lastPainter.end();

		painter->drawImage(pos, image);
	}
	else {
		QPainter shadowPainter(&lightShadow);
		shadowPainter.setCompositionMode(QPainter::CompositionMode_Source);
		shadowPainter.drawPixmap(0, 0, pixmap);
		shadowPainter.end();

		QImage blurred(lightShadow.size(), QImage::Format_ARGB32_Premultiplied);
		blurred.setDevicePixelRatio(pixmap.devicePixelRatioF());
		blurred.fill(0);

		QPainter blurPainter(&blurred);
		qt_blurImage(&blurPainter, lightShadow, d->_pBlur, false, true);
		blurPainter.end();

		lightShadow = std::move(blurred);

		// blacken the image...
		shadowPainter.begin(&lightShadow);
		shadowPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		shadowPainter.fillRect(lightShadow.rect(), d->_pLightColor);
		shadowPainter.end();
		darkShadow = lightShadow;

		// blacken the image...
		shadowPainter.begin(&darkShadow);
		shadowPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		shadowPainter.fillRect(darkShadow.rect(), d->_pDarkColor);
		shadowPainter.end();

		// draw the blurred drop shadow...
		painter->drawImage(pos + offset, lightShadow);
		painter->drawImage(pos - offset, darkShadow);

		// Draw the actual pixmap...
		painter->drawPixmap(pos, pixmap);
	}
	painter->setWorldTransform(restoreTransform);*/
}
