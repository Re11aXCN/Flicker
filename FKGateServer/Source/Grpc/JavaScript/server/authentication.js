/**
 * 密码验证服务
 * 提供gRPC服务，处理密码验证请求
 */

const grpc = require('@grpc/grpc-js');
const bcrypt = require('bcrypt');

const { serviceConfig } = require('../config/config-loader');
const { loadProtoFile } = require('../utils/proto-loader');
const { ERROR_CODE } = require('../utils/constants');
const { Logger } = require('../utils/logger');

/**
 * 处理密码重置验证请求
 * @param {Object} call - gRPC调用对象，包含请求信息
 * @param {Function} callback - 回调函数，用于返回响应
 */
async function authenticatePwdReset(call, callback) {
    const { rpc_request_type, hashed_password, encrypted_password } = call.request;
    Logger.info('收到密码验证请求');
    
    if (!hashed_password || !encrypted_password) {
        Logger.error('密码参数不完整');
        callback(null, {
            rpc_response_code: ERROR_CODE.FORM_PARAMS_MISSING,
            message: '密码参数不完整',
            is_authenticated: false
        });
        return;
    }
    
    try {
        // 验证密码
        const isValid = await bcrypt.compare(hashed_password, encrypted_password);
        
        Logger.info(`密码验证结果: ${isValid ? '有效' : '无效'}`);
        
        // 返回验证结果
        callback(null, {
            rpc_response_code: ERROR_CODE.SUCCESS,
            message: '密码验证完成',
            is_authenticated: isValid
        });
    } catch (error) {
        Logger.error(`密码验证失败: ${error.message}`);
        
        callback(null, {
            rpc_response_code: ERROR_CODE.CIPHER_AUTH_FAILED,
            message: `密码验证失败: ${error.message}`,
            is_authenticated: false
        });
    }
}

/**
 * 启动gRPC服务器
 */
function startServer() {
    const server = new grpc.Server();
    const protoDescriptor = loadProtoFile();
    
    server.addService(protoDescriptor.Authentication.service, {
        AuthenticatePwdReset: authenticatePwdReset
    });

    const { host, port } = serviceConfig.authentication;
    const serverAddress = `${host}:${port}`;
    server.bindAsync(
        serverAddress,
        grpc.ServerCredentials.createInsecure(),
        (error, port) => {
            if (error) {
                Logger.error(`服务器绑定失败: ${error.message}`);
                return;
            }

            Logger.info(`密码验证服务已启动，监听地址: ${serverAddress}`);
        }
    );
}

// 启动服务器
startServer();