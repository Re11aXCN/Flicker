/**
 * 密码加密服务入口文件
 * 提供gRPC服务，处理密码加密和验证请求
 */

const bcrypt = require('bcrypt');
const { ERROR_CODE } = require('../utils/constants');

/**
 * 处理密码加密请求
 * @param {Object} call - gRPC调用对象，包含请求信息
 * @param {Function} callback - 回调函数，用于返回响应
 */
async function encryptPassword(call, callback) {
    const { request_type, hashed_password } = call.request;
    console.log('收到密码加密请求');
    
    if (!hashed_password) {
        console.error('密码参数为空');
        callback(null, {
            status_code: ERROR_CODE.FORM_PARAMS_MISSING,
            message: '密码参数不能为空',
            request_type: request_type,
            encrypted_password: '',
            salt: ''
        });
        return;
    }
    
    try {
        // 生成盐值，默认使用10轮加密
        const saltRounds = 10;
        const salt = await bcrypt.genSalt(saltRounds);
        
        // 使用盐值加密密码
        const encryptedPassword = await bcrypt.hash(hashed_password, salt);
        
        console.log('密码加密成功');
        
        // 返回加密后的密码和盐值
        callback(null, {
            status_code: ERROR_CODE.SUCCESS,
            message: '密码加密成功',
            request_type: request_type,
            encrypted_password: encryptedPassword,
            salt: salt
        });
    } catch (error) {
        console.error(`密码加密失败: ${error.message}`);
        
        callback(null, {
            status_code: ERROR_CODE.ENCRYPT_ERROR,
            message: `密码加密失败: ${error.message}`,
            request_type: request_type,
            encrypted_password: '',
            salt: ''
        });
    }
}

/**
 * 处理密码验证请求
 * @param {Object} call - gRPC调用对象，包含请求信息
 * @param {Function} callback - 回调函数，用于返回响应
 */
async function authenticatePassword(call, callback) {
    const { request_type, hashed_password, encrypted_password } = call.request;
    console.log('收到密码验证请求');
    
    if (!hashed_password || !encrypted_password) {
        console.error('密码参数不完整');
        callback(null, {
            status_code: ERROR_CODE.FORM_PARAMS_MISSING,
            message: '密码参数不完整',
            request_type: request_type
        });
        return;
    }
    
    try {
        // 验证密码
        const isValid = await bcrypt.compare(hashed_password, encrypted_password);
        
        console.log(`密码验证结果: ${isValid ? '有效' : '无效'}`);
        
        // 返回验证结果
        callback(null, {
            status_code: ERROR_CODE.SUCCESS,
            message: '密码验证完成',
            request_type: request_type
        });
    } catch (error) {
        console.error(`密码验证失败: ${error.message}`);
        
        callback(null, {
            status_code: ERROR_CODE.CIPHER_AUTH_FAILED,
            message: `密码验证失败: ${error.message}`,
            request_type: request_type
        });
    }
}
module.exports = {
    encryptPassword,
    authenticatePassword
};