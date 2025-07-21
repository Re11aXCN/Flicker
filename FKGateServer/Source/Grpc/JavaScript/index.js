/**
 * Flicker gRPC服务入口文件
 * 同时启动验证码服务、加密服务和认证服务
 */

const { fork } = require('child_process');
const path = require('path');
const { serviceConfig } = require('./config/config-loader');
const { Logger, GeneratePolicy } = require('./utils/logger');

// 初始化日志系统
const logInitResult = Logger.initialize('Flicker-RPC', GeneratePolicy.SingleFile, true);
if (!logInitResult) {
    console.error('日志系统初始化失败，服务将继续启动但不会记录日志到文件');
}

/**
 * 启动服务
 * @param {string} scriptPath - 服务脚本路径
 * @param {string} serviceName - 服务名称
 */
function startService(scriptPath, serviceName) {
    const startMessage = `正在启动 ${serviceName}...`;
    console.log(startMessage);
    Logger.info(startMessage);
    
    const child = fork(scriptPath);
    
    child.on('message', (message) => {
        const msgText = `[${serviceName}] 消息: ${JSON.stringify(message)}`;
        console.log(msgText);
        Logger.info(msgText);
    });
    
    child.on('error', (error) => {
        const errorText = `[${serviceName}] 错误: ${error.message}`;
        console.error(errorText);
        Logger.error(errorText);
    });
    
    child.on('exit', (code, signal) => {
        if (code !== 0) {
            const exitErrorText = `[${serviceName}] 异常退出，代码: ${code}, 信号: ${signal}`;
            console.error(exitErrorText);
            Logger.error(exitErrorText);
            // 可以选择重启服务
            const restartText = `正在重启 ${serviceName}...`;
            console.log(restartText);
            Logger.warn(restartText);
            startService(scriptPath, serviceName);
        } else {
            const normalExitText = `[${serviceName}] 正常退出`;
            console.log(normalExitText);
            Logger.info(normalExitText);
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
    Logger.info('收到退出信号，正在关闭所有服务...');
    
    verificationServer.kill();
    encryptionServer.kill();
    authenticationServer.kill();
    
    // 关闭日志系统
    Logger.shutdown();
    process.exit(0);
});

const startupMessage = `所有服务已启动：
- 验证码服务: ${serviceConfig.verification.host}:${serviceConfig.verification.port}
- 加密服务: ${serviceConfig.encryption.host}:${serviceConfig.encryption.port}
- 认证服务: ${serviceConfig.authentication.host}:${serviceConfig.authentication.port}`;

console.log(startupMessage);
Logger.info(startupMessage);