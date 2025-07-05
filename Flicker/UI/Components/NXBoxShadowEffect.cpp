#include "NXBoxShadowEffect.h"

#include <QPainter>
#include <QPainterPath>
#include "NXTheme.h"
#pragma region qmemrotate
/*
// qt source code, from qmemrotate.cpp & qpixmapfilter.cpp
static const int tileSize = 32;

template <class T>
static inline void qt_memrotate270_tiled_unpacked(const T* src, int w, int h, int sstride, T* dest, int dstride) {
	const int numTilesX = (w + tileSize - 1) / tileSize;
	const int numTilesY = (h + tileSize - 1) / tileSize;

	for (int tx = 0; tx < numTilesX; ++tx) {
		const int startx = tx * tileSize;
		const int stopx = qMin(startx + tileSize, w);

		for (int ty = 0; ty < numTilesY; ++ty) {
			const int starty = h - 1 - ty * tileSize;
			const int stopy = qMax(starty - tileSize, 0);

			for (int x = startx; x < stopx; ++x) {
				T* d = (T*)((char*)dest + x * dstride) + h - 1 - starty;
				const char* s = (const char*)(src + x) + starty * sstride;
				for (int y = starty; y >= stopy; --y) {
					*d++ = *(const T*)s;
					s -= sstride;
				}
			}
		}
	}
}

template <class T>
static inline void qt_memrotate270_template(const T* src, int srcWidth, int srcHeight, int srcStride, T* dest, int dstStride) {
	//#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	//    // packed algorithm assumes little endian and that sizeof(quint32)/sizeof(T) is an integer
	//    if (sizeof(quint32) % sizeof(T) == 0)
	//        qt_memrotate270_tiled<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
	//    else
	//#endif
	qt_memrotate270_tiled_unpacked<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
}

template <>
inline void qt_memrotate270_template<quint32>(const quint32* src, int w, int h, int sstride, quint32* dest, int dstride) {
	// packed algorithm doesn't have any benefit for quint32
	qt_memrotate270_tiled_unpacked(src, w, h, sstride, dest, dstride);
}

template <>
inline void qt_memrotate270_template<quint64>(const quint64* src, int w, int h, int sstride, quint64* dest, int dstride) {
	qt_memrotate270_tiled_unpacked(src, w, h, sstride, dest, dstride);
}

template <class T>
static inline void qt_memrotate90_tiled_unpacked(const T* src, int w, int h, int sstride, T* dest, int dstride) {
	const int numTilesX = (w + tileSize - 1) / tileSize;
	const int numTilesY = (h + tileSize - 1) / tileSize;

	for (int tx = 0; tx < numTilesX; ++tx) {
		const int startx = w - tx * tileSize - 1;
		const int stopx = qMax(startx - tileSize, 0);

		for (int ty = 0; ty < numTilesY; ++ty) {
			const int starty = ty * tileSize;
			const int stopy = qMin(starty + tileSize, h);

			for (int x = startx; x >= stopx; --x) {
				T* d = (T*)((char*)dest + (w - x - 1) * dstride) + starty;
				const char* s = (const char*)(src + x) + starty * sstride;
				for (int y = starty; y < stopy; ++y) {
					*d++ = *(const T*)(s);
					s += sstride;
				}
			}
		}
	}
}

template <class T>
static inline void qt_memrotate90_template(const T* src, int srcWidth, int srcHeight, int srcStride, T* dest, int dstStride) {
	//#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	//    // packed algorithm assumes little endian and that sizeof(quint32)/sizeof(T) is an integer
	//    if (sizeof(quint32) % sizeof(T) == 0)
	//        qt_memrotate90_tiled<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
	//    else
	//#endif
	qt_memrotate90_tiled_unpacked<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
}

template <>
inline void qt_memrotate90_template<quint32>(const quint32* src, int w, int h, int sstride, quint32* dest, int dstride) {
	// packed algorithm doesn't have any benefit for quint32
	qt_memrotate90_tiled_unpacked(src, w, h, sstride, dest, dstride);
}

template <>
inline void qt_memrotate90_template<quint64>(const quint64* src, int w, int h, int sstride, quint64* dest, int dstride) {
	qt_memrotate90_tiled_unpacked(src, w, h, sstride, dest, dstride);
}

template <int shift>
inline int qt_static_shift(int value) {
	if (shift == 0)
		return value;
	else if (shift > 0)
		return value << (uint(shift) & 0x1f);
	else
		return value >> (uint(-shift) & 0x1f);
}

template <int aprec, int zprec>
inline void qt_blurinner(uchar* bptr, int& zR, int& zG, int& zB, int& zA, int alpha) {
	QRgb* pixel = (QRgb*)bptr;

#define Z_MASK (0xff << zprec)
	const int A_zprec = qt_static_shift<zprec - 24>(*pixel) & Z_MASK;
	const int R_zprec = qt_static_shift<zprec - 16>(*pixel) & Z_MASK;
	const int G_zprec = qt_static_shift<zprec - 8>(*pixel) & Z_MASK;
	const int B_zprec = qt_static_shift<zprec>(*pixel) & Z_MASK;
#undef Z_MASK

	const int zR_zprec = zR >> aprec;
	const int zG_zprec = zG >> aprec;
	const int zB_zprec = zB >> aprec;
	const int zA_zprec = zA >> aprec;

	zR += alpha * (R_zprec - zR_zprec);
	zG += alpha * (G_zprec - zG_zprec);
	zB += alpha * (B_zprec - zB_zprec);
	zA += alpha * (A_zprec - zA_zprec);

#define ZA_MASK (0xff << (zprec + aprec))
	* pixel = qt_static_shift<24 - zprec - aprec>(zA & ZA_MASK) | qt_static_shift<16 - zprec - aprec>(zR & ZA_MASK)
		| qt_static_shift<8 - zprec - aprec>(zG & ZA_MASK) | qt_static_shift<-zprec - aprec>(zB & ZA_MASK);
#undef ZA_MASK
}

const int alphaIndex = (QSysInfo::ByteOrder == QSysInfo::BigEndian ? 0 : 3);

template <int aprec, int zprec>
inline void qt_blurinner_alphaOnly(uchar* bptr, int& z, int alpha) {
	const int A_zprec = int(*(bptr)) << zprec;
	const int z_zprec = z >> aprec;
	z += alpha * (A_zprec - z_zprec);
	*(bptr) = z >> (zprec + aprec);
}

template <int aprec, int zprec, bool alphaOnly>
inline void qt_blurrow(QImage& im, int line, int alpha) {
	uchar* bptr = im.scanLine(line);

	int zR = 0, zG = 0, zB = 0, zA = 0;

	if (alphaOnly && im.format() != QImage::Format_Indexed8)
		bptr += alphaIndex;

	const int stride = im.depth() >> 3;
	const int im_width = im.width();
	for (int index = 0; index < im_width; ++index) {
		if (alphaOnly)
			qt_blurinner_alphaOnly<aprec, zprec>(bptr, zA, alpha);
		else
			qt_blurinner<aprec, zprec>(bptr, zR, zG, zB, zA, alpha);
		bptr += stride;
	}

	bptr -= stride;

	for (int index = im_width - 2; index >= 0; --index) {
		bptr -= stride;
		if (alphaOnly)
			qt_blurinner_alphaOnly<aprec, zprec>(bptr, zA, alpha);
		else
			qt_blurinner<aprec, zprec>(bptr, zR, zG, zB, zA, alpha);
	}
}

template <int aprec, int zprec, bool alphaOnly>
void expblur(QImage& img, qreal radius, bool improvedQuality = false, int transposed = 0) {
	// halve the radius if we're using two passes
	if (improvedQuality)
		radius *= qreal(0.5);

	Q_ASSERT(img.format() == QImage::Format_ARGB32_Premultiplied || img.format() == QImage::Format_RGB32
		|| img.format() == QImage::Format_Indexed8 || img.format() == QImage::Format_Grayscale8);

	// choose the alpha such that pixels at radius distance from a fully
	// saturated pixel will have an alpha component of no greater than
	// the cutOffIntensity
	const qreal cutOffIntensity = 2;
	int alpha =
		radius <= qreal(1e-5) ? ((1 << aprec) - 1) : qRound((1 << aprec) * (1 - qPow(cutOffIntensity * (1 / qreal(255)), 1 / radius)));

	int img_height = img.height();
	for (int row = 0; row < img_height; ++row) {
		for (int i = 0; i <= int(improvedQuality); ++i)
			qt_blurrow<aprec, zprec, alphaOnly>(img, row, alpha);
	}

	QImage temp(img.height(), img.width(), img.format());
	temp.setDevicePixelRatio(img.devicePixelRatioF());
	if (transposed >= 0) {
		if (img.depth() == 8) {
			qt_memrotate270_template(reinterpret_cast<const quint8*>(img.bits()),
				img.width(),
				img.height(),
				img.bytesPerLine(),
				reinterpret_cast<quint8*>(temp.bits()),
				temp.bytesPerLine());
		}
		else {
			qt_memrotate270_template(reinterpret_cast<const quint32*>(img.bits()),
				img.width(),
				img.height(),
				img.bytesPerLine(),
				reinterpret_cast<quint32*>(temp.bits()),
				temp.bytesPerLine());
		}
	}
	else {
		if (img.depth() == 8) {
			qt_memrotate90_template(reinterpret_cast<const quint8*>(img.bits()),
				img.width(),
				img.height(),
				img.bytesPerLine(),
				reinterpret_cast<quint8*>(temp.bits()),
				temp.bytesPerLine());
		}
		else {
			qt_memrotate90_template(reinterpret_cast<const quint32*>(img.bits()),
				img.width(),
				img.height(),
				img.bytesPerLine(),
				reinterpret_cast<quint32*>(temp.bits()),
				temp.bytesPerLine());
		}
	}

	img_height = temp.height();
	for (int row = 0; row < img_height; ++row) {
		for (int i = 0; i <= int(improvedQuality); ++i)
			qt_blurrow<aprec, zprec, alphaOnly>(temp, row, alpha);
	}

	if (transposed == 0) {
		if (img.depth() == 8) {
			qt_memrotate90_template(reinterpret_cast<const quint8*>(temp.bits()),
				temp.width(),
				temp.height(),
				temp.bytesPerLine(),
				reinterpret_cast<quint8*>(img.bits()),
				img.bytesPerLine());
		}
		else {
			qt_memrotate90_template(reinterpret_cast<const quint32*>(temp.bits()),
				temp.width(),
				temp.height(),
				temp.bytesPerLine(),
				reinterpret_cast<quint32*>(img.bits()),
				img.bytesPerLine());
		}
	}
	else {
		img = temp;
	}
}

#define AVG(a, b) (((((a) ^ (b)) & 0xfefefefeUL) >> 1) + ((a) & (b)))
#define AVG16(a, b) (((((a) ^ (b)) & 0xf7deUL) >> 1) + ((a) & (b)))

static QImage qt_halfScaled(const QImage& source) {
	if (source.width() < 2 || source.height() < 2)
		return QImage();

	QImage srcImage = source;

	if (source.format() == QImage::Format_Indexed8 || source.format() == QImage::Format_Grayscale8) {
		// assumes grayscale
		QImage dest(source.width() / 2, source.height() / 2, srcImage.format());
		dest.setDevicePixelRatio(source.devicePixelRatioF());

		const uchar* src = reinterpret_cast<const uchar*>(const_cast<const QImage&>(srcImage).bits());
		int sx = srcImage.bytesPerLine();
		int sx2 = sx << 1;

		uchar* dst = reinterpret_cast<uchar*>(dest.bits());
		int dx = dest.bytesPerLine();
		int ww = dest.width();
		int hh = dest.height();

		for (int y = hh; y; --y, dst += dx, src += sx2) {
			const uchar* p1 = src;
			const uchar* p2 = src + sx;
			uchar* q = dst;
			for (int x = ww; x; --x, ++q, p1 += 2, p2 += 2)
				*q = ((int(p1[0]) + int(p1[1]) + int(p2[0]) + int(p2[1])) + 2) >> 2;
		}

		return dest;
	}
	else if (source.format() == QImage::Format_ARGB8565_Premultiplied) {
		QImage dest(source.width() / 2, source.height() / 2, srcImage.format());
		dest.setDevicePixelRatio(source.devicePixelRatioF());

		const uchar* src = reinterpret_cast<const uchar*>(const_cast<const QImage&>(srcImage).bits());
		int sx = srcImage.bytesPerLine();
		int sx2 = sx << 1;

		uchar* dst = reinterpret_cast<uchar*>(dest.bits());
		int dx = dest.bytesPerLine();
		int ww = dest.width();
		int hh = dest.height();

		for (int y = hh; y; --y, dst += dx, src += sx2) {
			const uchar* p1 = src;
			const uchar* p2 = src + sx;
			uchar* q = dst;
			for (int x = ww; x; --x, q += 3, p1 += 6, p2 += 6) {
				// alpha
				q[0] = AVG(AVG(p1[0], p1[3]), AVG(p2[0], p2[3]));
				// rgb
				const quint16 p16_1 = (p1[2] << 8) | p1[1];
				const quint16 p16_2 = (p1[5] << 8) | p1[4];
				const quint16 p16_3 = (p2[2] << 8) | p2[1];
				const quint16 p16_4 = (p2[5] << 8) | p2[4];
				const quint16 result = AVG16(AVG16(p16_1, p16_2), AVG16(p16_3, p16_4));
				q[1] = result & 0xff;
				q[2] = result >> 8;
			}
		}

		return dest;
	}
	else if (source.format() != QImage::Format_ARGB32_Premultiplied && source.format() != QImage::Format_RGB32) {
		srcImage = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	}

	QImage dest(source.width() / 2, source.height() / 2, srcImage.format());
	dest.setDevicePixelRatio(source.devicePixelRatioF());

	const quint32* src = reinterpret_cast<const quint32*>(const_cast<const QImage&>(srcImage).bits());
	int sx = srcImage.bytesPerLine() >> 2;
	int sx2 = sx << 1;

	quint32* dst = reinterpret_cast<quint32*>(dest.bits());
	int dx = dest.bytesPerLine() >> 2;
	int ww = dest.width();
	int hh = dest.height();

	for (int y = hh; y; --y, dst += dx, src += sx2) {
		const quint32* p1 = src;
		const quint32* p2 = src + sx;
		quint32* q = dst;
		for (int x = ww; x; --x, q++, p1 += 2, p2 += 2)
			*q = AVG(AVG(p1[0], p1[1]), AVG(p2[0], p2[1]));
	}

	return dest;
}
*/
//static void qt_blurImage(/*QPainter *p, */ QImage& blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0) {
//	if (blurImage.format() != QImage::Format_ARGB32_Premultiplied && blurImage.format() != QImage::Format_RGB32) {
//		blurImage = blurImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
//	}
//
//	qreal scale = 1;
//	if (radius >= 4 && blurImage.width() >= 2 && blurImage.height() >= 2) {
//		blurImage = qt_halfScaled(blurImage);
//		scale = 2;
//		radius *= qreal(0.5);
//	}
//
//	if (alphaOnly)
//		expblur<12, 10, true>(blurImage, radius, quality, transposed);
//	else
//		expblur<12, 10, false>(blurImage, radius, quality, transposed);
//
//	//if (p) {
//	//    p->scale(scale, scale);
//	//    p->setRenderHint(QPainter::SmoothPixmapTransform);
//	//    p->drawImage(QRect(QPoint(0, 0), blurImage.size() / blurImage.devicePixelRatioF()), blurImage);
//	//}
//}
#pragma endregion qmemrotate

