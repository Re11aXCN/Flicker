/*************************************************************************************
 *
 * @ Filename     : FKHttpManager.cpp
 * @ Description : 
 * 
 * @ Version     : V1.0
 * @ Author         : Re11a
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

#include "Common/utils/utils.h"
#include "Common/logger/logger_defend.h"
SINGLETON_CREATE_SHARED_CPP(FKHttpManager)
FKHttpManager::FKHttpManager(QObject* parent /*= nullptr*/)
    : QObject(parent)
{

}

void FKHttpManager::sendHttpRequest(flicker::http::service serviceType, const QString& url, const QJsonObject& json)
{
    QByteArray data = QJsonDocument(json).toJson();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));

    QNetworkReply* reply = _pNetworkAccessManager.post(request, data);

    QObject::connect(reply, &QNetworkReply::finished, [=, self = shared_from_this()]() {
        /*
        // Qt的reply->error() 和 flicker::http::status 不一致，统一采用标准的flicker::http::status,
        // 这里不再处理reply->error()
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();

            LOGGER_ERROR("The client has no network!");
            QJsonObject errorObj;
            errorObj.insert("response_status_code", reply->error());
            errorObj.insert("message", utils::qconcat("Client: ", reply->errorString()));
            Q_EMIT this->httpRequestFinished(serviceType, errorObj);
            return;
        }
        */
        QByteArray responseData = reply->readAll();
        reply->deleteLater();

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);

        // 服务器response的 json格式错误
        if (parseError.error != QJsonParseError::NoError) {
            QJsonObject errorObj;
            // cast uint16_t向上转换int（隐式转换）  ok，uint32_t向下转换int（可能丢失数据）  error
            errorObj.insert("response_status_code", static_cast<int>(flicker::http::status::internal_server_error));
            errorObj.insert("message", utils::qconcat("Server: ", QString::fromUtf8(magic_enum::enum_name(flicker::http::status::internal_server_error))));
            Q_EMIT this->httpRequestFinished(serviceType, errorObj);
            return;
        }
        //switch (flicker::http::status::conflict == static_cast<flicker::http::status>(responseObj["response_status_code"].toInt())) {}

        Q_EMIT this->httpRequestFinished(serviceType, jsonDoc.object());
        });
}
