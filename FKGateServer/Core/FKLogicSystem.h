/*************************************************************************************
 *
 * @ Filename     : FKLogicSystem.h
 * @ Description : 
 * 
 * @ Version     : V1.0
 * @ Author         : Re11a
 * @ Date Created: 2025/6/17
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef FK_LOGIC_SYSTEM_H_
#define FK_LOGIC_SYSTEM_H_

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

#include "Flicker/Global/FKDef.h"
#include "universal/mysql/connection_pool.h"

class FKHttpConnection;
class FKLogicSystem {
    SINGLETON_CREATE_SHARED_H(FKLogicSystem)
    using MessageHandler = std::function<void(std::shared_ptr<FKHttpConnection>)>;
public:

    // 外部调用
    bool callBack(const std::string& url, boost::beast::http::verb requestType, std::shared_ptr<FKHttpConnection> connection);
    // 内部注册业务回调
    void registerCallback(const std::string& url, boost::beast::http::verb requestType, MessageHandler handler);
private:
    FKLogicSystem();
    ~FKLogicSystem() = default;

    std::unordered_map<std::string, MessageHandler> _pPostRequestCallBacks;
    std::unordered_map<std::string, MessageHandler> _pGetRequestCallBacks;

    universal::mysql::ConnectionPoolSharedPtr _pFlickerDbPool;
};

#endif // !FK_LOGIC_SYSTEM_H_


