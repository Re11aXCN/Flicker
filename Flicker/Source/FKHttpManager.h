#ifndef FKHTTPMANAGER_H
#define FKHTTPMANAGER_H
#include <NXDef.h>
#include <singleton.h>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include "FKDef.h"
class FKHttpManager : public QObject, public std::enable_shared_from_this<FKHttpManager>
{
	Q_OBJECT
	Q_SINGLETON_CREATE(FKHttpManager)
public:

Q_SIGNALS:
	void registerServiceFinished(QString&& resource, Http::RequestId requestId, Http::RequestErrorCode errorCode);
private:
	Q_SLOT void _handleHttpRequestFinished(QString&& response, Http::RequestId requestId, Http::RequestSeviceType serviceType, Http::RequestErrorCode errorCode);
private:
	explicit FKHttpManager(QObject* parent = nullptr);
	~FKHttpManager() = default;
	void _postHttpRequest(const QString& url, const QJsonObject& json, Http::RequestId requestId, Http::RequestSeviceType serviceType);

	QNetworkAccessManager _pNetworkAccessManager;
};

#endif // !FKHTTPMANAGER_H
