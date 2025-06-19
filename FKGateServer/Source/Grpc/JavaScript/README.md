# 邮箱验证码服务

这是一个基于gRPC的邮箱验证码服务，用于Flicker应用的用户注册验证流程。服务通过SMTP协议发送验证码邮件，并提供验证接口。

## 功能特性

- 基于gRPC的验证码服务
- 通过SMTP发送邮件验证码
- 支持HTML格式的邮件内容
- 使用UUID生成唯一验证码

## 目录结构

```
├── config/                  # 配置文件目录
│   ├── config.json         # 配置文件
│   └── configLoader.js     # 配置加载器
├── services/                # 服务模块目录
│   └── emailService.js     # 邮件服务
├── utils/                   # 工具模块目录
│   ├── constants.js        # 常量定义
│   └── protoLoader.js      # Proto加载器
├── emailVerificationServer.js # 服务入口文件
├── package.json            # 项目配置
└── README.md               # 项目说明
```

## 安装与运行

### 前置条件

- Node.js 14.0 或更高版本
- npm 或 yarn

### 安装依赖

```bash
npm install
```

### 配置

在 `config/config.json` 中配置邮箱和数据库信息：

```json
{
  "email": {
    "user": "your-email@example.com",
    "authcode": "your-email-auth-code"
  },
  "mysql": {
    "host": "localhost",
    "port": 3306,
    "password": "your-password"
  },
  "redis": {
    "host": "localhost",
    "port": 6379,
    "password": "your-password"
  }
}
```

### 运行服务

```bash
npm start
```

## API 说明

### GetVerificationCode

发送验证码到指定邮箱。

**请求参数：**

```protobuf
message VarifyCodeRequestBody {
  string email = 1;
}
```

**响应参数：**

```protobuf
message VarifyCodeResponseBody {
  int32 error = 1;  // 错误码，0表示成功
  string email = 2; // 邮箱地址
  string code = 3;  // 验证码（仅在测试环境返回）
}
```

## 错误码

| 错误码 | 描述 |
|--------|------|
| 0      | 成功 |
| 1      | Redis错误 |
| 2      | 系统异常 |
| 3      | 邮件发送失败 |
| 4      | 验证码已过期 |
| 5      | 验证码不匹配 |