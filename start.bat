@echo off
setlocal enabledelayedexpansion

:: 定义路径变量（方便后续修改）
set REDIS_DIR=E:\Development\Redis
set PROJECT_DIR=E:\Development\Project\my\Flicker\FKGrpcServer

:: 检查 MySQL 服务状态
sc query mysql | findstr /i "RUNNING" > nul
if %errorlevel% equ 0 (
    echo MySQL 服务已在运行
) else (
    echo 正在启动 MySQL 服务...
    net start mysql
    if %errorlevel% neq 0 (
        echo 启动 MySQL 失败，请检查服务状态
        pause
        exit /b 1
    )
)

:: 启动 Redis
echo 正在启动 Redis...
start "" "%REDIS_DIR%\redis-server.exe" "%REDIS_DIR%\redis.windows.conf"

:: 启动 Node.js 项目
echo 正在启动 Node.js 项目...
cd /d "%PROJECT_DIR%"
start "" "npm" start

echo 所有服务已启动完成!
timeout /t 3 > nul