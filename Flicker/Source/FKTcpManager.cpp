#include "FKTcpManager.h"

SINGLETON_CREATE_SHARED_CPP(FKTcpManager)
FKTcpManager::FKTcpManager(QObject* parent)
    : QObject(parent)
{
}

FKTcpManager::~FKTcpManager()
{
}
