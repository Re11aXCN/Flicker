/**
 * 邮箱验证码服务
 * 提供gRPC服务，处理验证码请求并发送邮件
 */

const grpc = require('@grpc/grpc-js');
const nodemailer = require('nodemailer');
const { v4: uuidv4 } = require('uuid');

const { mailConfig, serviceConfig } = require('../config/config-loader');
const { loadProtoFile } = require('../utils/proto-loader');
const { ERROR_CODE, VERIFICATION } = require('../utils/constants');
const { getRedis, setRedisExpire } = require('../utils/redis');
const { Logger } = require('../utils/logger');

/**
 * 创建邮件传输对象
 * 配置SMTP服务器连接信息
 */
const transporter = nodemailer.createTransport({
    host: mailConfig.smtp.host,      // SMTP服务器地址
    port: mailConfig.smtp.port,      // SMTP端口
    secure: mailConfig.smtp.secure,  // 启用SSL/TLS
    auth: {
        user: mailConfig.account.user,  // 邮箱账号
        pass: mailConfig.account.pass   // 邮箱密码/授权码
    }
});

/**
 * 发送邮件
 * @param {Object} mailOptions - 邮件选项
 * @param {string} mailOptions.from - 发件人地址
 * @param {string} mailOptions.to - 收件人地址
 * @param {string} mailOptions.subject - 邮件主题
 * @param {string} [mailOptions.text] - 纯文本内容
 * @param {string} [mailOptions.html] - HTML内容
 * @returns {Promise<string>} 发送结果
 */
async function sendMail(mailOptions) {
    return new Promise((resolve, reject) => {
        transporter.sendMail(mailOptions, (error, info) => {
            if (error) {
                Logger.error(`邮件发送失败: ${error.message}`);
                reject(error);
            } else {
                Logger.info(`邮件发送成功: ${info.response}`);
                resolve(info.response);
            }
        });
    });
}

/**
 * 发送验证码邮件
 * @param {string} email - 收件人邮箱
 * @param {string} code - 验证码
 * @returns {Promise<string>} 发送结果
 */
async function sendVerificationEmail(email, code) {
    const mailOptions = {
        from: mailConfig.account.user,
        to: email,
        subject: 'Flicker应用验证码',
        text: `您的验证码为 ${code}，请在五分钟内完成注册。`,
        html: `<div style="font-family: Arial, sans-serif; color: #333;">
                <h2 style="color: #4a86e8;">Flicker应用验证码</h2>
                <p>您好！</p>
                <p>您的验证码为: <strong style="font-size: 18px; color: #4a86e8;">${code}</strong></p>
                <p>请在五分钟内完成注册。</p>
                <p>如果您没有请求此验证码，请忽略此邮件。</p>
                <p>谢谢！</p>
              </div>`
    };

    return sendMail(mailOptions);
}

/**
 * 处理验证码请求并发送邮件
 * @param {Object} call - gRPC调用对象，包含请求信息
 * @param {Function} callback - 回调函数，用于返回响应
 */
async function getVerifyCode(call, callback) {
    const { email, rpc_request_type } = call.request;
    Logger.info(`收到验证码请求，邮箱: ${email}, 请求类型: ${rpc_request_type}`);

    try {
        // 构建Redis键名
        const redisKey = `${VERIFICATION.CODE_PREFIX}${email}`;

        // 检查是否已存在验证码（可选：如果已存在且未过期，可以重用）
        const existingCode = await getRedis(redisKey);
        if (existingCode) {
            Logger.info(`邮箱 ${email} 已有验证码，重用现有验证码`);

            // 返回成功响应，使用现有验证码
            callback(null, {
                rpc_response_code: ERROR_CODE.SUCCESS,
                message: "验证码发送成功",
                verify_code: existingCode
            });
            return;
        }

        // 生成唯一验证码
        const verificationCode = uuidv4().substring(0, 6).toUpperCase();
        Logger.info(`生成验证码: ${verificationCode} 用于邮箱: ${email}`);

        // 使用emailService发送验证邮件
        await sendVerificationEmail(email, verificationCode);
        Logger.info(`邮件发送成功: ${email}`);

        // 将验证码存储到Redis，设置过期时间为5分钟
        const saveResult = await setRedisExpire(redisKey, verificationCode, VERIFICATION.EXPIRATION);
        if (!saveResult) {
            Logger.error(`存储验证码到Redis失败: ${email}`);
            callback(null, {
                rpc_response_code: ERROR_CODE.REDIS_ERROR,
                message: "验证码存储失败，请稍后重试",
                verify_code: ""
            });
            return;
        }

        Logger.info(`验证码已存储到Redis，过期时间: ${VERIFICATION.EXPIRATION}秒`);

        // 返回成功响应 - 注意字段名与proto定义匹配
        callback(null, {
            rpc_response_code: ERROR_CODE.SUCCESS,
            message: "验证码发送成功",
            verify_code: verificationCode
        });
    } catch (error) {
        Logger.error(`发送验证码邮件时发生错误: ${error.message}`);

        // 返回错误响应 - 注意字段名与proto定义匹配
        callback(null, {
            rpc_response_code: ERROR_CODE.EMAIL_SEND_FAILED,
            message: `发送验证码失败: ${error.message}`,
            verify_code: ""
        });
    }
}

/**
 * 启动gRPC服务器
 */
function startServer() {
    const server = new grpc.Server();
    const protoDescriptor = loadProtoFile();
    
    server.addService(protoDescriptor.Verification.service, { 
        GetVerifyCode: getVerifyCode 
    });

    const { host, port } = serviceConfig.verification;
    const serverAddress = `${host}:${port}`;
    server.bindAsync(
        serverAddress, 
        grpc.ServerCredentials.createInsecure(), 
        (error, port) => {
            if (error) {
                Logger.error(`服务器绑定失败: ${error.message}`);
                return;
            }

            Logger.info(`验证码服务已启动，监听地址: ${serverAddress}`);
        }
    );
}

// 启动服务器
startServer();