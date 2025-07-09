/**
 * 密码服务入口文件
 * 提供gRPC服务
 * 注册加密密码、登录验证密码
 */

const grpc = require('@grpc/grpc-js');
const { serviceConfig } = require('./config/config-loader');
const { loadPasswordProtoFile } = require('./utils/proto-loader');
const { encryptPassword, authenticatePassword } = require('./services/cipher-service');
const passwordProto = loadPasswordProtoFile();

/**
 * 启动gRPC服务器
 */
function startServer() {
    const server = new grpc.Server();
    server.addService(passwordProto.PasswordService.service, {
        EncryptPassword: encryptPassword,
        AuthenticatePassword: authenticatePassword
    });

    const { host, port } = serviceConfig.cipher;
    const serverAddress = `${host}:${port}`;
    server.bindAsync(
        serverAddress,
        grpc.ServerCredentials.createInsecure(),
        (error, port) => {
            if (error) {
                console.error(`服务器绑定失败: ${error.message}`);
                return;
            }

            console.log(`密码服务已启动，监听地址: ${serverAddress}`);
        }
    );
}

// 启动服务器
startServer();