#ifndef FK_CONFIG_H_
#define FK_CONFIG_H_

#include <chrono>
#include <string>

namespace Flicker::Server::Config {
    struct BaseServer {
        std::string Host;
        uint16_t Port;
        bool UseSSL;
        std::string getEndPoint() const {
            return std::format("{}:{}", Host, Port);
        }
    };
    struct GateServer : public BaseServer {
        GateServer() : BaseServer{ .Host{"127.0.0.1"}, .Port{9527}, .UseSSL{false} } {}
    };

    struct StatusServer : public BaseServer {
        StatusServer() : BaseServer{ .Host{"0.0.0.0"}, .Port{9528}, .UseSSL{false} } {}
    };

    struct ChatMasterServer : public BaseServer {
        std::string ID{"ChatMasterServer"};
        ChatMasterServer() : BaseServer{ .Host{"127.0.0.1"}, .Port{9529}, .UseSSL{false} } {}
    };

    struct ChatSlaveServer : public BaseServer {
        std::string ID{"ChatSlaveServer"};
        ChatSlaveServer() : BaseServer{ .Host{"127.0.0.1"}, .Port{9530}, .UseSSL{false} } {}
    };

    struct BaseGrpcService {
        std::string Host;
        uint16_t Port;
        uint16_t PoolSize;
        bool UseSSL;
        bool KeepAlivePermitWithoutCalls;
        bool Http2MaxPingWithoutData;
        std::chrono::milliseconds KeepAliveTime;
        std::chrono::milliseconds KeepAliveTimeout;
        std::chrono::milliseconds MaxReconnectBackoff;
        std::chrono::milliseconds GrpclbCallTimeout;
        std::string getEndPoint() const {
            return std::format("{}:{}", Host, Port);
        }
    };

    struct AuthenticateLoginGrpcService : public BaseGrpcService {
        AuthenticateLoginGrpcService()
            : BaseGrpcService
            {
                .Host{"127.0.0.1"}, .Port{50051}, .PoolSize{10},
                .UseSSL{false}, .KeepAlivePermitWithoutCalls{false}, .Http2MaxPingWithoutData{0},
                .KeepAliveTime{std::chrono::milliseconds{30000}}, .KeepAliveTimeout{std::chrono::milliseconds{10000}}, .MaxReconnectBackoff{std::chrono::milliseconds{10000}}, .GrpclbCallTimeout{std::chrono::milliseconds{5000}}
            }
        {
        }
    };
	struct GenerateTokenGrpcService : public BaseGrpcService {
        GenerateTokenGrpcService()
            : BaseGrpcService
            {
                .Host{"127.0.0.1"}, .Port{9528}, .PoolSize{10},
                .UseSSL{false}, .KeepAlivePermitWithoutCalls{false}, .Http2MaxPingWithoutData{0},
                .KeepAliveTime{std::chrono::milliseconds{30000}}, .KeepAliveTimeout{std::chrono::milliseconds{10000}}, .MaxReconnectBackoff{std::chrono::milliseconds{10000}}, .GrpclbCallTimeout{std::chrono::milliseconds{5000}}
            }
        {
        }
    };
	struct ValidateTokenGrpcService : public BaseGrpcService {
        ValidateTokenGrpcService()
            : BaseGrpcService
            {
                .Host{"127.0.0.1"}, .Port{9528}, .PoolSize{10},
                .UseSSL{false}, .KeepAlivePermitWithoutCalls{false}, .Http2MaxPingWithoutData{0},
                .KeepAliveTime{std::chrono::milliseconds{30000}}, .KeepAliveTimeout{std::chrono::milliseconds{10000}}, .MaxReconnectBackoff{std::chrono::milliseconds{10000}}, .GrpclbCallTimeout{std::chrono::milliseconds{5000}}
            }
        {
        }
    };
}


#endif // !FK_CONFIG_H_