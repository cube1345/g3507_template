@echo off
:: ✨ 每次运行前清空终端，保持清爽
cls
chcp 65001 >nul

:: 获取当前脚本所在的绝对路径
set BASE_DIR=%~dp0

:: ==========================================
:: 🚀 阶段 1：Keil 编译 (TI MSPM0G3507)
:: ==========================================
echo 🚀 [1/2] 正在唤醒 Keil 编译 TI 工程...

:: 1. Keil 软件路径
set KEIL_PATH="D:\Keil_v5-537\UV4\UV4.exe"

:: 2. 你的 MSPM0 工程路径
set PROJECT_PATH="%BASE_DIR%keil\MSPG3507_FreeRTOS.uvprojx"

:: 调用 Keil 进行后台编译 (-b 代表 Build)
%KEIL_PATH% -b %PROJECT_PATH% -j0 -o "%BASE_DIR%build_log.txt"

:: 打印编译日志
IF EXIST "%BASE_DIR%build_log.txt" (
    type "%BASE_DIR%build_log.txt"
) ELSE (
    echo ❌ 编译日志丢失，请检查工程名是否正确！
)

:: 🎯 核心防骗机制：检查 Keil 的返回值 (0:无错误无警告, 1:有警告无错误, 2及以上:有错误)
IF %ERRORLEVEL% GEQ 2 (
    echo.
    echo ❌ 编译失败，发现 Error！已取消烧录流程。
    exit /b %ERRORLEVEL%
)

echo.
:: ==========================================
:: ⚡ 阶段 2：Keil 极速烧录 (调用工程内置配置)
:: ==========================================
echo ⚡ [2/2] 正在呼叫 Keil 烧录 MSPM0 芯片...

:: 调用 Keil 进行后台烧录 (-f 代表 Flash)
%KEIL_PATH% -f %PROJECT_PATH% -j0 -o "%BASE_DIR%flash_log.txt"

:: 打印烧录日志
IF EXIST "%BASE_DIR%flash_log.txt" (
    type "%BASE_DIR%flash_log.txt"
) ELSE (
    echo ❌ 烧录日志丢失！
)

:: 检查烧录结果
IF %ERRORLEVEL% NEQ 0 (
    echo.
    echo ❌ 烧录失败！请检查 DAPLink 是否连接，或 Keil 工程里的 Debug 设置。
    exit /b %ERRORLEVEL%
)

echo.
echo 🎉 完美！TI MSPM0 编译与烧录一气呵成！板子已跑起！