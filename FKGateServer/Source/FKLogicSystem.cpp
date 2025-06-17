/*************************************************************************************
 *
 * @ Filename	 : FKLogicSystem.cpp
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
#include "FKLogicSystem.h"
#include "FKHttpConnection.h"

SINGLETON_CREATE_SHARED_CPP(FKLogicSystem)
FKLogicSystem::FKLogicSystem()
{
	auto getTestFunc = [](std::shared_ptr<FKHttpConnection> connection) {
		boost::beast::ostream(connection->getResponse().body()) << "收到 get_test 请求";
		};
	_pGetRequestCallBacks["/get_test"] = getTestFunc;
}


bool FKLogicSystem::handleGetRequest(const std::string& url, std::shared_ptr<FKHttpConnection> connection)
{
	if(_pGetRequestCallBacks.find(url) == _pGetRequestCallBacks.end()) return false;

	_pGetRequestCallBacks[url](connection);
	return true;
}