﻿/**
 * Redis工具模块
 * 提供Redis连接和操作的封装
 */

const { redisConfig } = require('../config/config-loader');
const Redis = require("ioredis");

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
    console.error(`Redis连接错误: ${err.message}`);
});

/**
 * 根据key获取value
 * @param {string} key - Redis键名
 * @returns {Promise<string|null>} - 返回键值或null（如果键不存在）
 */
async function getRedis(key) {
    try {
        const result = await redisClient.get(key);
        if (result === null) {
            console.log(`键 ${key} 不存在`);
            return null;
        }
        console.log(`成功获取键 ${key} 的值: ${result}`);
        return result;
    } catch (error) {
        console.error(`获取键 ${key} 时发生错误:`, error);
        return null;
    }
}

/**
 * 根据key查询redis中是否存在key
 * @param {string} key - 要查询的键名
 * @returns {Promise<boolean|null>} - 存在返回true，不存在返回false，错误返回null
 */
async function queryRedis(key) {
    try {
        const result = await redisClient.exists(key);
        const exists = result === 1;
        console.log(`键 ${key} ${exists ? '存在' : '不存在'}`);
        return exists;
    } catch (error) {
        console.error(`查询键 ${key} 时发生错误:`, error);
        return null;
    }
}

/**
 * 设置key和value，并设置过期时间
 * @param {string} key - 键名
 * @param {string} value - 键值
 * @param {number} exptime - 过期时间（秒）
 * @returns {Promise<boolean>} - 操作成功返回true，失败返回false
 */
async function setRedisExpire(key, value, exptime) {
    try {
        // 使用单个命令设置键值和过期时间，更高效
        await redisClient.set(key, value, 'EX', exptime);
        console.log(`成功设置键 ${key} 的值，过期时间 ${exptime} 秒`);
        return true;
    } catch (error) {
        console.error(`设置键 ${key} 时发生错误:`, error);
        return false;
    }
}

/**
 * 删除指定的键
 * @param {string} key - 要删除的键名
 * @returns {Promise<boolean>} - 操作成功返回true，失败返回false
 */
async function deleteRedis(key) {
    try {
        await redisClient.del(key);
        console.log(`成功删除键 ${key}`);
        return true;
    } catch (error) {
        console.error(`删除键 ${key} 时发生错误:`, error);
        return false;
    }
}

/**
 * 关闭Redis连接
 */
function quit() {
    redisClient.quit();
    console.log('Redis连接已关闭');
}

module.exports = { 
    getRedis, 
    queryRedis, 
    setRedisExpire, 
    deleteRedis,
    quit 
}