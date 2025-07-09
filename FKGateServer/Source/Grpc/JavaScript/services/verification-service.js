/**
 * 邮件服务模块
 * 负责处理邮件发送功能
 */

const nodemailer = require('nodemailer');
const { v4: uuidv4 } = require('uuid');
const { mailConfig } = require('../config/config-loader');
const { ERROR_CODE, VERIFICATION } = require('../utils/constants');
const { getRedis, queryRedis, setRedisExpire } = require('../utils/redis');

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
                console.error(`邮件发送失败: ${error.message}`);
                reject(error);
            } else {
                console.log(`邮件发送成功: ${info.response}`);
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
                verify_code: existingCode
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
                verify_code: ""
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
            verify_code: verificationCode
        });
    } catch (error) {
        console.error(`发送验证码邮件时发生错误: ${error.message}`);

        // 返回错误响应 - 注意字段名与proto定义匹配
        callback(null, {
            status_code: ERROR_CODE.EMAIL_SEND_FAILED,
            message: `发送验证码失败: ${error.message}`,
            request_type: request_type || 0,
            email: email,
            verify_code: ""
        });
    }
}


module.exports = {
    getVerificationCode,
};