/**
 * 配置加载器
 * 负责加载和解析配置文件
 */

const fs = require('fs');
const path = require('path');

// 默认使用console作为日志记录器
let currentLogger = {
    info: console.info,
    error: console.error,
    debug: console.debug,
    warn: console.warn
};

/**
 * 设置当前使用的日志记录器
 * @param {Object} logger - 日志记录器实例
 */
function setLogger(logger) {
    if (logger && typeof logger.info === 'function') {
        currentLogger = logger;
    }
}

/**
 * 获取当前运行环境
 * @returns {string} 当前环境 ('development' 或 'production')
 */
function getEnvironment() {
    return process.env.NODE_ENV || 'development';
}

/**
 * 获取配置文件路径
 * @returns {string} 配置文件路径
 */
function getConfigPath() {
    const env = getEnvironment();
    let configFileName;
    
    switch (env) {
        case 'production':
            configFileName = 'config.prod.json';
            break;
        case 'development':
        default:
            configFileName = 'config.dev.json';
            break;
    }
    
    // 如果指定的环境配置文件不存在，回退到默认配置
    const envConfigPath = path.join(__dirname, configFileName);
    if (!fs.existsSync(envConfigPath)) {
        currentLogger.warn(`环境配置文件 ${configFileName} 不存在，使用默认配置文件`);
        return path.join(__dirname, 'config.json');
    }
    
    return envConfigPath;
}

/**
 * 加载配置文件
 * @param {Object} [logger] - 可选的日志记录器实例
 * @returns {Object} 解析后的配置对象
 */
function loadConfig(logger) {
    // 如果提供了logger参数，则使用它
    if (logger && typeof logger.info === 'function') {
        currentLogger = logger;
    }
    try {
        const configPath = getConfigPath();
        let configData = fs.readFileSync(configPath, 'utf8');

        // 修复：移除开头的 BOM 字符
        if (configData.charCodeAt(0) === 0xFEFF) {
            configData = configData.slice(1);
        }

        return JSON.parse(configData);
    } catch (error) {
        currentLogger.error(`加载配置文件失败: ${error.message}`);
        throw new Error(`配置文件加载失败: ${error.message}`);
    }
}

// 加载配置
const config = loadConfig();

// mail配置
const mailConfig = {
    smtp: config.mail.smtp,
    account: config.mail.account,
};

// MySQL配置
const mysqlConfig = {
    host: config.mysql.host,
    port: config.mysql.port,
    password: config.mysql.password
};

// Redis配置
const redisConfig = {
    host: config.redis.host,
    port: config.redis.port,
    password: config.redis.password
};

// 服务配置
const serviceConfig = {
    verification: config.server.verification,
    encryption: config.server.encryption,
    authentication: config.server.authentication
};

module.exports = {
    mailConfig,
    mysqlConfig,
    redisConfig,
    serviceConfig,
    setLogger,
    loadConfig
};