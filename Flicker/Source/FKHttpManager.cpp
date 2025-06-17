/*************************************************************************************
 *
 * @ Filename	 : FKHttpManager.cpp
 * @ Description : 
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
 * @ Date Created: 2025/6/16
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#include "FKHttpManager.h"
#include <QJsonDocument>
#include <QNetworkReply>

SINGLETON_CREATE_SHARED_CPP(FKHttpManager)
FKHttpManager::FKHttpManager(QObject* parent /*= nullptr*/)
    : QObject(parent)
{

}

void FKHttpManager::_postHttpRequest(const QString& url, const QJsonObject& json, Http::RequestId requestId, Http::RequestSeviceType serviceType)
{
	QByteArray data = QJsonDocument(json).toJson();
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));

	QNetworkReply* reply = _pNetworkAccessManager.post(request, data);

	QObject::connect(reply, &QNetworkReply::finished, [=, self = shared_from_this()]() {
		if (reply->error() != QNetworkReply::NoError) {
			qDebug() << reply->errorString();
			self->_handleHttpRequestFinished(QString{}, requestId, serviceType, Http::RequestErrorCode::NETWORK_ERROR);
			reply->deleteLater();
			return;
		}
		QString responseData = reply->readAll();
		self->_handleHttpRequestFinished(responseData, requestId, serviceType, Http::RequestErrorCode::SUCCESS);
		reply->deleteLater();
		return;
		});
}


void FKHttpManager::_handleHttpRequestFinished(const QString& response, Http::RequestId requestId, Http::RequestSeviceType serviceType, Http::RequestErrorCode errorCode)
{
	switch (serviceType) {
	case Http::REGISTER_SERVICE:
		Q_EMIT registerServiceFinished(response, requestId, errorCode);
	default:
		break;
	}
}
