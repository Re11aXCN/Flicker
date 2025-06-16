#include "FKHttpManager.h"
#include <QJsonDocument>
#include <QNetworkReply>
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
			self->_handleHttpRequestFinished(std::move(QString{}), requestId, serviceType, Http::RequestErrorCode::NETWORK_ERROR);
			reply->deleteLater();
			return;
		}

		self->_handleHttpRequestFinished(std::move(reply->readAll()), requestId, serviceType, Http::RequestErrorCode::SUCCESS);
		reply->deleteLater();
		return;
		});
}


void FKHttpManager::_handleHttpRequestFinished(QString&& response, Http::RequestId requestId, Http::RequestSeviceType serviceType, Http::RequestErrorCode errorCode)
{
	switch (serviceType) {
	case Http::REGISTER_SERVICE:
		Q_EMIT registerServiceFinished(std::move(response), requestId, errorCode);
	default:
		break;
	}
}
