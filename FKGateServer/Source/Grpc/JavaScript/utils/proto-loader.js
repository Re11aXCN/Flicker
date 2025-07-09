/**
 * Proto文件加载器
 * 负责加载和解析gRPC服务定义文件
 */

const path = require('path');
const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');

const VERIFY_CODE_PROTO_PATH = path.join(__dirname, '../../VerifyCode/FKVerifyCodeGrpc.proto');
const PASSWORD_PROTO_PATH = path.join(__dirname, '../../Password/FKPasswordGrpc.proto');

function loadVerifyCodeProtoFile() {
    try {
        // 加载Proto文件定义
        const packageDefinition = protoLoader.loadSync(
            VERIFY_CODE_PROTO_PATH,
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
        
        // 返回验证码服务定义
        return protoDescriptor.FKVerifyCodeGrpc;
    } catch (error) {
        console.error(`加载Proto文件失败: ${error.message}`);
        console.error(`Proto文件路径: ${VERIFY_CODE_PROTO_PATH}`);
        if (error.code === 'ENOENT') {
            console.error('错误原因: Proto文件不存在');
        }
        throw new Error(`Proto文件加载失败: ${error.message}`);
    }
}

function loadPasswordProtoFile() {
    try {
        // 加载Proto文件定义
        const packageDefinition = protoLoader.loadSync(
            PASSWORD_PROTO_PATH,
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

        // 返回验证码服务定义
        return protoDescriptor.FKPasswordGrpc;
    } catch (error) {
        console.error(`加载Proto文件失败: ${error.message}`);
        console.error(`Proto文件路径: ${PASSWORD_PROTO_PATH}`);
        if (error.code === 'ENOENT') {
            console.error('错误原因: Proto文件不存在');
        }
        throw new Error(`Proto文件加载失败: ${error.message}`);
    }
}

module.exports = { loadVerifyCodeProtoFile, loadPasswordProtoFile };