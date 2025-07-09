/**
 * Flicker gRPC服务入口文件
 * 同时启动邮箱验证码服务和密码加密服务
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

// 启动邮箱验证码服务
const verificationServerPath = path.join(__dirname, 'verification-server.js');
const verificationServer = startService(verificationServerPath, '邮箱验证码服务');

// 启动密码加密服务
const cipherServerPath = path.join(__dirname, 'cipher-server.js');
const cipherServer = startService(cipherServerPath, '密码加密服务');

// 处理主进程的退出信号
process.on('SIGINT', () => {
    console.log('收到退出信号，正在关闭所有服务...');
    verificationServer.kill();
    cipherServer.kill();
    process.exit(0);
});

console.log(`所有服务已启动：
- 邮箱验证码服务: ${serviceConfig.verification.host}:${serviceConfig.verification.port}
- 密码加密服务: ${serviceConfig.cipher.host}:${serviceConfig.cipher.port}`);