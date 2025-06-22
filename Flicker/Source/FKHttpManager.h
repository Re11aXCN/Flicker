/*************************************************************************************
 *
 * @ Filename	 : FKHttpManager.h
 * @ Description : 
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
 * @ Date Created: 2025/6/17
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef FK_HTTPMANAGER_H_
#define FK_HTTPMANAGER_H_
#include <QNetworkAccessManager>
#include <QJsonObject>
#include "FKDef.h"
#include "FKMacro.h"
class FKHttpManager : public QObject, public std::enable_shared_from_this<FKHttpManager>
{
	Q_OBJECT
	SINGLETON_CREATE_SHARED_H(FKHttpManager)
public:
	~FKHttpManager() = default;
	void postHttpRequest(const QString& url, const QJsonObject& json, Http::RequestId requestId, Http::RequestSeviceType serviceType);

Q_SIGNALS:
	void httpRequestFinished(const QString& response, Http::RequestId requestId, Http::RequestSeviceType serviceType, Http::RequestStatusCode statusCode);
private:
	explicit FKHttpManager(QObject* parent = nullptr);

	QNetworkAccessManager _pNetworkAccessManager;
};

#endif // !FK_HTTPMANAGER_H_
