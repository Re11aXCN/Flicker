﻿/**
 * 配置加载器
 * 负责加载和解析配置文件
 */

const fs = require('fs');
const path = require('path');

/**
 * 加载配置文件
 * @returns {Object} 解析后的配置对象
 */
function loadConfig() {
    try {
        // 使用相对路径，从当前文件所在目录读取配置
        const configPath = path.join(__dirname, 'config.json');
        let configData = fs.readFileSync(configPath, 'utf8');

        // 修复：移除开头的 BOM 字符
        if (configData.charCodeAt(0) === 0xFEFF) {
            configData = configData.slice(1);
        }

        return JSON.parse(configData);
    } catch (error) {
        console.error(`加载配置文件失败: ${error.message}`);
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
    cipher: config.server.cipher
};

module.exports = {
    mailConfig,
    mysqlConfig,
    redisConfig,
    serviceConfig
};