QT_BEGIN_NAMESPACE
extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter* p, QImage& blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

NXBoxShadowEffectPrivate::NXBoxShadowEffectPrivate(QObject* parent)
	: QObject{ parent }
{
}
NXBoxShadowEffectPrivate::~NXBoxShadowEffectPrivate()
{

}

void NXBoxShadowEffectPrivate::_drawInsetShadow(QPainter* painter, const QPixmap& pixmap, const QPoint& pos)
{
	QImage lightShadowImg = _createBaseShadowImage(pixmap, _pLightColor);
	QImage darkShadowImg = _createBaseShadowImage(pixmap, _pDarkColor);
	QSizeF imgSize = pixmap.size() / pixmap.devicePixelRatioF();
	QRectF innerRect(QPointF(0, 0), imgSize);
	QPointF offset;
	switch (_pRotateMode)
	{
	case NXWidgetType::BoxShadow::Rotate45:
		innerRect.adjust(qAbs(_pLightOffset.x()), qAbs(_pLightOffset.y()), -qAbs(_pDarkOffset.x()), -qAbs(_pDarkOffset.y()));
		offset = _pLightOffset;
		break;
	case NXWidgetType::BoxShadow::Rotate135:
		innerRect.adjust(qAbs(_pDarkOffset.y()), qAbs(_pLightOffset.x()), -qAbs(_pLightOffset.y()), -qAbs(_pDarkOffset.x()));
		offset = { -_pLightOffset.y(), _pLightOffset.x() };
		break;
	case NXWidgetType::BoxShadow::Rotate225:
		innerRect.adjust(qAbs(_pDarkOffset.x()), qAbs(_pDarkOffset.y()), -qAbs(_pLightOffset.x()), -qAbs(_pLightOffset.y()));
		offset = { -_pLightOffset.x(), -_pLightOffset.y() };
		break;
	case NXWidgetType::BoxShadow::Rotate315:
		innerRect.adjust(qAbs(_pLightOffset.y()), qAbs(_pDarkOffset.x()), -qAbs(_pDarkOffset.y()), -qAbs(_pLightOffset.x()));
		offset = { _pLightOffset.y(), _pLightOffset.x() };
		break;
	default:
		break;
	}
	// 组合两种阴影
	{
		QPainter shadowPainter(&lightShadowImg);
		shadowPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		shadowPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
		shadowPainter.setPen(Qt::NoPen);
		shadowPainter.drawImage(offset, darkShadowImg);
		shadowPainter.end();
	}
	// 重叠区域清除
	{
		QPainter clearPainter(&lightShadowImg);
		clearPainter.setCompositionMode(QPainter::CompositionMode_Clear);
		clearPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
		clearPainter.setPen(Qt::NoPen);
		clearPainter.fillRect(innerRect, Qt::transparent);
		clearPainter.end();
	}

	// 应用模糊效果
	QImage blurred = _applyBlur(lightShadowImg, false);

	// 组合最终图像
	QImage resultImage = pixmap.toImage();
	{
		QPainter resultPainter(&resultImage);
		resultPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		resultPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
		resultPainter.setPen(Qt::NoPen);
		resultPainter.drawImage(blurred.rect(), blurred);
		resultPainter.end();
	}
	painter->drawImage(pos, resultImage);
}

