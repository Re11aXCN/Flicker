/**
 * 邮箱验证码服务入口文件
 * 提供gRPC服务，处理验证码请求并发送邮件
 */

const grpc = require('@grpc/grpc-js');
const { v4: uuidv4 } = require('uuid');
const verifyCodeProto = require('./utils/protoLoader');
const { ERROR_CODE, VERIFICATION } = require('./utils/constants');
const { sendVerificationEmail } = require('./services/emailService');
const { getRedis, queryRedis, setRedisExpire } = require('./utils/redis');

/**
 * 处理验证码请求并发送邮件
 * @param {Object} call - gRPC调用对象，包含请求信息
 * @param {Function} callback - 回调函数，用于返回响应
 */
async function getVerificationCode(call, callback) {
    const { email, request_type } = call.request;
    console.log(`收到验证码请求，邮箱: ${email}, 请求类型: ${request_type}`);
    
    try {
        // 构建Redis键名
        const redisKey = `${VERIFICATION.CODE_PREFIX}${email}`;
        
        // 检查是否已存在验证码（可选：如果已存在且未过期，可以重用）
        const existingCode = await getRedis(redisKey);
        if (existingCode) {
            console.log(`邮箱 ${email} 已有验证码，重用现有验证码`);
            
            // 返回成功响应，使用现有验证码
            callback(null, {
                status_code: ERROR_CODE.SUCCESS,
                message: "验证码发送成功",
                request_type: request_type,
                email: email,
                varify_code: existingCode
            });
            return;
        }
        
        // 生成唯一验证码
        const verificationCode = uuidv4().substring(0, 6).toUpperCase();
        console.log(`生成验证码: ${verificationCode} 用于邮箱: ${email}`);
        
        // 使用emailService发送验证邮件
        await sendVerificationEmail(email, verificationCode);
        console.log(`邮件发送成功: ${email}`);
        
        // 将验证码存储到Redis，设置过期时间为5分钟
        const saveResult = await setRedisExpire(redisKey, verificationCode, VERIFICATION.EXPIRATION);
        if (!saveResult) {
            console.error(`存储验证码到Redis失败: ${email}`);
            callback(null, {
                status_code: ERROR_CODE.REDIS_ERROR,
                message: "验证码存储失败，请稍后重试",
                request_type: request_type,
                email: email,
                varify_code: ""
            });
            return;
        }
        
        console.log(`验证码已存储到Redis，过期时间: ${VERIFICATION.EXPIRATION}秒`);

        // 返回成功响应 - 注意字段名与proto定义匹配
        callback(null, {
            status_code: ERROR_CODE.SUCCESS,
            message: "验证码发送成功",
            request_type: request_type,
            email: email,
            varify_code: verificationCode
        });
    } catch (error) {
        console.error(`发送验证码邮件时发生错误: ${error.message}`);
        
        // 返回错误响应 - 注意字段名与proto定义匹配
        callback(null, {
            status_code: ERROR_CODE.EMAIL_SEND_FAILED,
            message: `发送验证码失败: ${error.message}`,
            request_type: request_type || 0,
            email: email,
            varify_code: ""
        });
    }
}

/**
 * 启动gRPC服务器
 */
function startServer() {
    const server = new grpc.Server();
    server.addService(verifyCodeProto.VarifyCodeService.service, { 
        GetVarifyCode: getVerificationCode 
    });
    
    const serverAddress = '127.0.0.1:50051';
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