/**
 * 密码加密服务
 * 提供gRPC服务，处理密码加密请求
 */

const grpc = require('@grpc/grpc-js');
const bcrypt = require('bcrypt');

const { serviceConfig } = require('../config/config-loader');
const { loadProtoFile } = require('../utils/proto-loader');
const { ERROR_CODE } = require('../utils/constants');
const { Logger } = require('../utils/logger');

/**
 * 处理密码加密请求
 * @param {Object} call - gRPC调用对象，包含请求信息
 * @param {Function} callback - 回调函数，用于返回响应
 */
async function encryptPassword(call, callback) {
    const { rpc_request_type, hashed_password } = call.request;
    Logger.info('收到密码加密请求');
    
    if (!hashed_password) {
        Logger.error('密码参数为空');
        callback(null, {
            rpc_response_code: ERROR_CODE.FORM_PARAMS_MISSING,
            message: '密码参数不能为空',
            encrypted_password: ''
        });
        return;
    }
    
    try {
        // 生成盐值，默认使用10轮加密
        const saltRounds = 10;
        const salt = await bcrypt.genSalt(saltRounds);
        
        // 使用盐值加密密码
        const encryptedPassword = await bcrypt.hash(hashed_password, salt);
        
        Logger.info('密码加密成功');
        
        // 返回加密后的密码
        callback(null, {
            rpc_response_code: ERROR_CODE.SUCCESS,
            message: '密码加密成功',
            encrypted_password: encryptedPassword
        });
    } catch (error) {
        Logger.error(`密码加密失败: ${error.message}`);
        
        callback(null, {
            rpc_response_code: ERROR_CODE.ENCRYPT_ERROR,
            message: `密码加密失败: ${error.message}`,
            encrypted_password: ''
        });
    }
}

/**
 * 启动gRPC服务器
 */
function startServer() {
    const server = new grpc.Server();
    const protoDescriptor = loadProtoFile();
    
    server.addService(protoDescriptor.Encryption.service, {
        EncryptPassword: encryptPassword
    });

    const { host, port } = serviceConfig.encryption;
    const serverAddress = `${host}:${port}`;
    server.bindAsync(
        serverAddress,
        grpc.ServerCredentials.createInsecure(),
        (error, port) => {
            if (error) {
                Logger.error(`服务器绑定失败: ${error.message}`);
                return;
            }

            Logger.info(`密码加密服务已启动，监听地址: ${serverAddress}`);
        }
    );
}

// 启动服务器
startServer();