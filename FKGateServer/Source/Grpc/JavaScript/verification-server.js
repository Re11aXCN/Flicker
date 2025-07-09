/**
 * 邮箱验证码服务入口文件
 * 提供gRPC服务，处理验证码请求并发送邮件
 */

const grpc = require('@grpc/grpc-js');

const { serviceConfig } = require('./config/config-loader');
const { loadVerifyCodeProtoFile } = require('./utils/proto-loader');
const { getVerificationCode } = require('./services/verification-service');
const verifyCodeProto = loadVerifyCodeProtoFile();

/**
 * 启动gRPC服务器
 */
function startServer() {
    const server = new grpc.Server();
    server.addService(verifyCodeProto.VerifyCodeService.service, { 
        GetVerifyCode: getVerificationCode 
    });

    const { host, port } = serviceConfig.verification;
    const serverAddress = `${host}:${port}`;
    server.bindAsync(
        serverAddress, 
        grpc.ServerCredentials.createInsecure(), 
        (error, port) => {
            if (error) {
                console.error(`服务器绑定失败: ${error.message}`);
                return;
            }

            // 修复：移除 server.start() 调用、新版本中 bindAsync 会自动启动服务器
            console.log(`验证码服务已启动，监听地址: ${serverAddress}`);
        }
    );
}

// 启动服务器
startServer();