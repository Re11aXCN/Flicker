/**
 * 密码加密服务入口文件
 * 提供gRPC服务，处理密码加密和验证请求
 */

const grpc = require('@grpc/grpc-js');
const bcrypt = require('bcrypt');
const path = require('path');
const protoLoader = require('@grpc/proto-loader');
const { servicesConfig } = require('./config/configLoader');

// 加载Proto文件
const PROTO_PATH = path.join(__dirname, '../FKPasswordGrpc.proto');

// 加载Proto文件定义
function loadProtoFile() {
    try {
        const packageDefinition = protoLoader.loadSync(
            PROTO_PATH,
            {
                keepCase: true,
                longs: String,
                enums: String,
                defaults: true,
                oneofs: true
            }
        );

        // 解析Proto包定义
        const protoDescriptor = grpc.loadPackageDefinition(packageDefinition);
        
        // 返回密码服务定义
        return protoDescriptor.FKPasswordGrpc;
    } catch (error) {
        console.error(`加载Proto文件失败: ${error.message}`);
        console.error(`Proto文件路径: ${PROTO_PATH}`);
        if (error.code === 'ENOENT') {
            console.error('错误原因: Proto文件不存在');
        }
        throw new Error(`Proto文件加载失败: ${error.message}`);
    }
}

// 错误码定义
const ERROR_CODE = {
    SUCCESS: 0,
    HASH_ERROR: 1,
    VERIFY_ERROR: 2,
    INVALID_PARAMS: 3
};

/**
 * 处理密码加密请求
 * @param {Object} call - gRPC调用对象，包含请求信息
 * @param {Function} callback - 回调函数，用于返回响应
 */
async function hashPassword(call, callback) {
    const { password } = call.request;
    console.log('收到密码加密请求');
    
    if (!password) {
        console.error('密码参数为空');
        callback(null, {
            status_code: ERROR_CODE.INVALID_PARAMS,
            message: '密码参数不能为空',
            hashed_password: '',
            salt: ''
        });
        return;
    }
    
    try {
        // 生成盐值，默认使用10轮加密
        const saltRounds = 10;
        const salt = await bcrypt.genSalt(saltRounds);
        
        // 使用盐值加密密码
        const hashedPassword = await bcrypt.hash(password, salt);
        
        console.log('密码加密成功');
        
        // 返回加密后的密码和盐值
        callback(null, {
            status_code: ERROR_CODE.SUCCESS,
            message: '密码加密成功',
            hashed_password: hashedPassword,
            salt: salt
        });
    } catch (error) {
        console.error(`密码加密失败: ${error.message}`);
        
        callback(null, {
            status_code: ERROR_CODE.HASH_ERROR,
            message: `密码加密失败: ${error.message}`,
            hashed_password: '',
            salt: ''
        });
    }
}

/**
 * 处理密码验证请求
 * @param {Object} call - gRPC调用对象，包含请求信息
 * @param {Function} callback - 回调函数，用于返回响应
 */
async function verifyPassword(call, callback) {
    const { password, hashed_password } = call.request;
    console.log('收到密码验证请求');
    
    if (!password || !hashed_password) {
        console.error('密码参数不完整');
        callback(null, {
            status_code: ERROR_CODE.INVALID_PARAMS,
            message: '密码参数不完整',
            is_valid: false
        });
        return;
    }
    
    try {
        // 验证密码
        const isValid = await bcrypt.compare(password, hashed_password);
        
        console.log(`密码验证结果: ${isValid ? '有效' : '无效'}`);
        
        // 返回验证结果
        callback(null, {
            status_code: ERROR_CODE.SUCCESS,
            message: '密码验证完成',
            is_valid: isValid
        });
    } catch (error) {
        console.error(`密码验证失败: ${error.message}`);
        
        callback(null, {
            status_code: ERROR_CODE.VERIFY_ERROR,
            message: `密码验证失败: ${error.message}`,
            is_valid: false
        });
    }
}

/**
 * 启动gRPC服务器
 */
function startServer() {
    const passwordProto = loadProtoFile();
    const server = new grpc.Server();
    
    server.addService(passwordProto.PasswordService.service, { 
        HashPassword: hashPassword,
        VerifyPassword: verifyPassword
    });
    
    const { host, port } = servicesConfig.password;
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