void NXBoxShadowEffectPrivate::_drawOutsetShadow(QPainter* painter, const QPixmap& pixmap, const QPoint& pos)
{
	// 创建基础阴影图像
	QImage baseShadow = _createBaseShadowImage(pixmap, _pLightColor);

	QImage blurred = _applyBlur(baseShadow, true);

	QPointF lightOffset, darkOffset;
	switch (_pRotateMode)
	{
	case NXWidgetType::BoxShadow::Rotate45:
		lightOffset = _pLightOffset;
		darkOffset = _pDarkOffset;
		break;
	case NXWidgetType::BoxShadow::Rotate135:
		lightOffset = { -_pLightOffset.y(), _pLightOffset.x() };
		darkOffset = { -_pDarkOffset.y(), _pDarkOffset.x() };
		break;
	case NXWidgetType::BoxShadow::Rotate225:
		lightOffset = { -_pLightOffset.x(), -_pLightOffset.y() };
		darkOffset = { -_pDarkOffset.x(), -_pDarkOffset.y() };
		break;
	case NXWidgetType::BoxShadow::Rotate315:
		lightOffset = { _pLightOffset.y(), _pLightOffset.x() };
		darkOffset = { _pDarkOffset.y(), _pDarkOffset.x() };
		break;
	default:
		break;
	}

	QImage lightShadow = _applyColor(blurred, _pLightColor);
	QImage darkShadow = _applyColor(blurred, _pDarkColor);

	painter->drawImage(pos + lightOffset, lightShadow);
	painter->drawImage(pos + darkOffset, darkShadow);

	painter->drawPixmap(pos, pixmap);
}

