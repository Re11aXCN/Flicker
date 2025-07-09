@echo off
setlocal enabledelayedexpansion

:: 使用系统环境变量中的VCPKG_PATH
if "%VCPKG_PATH%"=="" (
    echo Error: VCPKG_PATH environment variable is not defined.
    echo Please set VCPKG_PATH to your vcpkg installation directory.
    exit /b 1
)

:: 设置工具路径
set PROTOC="%VCPKG_PATH%\packages\protobuf_x64-windows\tools\protobuf\protoc.exe"
set GRPC_PLUGIN="%VCPKG_PATH%\packages\grpc_x64-windows\tools\grpc\grpc_cpp_plugin.exe"

:: 检查工具是否存在
if not exist %PROTOC% (
    echo Error: protoc.exe not found at %PROTOC%
    exit /b 1
)
if not exist %GRPC_PLUGIN% (
    echo Error: grpc_cpp_plugin.exe not found at %GRPC_PLUGIN%
    exit /b 1
)

:: 处理当前目录下所有.proto文件
echo Processing all .proto files in current directory...
for %%f in (*.proto) do (
    echo Generating code for %%~nxf...
    
    :: 生成gRPC代码
    %PROTOC% -I="." --grpc_out="." --plugin=protoc-gen-grpc=%GRPC_PLUGIN% "%%f"
    if errorlevel 1 (
        echo Error generating gRPC code for %%~nxf
        exit /b 1
    )
    
    :: 生成Protobuf代码
    %PROTOC% --cpp_out=. "%%f"
    if errorlevel 1 (
        echo Error generating Protobuf code for %%~nxf
        exit /b 1
    )
    
    echo Successfully processed %%~nxf
)

echo All .proto files processed successfully!
endlocal