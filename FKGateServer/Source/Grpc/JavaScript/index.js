/**
 * Flicker gRPC服务入口文件
 * 同时启动验证码服务、加密服务和认证服务
 */

const { fork } = require('child_process');
const path = require('path');
const { serviceConfig } = require('./config/config-loader');
const { createLogger } = require('./utils/logger');

// 创建主进程的日志记录器
const IndexLogger = createLogger('主进程', 'Flicker-RPC-Index');

// ==== 设置UTF-8编码 ====
if (process.platform === 'win32') {
    // Windows系统执行chcp命令切换代码页
    const { execSync } = require('child_process');
    execSync('chcp 65001', { stdio: 'inherit' });
} else {
    // 非Windows系统设置环境变量
    process.env.LANG = 'en_US.UTF-8';
    process.env.LC_ALL = 'en_US.UTF-8';
}

/**
 * 启动服务
 * @param {string} scriptPath - 服务脚本路径
 * @param {string} serviceName - 服务名称
 * @param {string} logFileName - 日志文件名
 */
function startService(scriptPath, serviceName, logFileName) {
    const startMessage = `正在启动 ${serviceName}...`;
    IndexLogger.info(startMessage);
    
    // 传递日志文件名作为环境变量
    const child = fork(scriptPath, [], {
        env: {
            ...process.env,
            LOG_FILE_NAME: logFileName
        }
    });
    
    child.on('message', (message) => {
        const msgText = `[${serviceName}] 消息: ${JSON.stringify(message)}`;
        IndexLogger.info(msgText);
    });
    
    child.on('error', (error) => {
        const errorText = `[${serviceName}] 错误: ${error.message}`;
        IndexLogger.error(errorText);
    });
    
    child.on('exit', (code, signal) => {
        if (code !== 0) {
            const exitErrorText = `[${serviceName}] 异常退出，代码: ${code}, 信号: ${signal}`;
            IndexLogger.error(exitErrorText);
            // 可以选择重启服务
            const restartText = `正在重启 ${serviceName}...`;
            IndexLogger.warn(restartText);
            startService(scriptPath, serviceName, logFileName);
        } else {
            const normalExitText = `[${serviceName}] 正常退出`;
            IndexLogger.info(normalExitText);
        }
    });
    
    return child;
}

// 启动验证码服务
const verificationServerPath = path.join(__dirname, './server/verification.js');
const verificationServer = startService(verificationServerPath, '验证码服务', 'Flicker-RPC-Verification');

// 启动加密服务
const encryptionServerPath = path.join(__dirname, './server/encryption.js');
const encryptionServer = startService(encryptionServerPath, '加密服务', 'Flicker-RPC-Encryption');

// 启动认证服务
const authenticationServerPath = path.join(__dirname, './server/authentication.js');
const authenticationServer = startService(authenticationServerPath, '认证服务', 'Flicker-RPC-Authentication');

// 处理主进程的退出信号
process.on('SIGINT', () => {
    IndexLogger.info('收到退出信号，正在关闭所有服务...');
    
    verificationServer.kill();
    encryptionServer.kill();
    authenticationServer.kill();
    
    // 关闭日志系统
    IndexLogger.shutdown();
    process.exit(0);
});

const startupMessage = `所有服务已启动：
- 验证码服务: ${serviceConfig.verification.host}:${serviceConfig.verification.port}
- 加密服务: ${serviceConfig.encryption.host}:${serviceConfig.encryption.port}
- 认证服务: ${serviceConfig.authentication.host}:${serviceConfig.authentication.port}`;

IndexLogger.info(startupMessage);