/**
 * 日志工具模块
 * 使用ffi-rs调用C++的FKLogger库进行日志记录
 * 支持为不同服务创建独立的日志实例
 */
// 日志级别枚举
const LogLevel = {
    TRACE: 0,
    DEBUG: 1,
    INFO: 2,
    WARN: 3,
    ERROR: 4,
    CRITICAL: 5
};

// 日志生成策略枚举
const GeneratePolicy = {
    SingleFile: 0,       // 单个日志文件
    MultiplePerDay: 1,   // 按日期生成文件
    MultiplePerRun: 2    // 每次运行生成新文件
};


const { open, load, DataType, define } = require('ffi-rs');
const path = require('path');
const fs = require('fs');

const dllDir = 'E:/Development/Project/my/Flicker/Bin/Flicker_RelWithDebInfo_x64';
const separator = process.platform === 'win32' ? ';' : ':';
process.env.PATH = `${dllDir}${separator}${process.env.PATH}`;

//const spdlogPath = path.join(dllDir, 'spdlog.dll');
//const fmtPath = path.join(dllDir, 'fmt.dll');
const fkloggerPath = path.join(dllDir, 'fklogger.dll');
//open({ library: 'spdlog', path: spdlogPath });
//open({ library: 'fmt', path: fmtPath });
open({ library: 'fklogger', path: fkloggerPath });

// 定义C++库函数接口
const FKLoggerLib = define({
    FKLogger_Initialize: {
        library: 'fklogger',
        retType: DataType.Boolean,
        paramsType: [DataType.String, DataType.I32, DataType.Boolean, DataType.String]
    },
    FKLogger_Shutdown: {
        library: 'fklogger',
        retType: DataType.Void,
        paramsType: []
    },
    FKLogger_Flush: {
        library: 'fklogger',
        retType: DataType.Void,
        paramsType: []
    },
    FKLogger_Info: {
        library: 'fklogger',
        retType: DataType.Void,
        paramsType: [DataType.String]
    },
    FKLogger_Warn: {
        library: 'fklogger',
        retType: DataType.Void,
        paramsType: [DataType.String]
    },
    FKLogger_Error: {
        library: 'fklogger',
        retType: DataType.Void,
        paramsType: [DataType.String]
    },
    FKLogger_Debug: {
        library: 'fklogger',
        retType: DataType.Void,
        paramsType: [DataType.String]
    },
    FKLogger_Critical: {
        library: 'fklogger',
        retType: DataType.Void,
        paramsType: [DataType.String]
    },
    FKLogger_Trace: {
        library: 'fklogger',
        retType: DataType.Void,
        paramsType: [DataType.String]
    }
});

/**
 * 日志记录器类
 */
class Logger {
    constructor(serviceName) {
        this.serviceName = serviceName;
        this.initialized = false;
    }

    /**
     * 初始化日志记录器
     * @param {string} fileName - 日志文件名（不含路径和扩展名）
     * @param {number} policy - 日志生成策略 (0: SingleFile, 1: MultiplePerDay, 2: MultiplePerRun)
     * @param {boolean} truncate - 是否在启动时清空旧日志
     * @param {string} fileDir - 日志文件目录
     * @returns {boolean} - 初始化是否成功
     */
    initialize(fileName, policy, truncate = false, fileDir = '') {
        try {
            fs.mkdirSync(fileDir, { recursive: true });

            const result = FKLoggerLib.FKLogger_Initialize([fileName, policy, truncate, fileDir]);
            if (result) {
                this.initialized = true;
                this.info(`${this.serviceName}日志系统初始化成功`);
            } else {
                console.error(`${this.serviceName}日志系统初始化失败`);
            }
            return result;
        } catch (error) {
            console.error(`${this.serviceName}日志系统初始化异常: ${error.message}`);
            return false;
        }
    }

    shutdown() {
        try {
            if (this.initialized) {
                this.info(`${this.serviceName}日志系统正在关闭...`);
                FKLoggerLib.FKLogger_Shutdown([]);
                this.initialized = false;
            }
        } catch (error) {
            // 这里必须使用console.error，因为Logger可能已经无法正常工作
            console.error(`${this.serviceName}日志系统关闭异常: ${error.message}`);
        }
    }

    flush() {
        try {
            if (this.initialized) {
                FKLoggerLib.FKLogger_Flush([]);
            }
        } catch (error) {
            console.error(`${this.serviceName}日志刷新异常: ${error.message}`);
        }
    }

    debug(message) {
        try {
            if (this.initialized) {
                FKLoggerLib.FKLogger_Debug([message]);
            }
        } catch (error) {
            console.error(`${this.serviceName}记录调试日志异常: ${error.message}`);
        }
    }

    info(message) {
        try {
            if (this.initialized) {
                FKLoggerLib.FKLogger_Info([message]);
            }
        } catch (error) {
            console.error(`${this.serviceName}记录信息日志异常: ${error.message}`);
        }
    }

    warn(message) {
        try {
            if (this.initialized) {
                FKLoggerLib.FKLogger_Warn([message]);
            }
        } catch (error) {
            console.error(`${this.serviceName}记录警告日志异常: ${error.message}`);
        }
    }

    error(message) {
        try {
            if (this.initialized) {
                FKLoggerLib.FKLogger_Error([message]);
            }
        } catch (error) {
            console.error(`${this.serviceName}记录错误日志异常: ${error.message}`);
        }
    }

    critical(message) {
        try {
            if (this.initialized) {
                FKLoggerLib.FKLogger_Critical([message]);
            }
        } catch (error) {
            console.error(`${this.serviceName}记录严重日志异常: ${error.message}`);
        }
    }
    
    trace(message) {
        try {
            if (this.initialized) {
                FKLoggerLib.FKLogger_Trace([message]);
            }
        } catch (error) {
            console.error(`${this.serviceName}记录追踪日志异常: ${error.message}`);
        }
    }
}

/**
 * 创建日志记录器实例
 * @param {string} serviceName - 服务名称
 * @param {string} fileName - 日志文件名（不含路径和扩展名）
 * @param {number} policy - 日志生成策略
 * @param {boolean} truncate - 是否在启动时清空旧日志
 * @returns {Logger} - 日志记录器实例
 */
function createLogger(serviceName, fileName, policy = GeneratePolicy.SingleFile, truncate = true) {
    const logger = new Logger(serviceName);
    const logDir = path.join(dllDir, 'logs');
    const initResult = logger.initialize(fileName, policy, truncate, logDir);
    
    if (!initResult) {
        console.error(`${serviceName}日志系统初始化失败，服务将继续启动但不会记录日志到文件`);
    }
    
    return logger;
}

module.exports = {
    Logger,
    LogLevel,
    GeneratePolicy,
    dllDir,
    createLogger
};

// 进程退出时关闭动态库
process.on('exit', () => {
    try {
        require('ffi-rs').close('fklogger');
        //require('ffi-rs').close('fmt');
        //require('ffi-rs').close('spdlog');
    } catch (error) {
        console.error(`关闭动态库异常: ${error.message}`);
    }
});