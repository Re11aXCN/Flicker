/**
 * Proto文件加载器
 * 负责加载和解析gRPC服务定义文件
 */

const path = require('path');
const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');

const PROTO_PATH = path.join(__dirname, '../../FKGrpcService.proto');

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
 * 加载Proto文件
 * @param {Object} [logger] - 可选的日志记录器实例
 * @returns {Object} Proto服务描述对象
 */
function loadProtoFile(logger) {
    // 如果提供了logger参数，则使用它
    if (logger && typeof logger.info === 'function') {
        currentLogger = logger;
    }
    try {
        // 加载Proto文件定义
        const packageDefinition = protoLoader.loadSync(
            PROTO_PATH,
            {
                keepCase: true,
                longs: String,
                enums: String,
                defaults: true,
                oneofs: true
            }
        );

        // 解析Proto包定义
        const protoDescriptor = grpc.loadPackageDefinition(packageDefinition);
        
        // 返回服务定义
        return protoDescriptor.FKGrpcService;
    } catch (error) {
        currentLogger.error(`加载Proto文件失败: ${error.message}`);
        currentLogger.error(`Proto文件路径: ${PROTO_PATH}`);
        if (error.code === 'ENOENT') {
            currentLogger.error('错误原因: Proto文件不存在');
        }
        throw new Error(`Proto文件加载失败: ${error.message}`);
    }
}

module.exports = { loadProtoFile, setLogger };