/**
 * Flicker gRPC服务入口文件
 * 同时启动验证码服务、加密服务和认证服务
 */

const { fork } = require('child_process');
const path = require('path');
const { serviceConfig } = require('./config/config-loader');

/**
 * 启动服务
 * @param {string} scriptPath - 服务脚本路径
 * @param {string} serviceName - 服务名称
 */
function startService(scriptPath, serviceName) {
    console.log(`正在启动 ${serviceName}...`);
    
    const child = fork(scriptPath);
    
    child.on('message', (message) => {
        console.log(`[${serviceName}] 消息:`, message);
    });
    
    child.on('error', (error) => {
        console.error(`[${serviceName}] 错误:`, error);
    });
    
    child.on('exit', (code, signal) => {
        if (code !== 0) {
            console.error(`[${serviceName}] 异常退出，代码: ${code}, 信号: ${signal}`);
            // 可以选择重启服务
            console.log(`正在重启 ${serviceName}...`);
            startService(scriptPath, serviceName);
        } else {
            console.log(`[${serviceName}] 正常退出`);
        }
    });
    
    return child;
}

// 启动验证码服务
const verificationServerPath = path.join(__dirname, './server/verification.js');
const verificationServer = startService(verificationServerPath, '验证码服务');

// 启动加密服务
const encryptionServerPath = path.join(__dirname, './server/encryption.js');
const encryptionServer = startService(encryptionServerPath, '加密服务');

// 启动认证服务
const authenticationServerPath = path.join(__dirname, './server/authentication.js');
const authenticationServer = startService(authenticationServerPath, '认证服务');

// 处理主进程的退出信号
process.on('SIGINT', () => {
    console.log('收到退出信号，正在关闭所有服务...');
    verificationServer.kill();
    encryptionServer.kill();
    authenticationServer.kill();
    process.exit(0);
});

console.log(`所有服务已启动：
- 验证码服务: ${serviceConfig.verification.host}:${serviceConfig.verification.port}
- 加密服务: ${serviceConfig.encryption.host}:${serviceConfig.encryption.port}
- 认证服务: ${serviceConfig.authentication.host}:${serviceConfig.authentication.port}`);