QImage NXBoxShadowEffectPrivate::_createBaseShadowImage(const QPixmap& pixmap, const QColor& color)
{
	QImage image(pixmap.size(), QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(pixmap.devicePixelRatioF());
	image.fill(0);

	QPainter painter(&image);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	painter.setPen(Qt::NoPen);
	painter.drawPixmap(0, 0, pixmap);

	if(_pProjectionType == NXWidgetType::BoxShadow::ProjectionType::Inset)
		painter.fillRect(image.rect(), color);

	painter.end();
	return image;
}

QImage NXBoxShadowEffectPrivate::_applyBlur(const QImage& source, bool alphaOnly)
{
	QImage blurred(source.size(), QImage::Format_ARGB32_Premultiplied);
	blurred.setDevicePixelRatio(source.devicePixelRatioF());
	blurred.fill(0);

	QPainter painter(&blurred);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	painter.setPen(Qt::NoPen);
	qt_blurImage(&painter, const_cast<QImage&>(source), _pBlur, true, alphaOnly);
	painter.end();

	return blurred;
}

QImage NXBoxShadowEffectPrivate::_applyColor(const QImage& image, const QColor& color)
{
	QImage result = image;
	QPainter painter(&result);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	painter.setPen(Qt::NoPen);
	painter.fillRect(result.rect(), color);
	painter.end();
	return result;
}

NXBoxShadowEffect::NXBoxShadowEffect(QObject* parent /*= nullptr*/)
	: QGraphicsEffect{ parent }, d_ptr(new NXBoxShadowEffectPrivate())
{
	Q_D(NXBoxShadowEffect);
	d->q_ptr = this;
	d->_pBlur = 12.0;
	d->_pSpread = 8.0;
	d->_pLightColor = NXThemeColor(d->_themeMode, BasicBaseAlpha);
	d->_pDarkColor = NXThemeColor(d->_themeMode, BasicBaseAlpha);
	d->_pRotateMode = NXWidgetType::BoxShadow::RotateMode::Rotate45;
	d->_pProjectionType = NXWidgetType::BoxShadow::ProjectionType::Inset;
	d->_pLightOffset = QPointF{ 0.0,0.0 };
	d->_pDarkOffset = QPointF{ 0.0,0.0 };
}

NXBoxShadowEffect::~NXBoxShadowEffect()
{

}

void NXBoxShadowEffect::setDarkOffset(const QPointF& size) {
	Q_D(NXBoxShadowEffect);
	d->_pDarkOffset = size;
	update();
}

QPointF NXBoxShadowEffect::getDarkOffset() const {
	Q_D(const NXBoxShadowEffect);
	return d->_pDarkOffset;
}

void NXBoxShadowEffect::setLightOffset(const QPointF& size) {
	Q_D(NXBoxShadowEffect);
	d->_pLightOffset = size;
	update();
}

QPointF NXBoxShadowEffect::getLightOffset() const {
	Q_D(const NXBoxShadowEffect);
	return d->_pLightOffset;
}

void NXBoxShadowEffect::setRotateMode(const NXWidgetType::BoxShadow::RotateMode& mode) {
	Q_D(NXBoxShadowEffect);
	d->_pRotateMode = mode;
	update();
}

NXWidgetType::BoxShadow::RotateMode NXBoxShadowEffect::getRotateMode() const {
	Q_D(const NXBoxShadowEffect);
	return d->_pRotateMode;
}

void NXBoxShadowEffect::setProjectionType(const NXWidgetType::BoxShadow::ProjectionType& type) {
	Q_D(NXBoxShadowEffect);
	d->_pProjectionType = type;
	update();
}

NXWidgetType::BoxShadow::ProjectionType NXBoxShadowEffect::getProjectionType() const {
	Q_D(const NXBoxShadowEffect);
	return d->_pProjectionType;
}

void NXBoxShadowEffect::setBlur(const qreal& blur) {
	Q_D(NXBoxShadowEffect);
	d->_pBlur = blur;
	update();
}

qreal NXBoxShadowEffect::getBlur() const {
	Q_D(const NXBoxShadowEffect);
	return d->_pBlur;
}

void NXBoxShadowEffect::setSpread(const qreal& spread) {
	Q_D(NXBoxShadowEffect);
	d->_pSpread = spread;
	update();
}

qreal NXBoxShadowEffect::getSpread() const {
	Q_D(const NXBoxShadowEffect);
	return d->_pSpread;
}

void NXBoxShadowEffect::setLightColor(const QColor& color) {
	Q_D(NXBoxShadowEffect);
	d->_pLightColor = color;
	update();
}

QColor NXBoxShadowEffect::getLightColor() const {
	Q_D(const NXBoxShadowEffect);
	return d->_pLightColor;
}

void NXBoxShadowEffect::setDarkColor(const QColor& color) {
	Q_D(NXBoxShadowEffect);
	d->_pDarkColor = color;
	update();
}

QColor NXBoxShadowEffect::getDarkColor() const {
	Q_D(const NXBoxShadowEffect);
	return d->_pDarkColor;
}

QRectF NXBoxShadowEffect::boundingRectFor(const QRectF& rect) const
{
	Q_D(const NXBoxShadowEffect);
	QRectF boundingRect = rect.united(rect.translated(0, 0));
	
	// 对于内阴影，不需要扩展边界
	if (d->_pProjectionType == NXWidgetType::BoxShadow::ProjectionType::Inset) {
		return boundingRect;
	}
	
	// 对于外阴影，需要扩展边界以容纳阴影
	qreal boundary = d->_pBlur + d->_pSpread;
	
	// 无论是矩形还是圆形阴影，都需要在所有方向上扩展边界
	return boundingRect.united(rect.translated(0, 0).adjusted(-boundary, -boundary, boundary, boundary));
}

void NXBoxShadowEffect::draw(QPainter* painter)
{
	Q_D(NXBoxShadowEffect);
	QPoint pos;
	const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &pos, PadToEffectiveBoundingRect);
	if (pixmap.isNull()) return;
	QTransform restoreTransform = painter->worldTransform();
	painter->setWorldTransform(QTransform());
	painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

	if (d->_pProjectionType == NXWidgetType::BoxShadow::ProjectionType::Inset) {
		d->_drawInsetShadow(painter, pixmap, pos);
	}
	else {
		d->_drawOutsetShadow(painter, pixmap, pos);
	}

	painter->setWorldTransform(restoreTransform);
}
