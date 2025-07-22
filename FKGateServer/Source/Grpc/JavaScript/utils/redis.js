/**
 * Redis工具模块
 * 提供Redis连接和操作的封装
 */

const { redisConfig } = require('../config/config-loader');
const Redis = require("ioredis");

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

// 创建Redis客户端实例
const redisClient = new Redis({
    host: redisConfig.host,       // Redis服务器主机名
    port: redisConfig.port,       // Redis服务器端口号
    password: redisConfig.password, // Redis密码
    retryStrategy: (times) => {
        // 重试策略：最多重试5次，每次间隔2000ms
        const delay = Math.min(times * 2000, 10000);
        return times <= 5 ? delay : null;
    }
});

/**
 * 监听错误信息
 */
redisClient.on("error", function (err) {
    currentLogger.error(`Redis连接错误: ${err.message}`);
});

/**
 * 根据key获取value
 * @param {string} key - Redis键名
 * @param {Object} [logger] - 可选的日志记录器实例
 * @returns {Promise<string|null>} - 返回键值或null（如果键不存在）
 */
async function getRedis(key, logger) {
    // 如果提供了logger参数，则使用它
    if (logger && typeof logger.info === 'function') {
        currentLogger = logger;
    }
    
    try {
        const result = await redisClient.get(key);
        if (result === null) {
            currentLogger.debug(`键 ${key} 不存在`);
            return null;
        }
        currentLogger.debug(`成功获取键 ${key} 的值: ${result}`);
        return result;
    } catch (error) {
        currentLogger.error(`获取键 ${key} 时发生错误: ${error.message}`);
        return null;
    }
}

/**
 * 根据key查询redis中是否存在key
 * @param {string} key - 要查询的键名
 * @param {Object} [logger] - 可选的日志记录器实例
 * @returns {Promise<boolean|null>} - 存在返回true，不存在返回false，错误返回null
 */
async function queryRedis(key, logger) {
    // 如果提供了logger参数，则使用它
    if (logger && typeof logger.info === 'function') {
        currentLogger = logger;
    }
    
    try {
        const result = await redisClient.exists(key);
        const exists = result === 1;
        currentLogger.debug(`键 ${key} ${exists ? '存在' : '不存在'}`);
        return exists;
    } catch (error) {
        currentLogger.error(`查询键 ${key} 时发生错误: ${error.message}`);
        return null;
    }
}

/**
 * 设置key和value，并设置过期时间
 * @param {string} key - 键名
 * @param {string} value - 键值
 * @param {number} exptime - 过期时间（秒）
 * @param {Object} [logger] - 可选的日志记录器实例
 * @returns {Promise<boolean>} - 操作成功返回true，失败返回false
 */
async function setRedisExpire(key, value, exptime, logger) {
    // 如果提供了logger参数，则使用它
    if (logger && typeof logger.info === 'function') {
        currentLogger = logger;
    }
    
    try {
        // 使用单个命令设置键值和过期时间，更高效
        await redisClient.set(key, value, 'EX', exptime);
        currentLogger.debug(`成功设置键 ${key} 的值，过期时间 ${exptime} 秒`);
        return true;
    } catch (error) {
        currentLogger.error(`设置键 ${key} 时发生错误: ${error.message}`);
        return false;
    }
}

/**
 * 删除指定的键
 * @param {string} key - 要删除的键名
 * @param {Object} [logger] - 可选的日志记录器实例
 * @returns {Promise<boolean>} - 操作成功返回true，失败返回false
 */
async function deleteRedis(key, logger) {
    // 如果提供了logger参数，则使用它
    if (logger && typeof logger.info === 'function') {
        currentLogger = logger;
    }
    
    try {
        await redisClient.del(key);
        currentLogger.debug(`成功删除键 ${key}`);
        return true;
    } catch (error) {
        currentLogger.error(`删除键 ${key} 时发生错误: ${error.message}`);
        return false;
    }
}

/**
 * 关闭Redis连接
 * @param {Object} [logger] - 可选的日志记录器实例
 */
function quit(logger) {
    // 如果提供了logger参数，则使用它
    if (logger && typeof logger.info === 'function') {
        currentLogger = logger;
    }
    
    redisClient.quit();
    currentLogger.info('Redis连接已关闭');
}

module.exports = { 
    getRedis, 
    queryRedis, 
    setRedisExpire, 
    deleteRedis,
    quit,
    setLogger
}