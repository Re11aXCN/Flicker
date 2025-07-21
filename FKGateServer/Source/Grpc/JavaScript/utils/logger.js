/**
 * 日志工具模块
 * 使用ffi-rs调用C++的FKLogger库进行日志记录
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
const dllDir = 'E:/Development/Project/my/Flicker/Bin/Flicker_RelWithDebInfo_x64';
const isWindows = process.platform === 'win32';
const separator = isWindows ? ';' : ':';
process.env.PATH = `${dllDir}${separator}${process.env.PATH}`;
console.log('Updated PATH:', process.env.PATH);
//open({
//    library: 'kernel32',
//    path: 'kernel32.dll'
//});
//const kernel32 = define({
//    SetDllDirectoryW: {
//        library: "kernel32",
//        retType: DataType.Boolean,
//        paramsType: [DataType.String]
//    }
//})
//const success = kernel32.SetDllDirectoryW([dllDir]);
const spdlogPath = path.join(dllDir, 'spdlog.dll');
const fmtPath = path.join(dllDir, 'fmt.dll');
const fkloggerPath = path.join(dllDir, 'fklogger.dll');

// 打开动态库
open({
    library: 'spdlog',
    path: spdlogPath
});
open({
    library: 'fmt',
    path: fmtPath
});
open({
    library: 'fklogger',
    path: fkloggerPath
});

// 定义C++库函数接口
const FKLoggerLib = define({
    // 初始化日志记录器
    FKLogger_Initialize: {
        library: 'fklogger',
        retType: DataType.Boolean,
        paramsType: [DataType.String, DataType.I32, DataType.Boolean]
    },
    
    // 关闭并刷新所有日志
    FKLogger_Shutdown: {
        library: 'fklogger',
        retType: DataType.Void,
        paramsType: []
    },
    
    // 手动刷新日志缓冲区
    FKLogger_Flush: {
        library: 'fklogger',
        retType: DataType.Void,
        paramsType: []
    },
    
    // 记录不同级别的日志
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
    }
});

/**
 * 日志记录器类
 */
class Logger {
    /**
     * 初始化日志记录器
     * @param {string} filename - 日志文件名（不含路径和扩展名）
     * @param {number} policy - 日志生成策略 (0: SingleFile, 1: MultiplePerDay, 2: MultiplePerRun)
     * @param {boolean} truncate - 是否在启动时清空旧日志
     * @returns {boolean} - 初始化是否成功
     */
    static initialize(filename = 'Flicker-RPC', policy = GeneratePolicy.SingleFile, truncate = true) {
        try {
            const result = FKLoggerLib.FKLogger_Initialize([filename, policy, truncate]);
            if (result) {
                this.info('日志系统初始化成功');
            } else {
                console.error('日志系统初始化失败');
            }
            return result;
        } catch (error) {
            console.error(`日志系统初始化异常: ${error.message}`);
            return false;
        }
    }

    /**
     * 关闭日志系统
     */
    static shutdown() {
        try {
            this.info('日志系统正在关闭...');
            FKLoggerLib.FKLogger_Shutdown([]);
        } catch (error) {
            console.error(`日志系统关闭异常: ${error.message}`);
        }
    }

    /**
     * 手动刷新日志缓冲区
     */
    static flush() {
        try {
            FKLoggerLib.FKLogger_Flush([]);
        } catch (error) {
            console.error(`日志刷新异常: ${error.message}`);
        }
    }

    /**
     * 记录调试级别日志
     * @param {string} message - 日志消息
     */
    static debug(message) {
        try {
            FKLoggerLib.FKLogger_Debug([message]);
        } catch (error) {
            console.error(`记录调试日志异常: ${error.message}`);
        }
    }

    /**
     * 记录信息级别日志
     * @param {string} message - 日志消息
     */
    static info(message) {
        try {
            FKLoggerLib.FKLogger_Info([message]);
        } catch (error) {
            console.error(`记录信息日志异常: ${error.message}`);
        }
    }

    /**
     * 记录警告级别日志
     * @param {string} message - 日志消息
     */
    static warn(message) {
        try {
            FKLoggerLib.FKLogger_Warn([message]);
        } catch (error) {
            console.error(`记录警告日志异常: ${error.message}`);
        }
    }

    /**
     * 记录错误级别日志
     * @param {string} message - 日志消息
     */
    static error(message) {
        try {
            FKLoggerLib.FKLogger_Error([message]);
        } catch (error) {
            console.error(`记录错误日志异常: ${error.message}`);
        }
    }
}

// 导出日志记录器和相关枚举
module.exports = {
    Logger,
    LogLevel,
    GeneratePolicy
};

// 进程退出时关闭动态库
process.on('exit', () => {
    try {
        require('ffi-rs').close('fklogger');
    } catch (error) {
        console.error(`关闭动态库异常: ${error.message}`);
    }
});