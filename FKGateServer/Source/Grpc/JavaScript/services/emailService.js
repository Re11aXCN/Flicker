/**
 * 邮件服务模块
 * 负责处理邮件发送功能
 */

const nodemailer = require('nodemailer');
const { emailConfig } = require('../config/configLoader');

/**
 * 创建邮件传输对象
 * 配置SMTP服务器连接信息
 */
const transporter = nodemailer.createTransport({
    host: emailConfig.smtp.host,
    port: emailConfig.smtp.port,
    secure: emailConfig.smtp.secure, // 使用SSL
    auth: {
        user: emailConfig.user,
        pass: emailConfig.authcode // 授权码，非邮箱密码
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
        from: emailConfig.user,
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

module.exports = {
    sendMail,
    sendVerificationEmail
};