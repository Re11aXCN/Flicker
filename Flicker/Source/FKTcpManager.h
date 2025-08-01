#ifndef FK_TCP_MANAGER_H_
#define FK_TCP_MANAGER_H_

#include <QTcpSocket>

#include "Common/global/macro.h"
class FKTcpManager : public QObject, public std::enable_shared_from_this<FKTcpManager>
{
    Q_OBJECT
    SINGLETON_CREATE_SHARED_H(FKTcpManager)
public:
    ~FKTcpManager();

private:
    explicit FKTcpManager(QObject* parent = nullptr);
};

#endif // !FK_TCP_MANAGER_H_

