/**
 * 常量定义文件
 * 包含系统中使用的各种常量和错误码
 */

/**
 * 验证码相关常量
 */
const VERIFICATION = {
    // 验证码在Redis中的前缀
    CODE_PREFIX: "verification_code_",
    // 验证码有效期（秒）
    EXPIRATION: 300
};

/**
 * 错误码定义
 */
const ERROR_CODE = {
    // 操作成功
    SUCCESS: 0,
    // Redis操作错误
    REDIS_ERROR: 1,
    // 系统异常
    EXCEPTION: 2,
    // 邮件发送失败
    EMAIL_SEND_FAILED: 3,
    // 验证码已过期
    CODE_EXPIRED: 4,
    // 验证码不匹配
    CODE_MISMATCH: 5,
    // 加密错误
    ENCRYPT_ERROR: 6,
    // 密码认证失败
    CIPHER_AUTH_FAILED: 7,
    // 表单参数缺失
    FORM_PARAMS_MISSING: 8,
};

module.exports = {
    VERIFICATION,
    ERROR_CODE
};