# Flicker gRPC 服务

这是Flicker应用的gRPC服务，包括邮箱验证码、密码加密和认证服务。

## 环境要求

- Node.js 14.0.0 或更高版本
- npm 6.0.0 或更高版本
- Redis 服务器

## 安装依赖

```bash
npm install
```

## 配置文件

项目使用不同的配置文件来支持不同的运行环境：

- `config/config.dev.json`: 开发环境配置
- `config/config.prod.json`: 生产环境配置
- `config/config.json`: 默认配置（当指定环境的配置文件不存在时使用）

## 启动方式

### 开发环境

```bash
# 使用 nodemon 启动所有服务（自动重启）
npm run dev

# 或使用批处理文件启动
start-dev.bat

# 单独启动各服务
npm run dev:verification  # 验证码服务
npm run dev:encryption    # 加密服务
npm run dev:authentication # 认证服务

# 同时启动所有服务（使用 concurrently）
npm run dev:all
```

### 调试环境

```bash
# 启动调试模式
npm run debug

# 或使用批处理文件启动
start-debug.bat

# 单独调试各服务
npm run debug:verification  # 验证码服务
npm run debug:encryption    # 加密服务
npm run debug:authentication # 认证服务
```

调试时，请在Chrome浏览器中访问 `chrome://inspect` 进行调试。

### 生产环境

```bash
# 启动所有服务
npm run start

# 或使用批处理文件启动
start-prod.bat

# 单独启动各服务
npm run start:verification  # 验证码服务
npm run start:encryption    # 加密服务
npm run start:authentication # 认证服务
```

## 服务说明

- **验证码服务**: 处理验证码请求并发送邮件
- **加密服务**: 处理密码加密请求
- **认证服务**: 处理密码验证请求

## 日志系统

项目使用 `FKLogger` 进行日志记录，日志级别可在配置文件中设置：

- 开发环境: `debug` 级别
- 生产环境: `info` 级别