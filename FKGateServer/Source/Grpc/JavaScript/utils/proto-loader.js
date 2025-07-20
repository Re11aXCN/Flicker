/**
 * Proto文件加载器
 * 负责加载和解析gRPC服务定义文件
 */

const path = require('path');
const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');

const PROTO_PATH = path.join(__dirname, '../../FKGrpcService.proto');

function loadProtoFile() {
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
        console.error(`加载Proto文件失败: ${error.message}`);
        console.error(`Proto文件路径: ${PROTO_PATH}`);
        if (error.code === 'ENOENT') {
            console.error('错误原因: Proto文件不存在');
        }
        throw new Error(`Proto文件加载失败: ${error.message}`);
    }
}

module.exports = { loadProtoFile, };