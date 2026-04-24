cmake_minimum_required(VERSION 3.20)
project(MSPM0G3507_Editor C ASM)

# 💡 核心：定义宏，解决 DL_UART_... 找不到定义的问题
# 这些宏通常在 Keil 的 Options for Target -> C/C++ 选项卡中
add_definitions(-D__MSPM0G3507__)
add_definitions(-DMSPM0G3507)

# 💡 核心：把所有包含路径写全
# 注意：${CMAKE_CURRENT_SOURCE_DIR} 代表当前文件夹
# 新架构路径优先放在旧路径之前，避免同名兼容壳覆盖 MyDevices 中的真实设备类头文件。
include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/source/third_party/CMSIS/Core/Include"
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
        "${CMAKE_CURRENT_SOURCE_DIR}/Mid"
        "${CMAKE_CURRENT_SOURCE_DIR}/Mid/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Mid/portable"
        "${CMAKE_CURRENT_SOURCE_DIR}/MyUsed/MyApps"
        "${CMAKE_CURRENT_SOURCE_DIR}/MyUsed/MyCore"
        "${CMAKE_CURRENT_SOURCE_DIR}/MyUsed/MyPlatform"
        "${CMAKE_CURRENT_SOURCE_DIR}/MyUsed/MyDevices"
        "${CMAKE_CURRENT_SOURCE_DIR}/MyUsed/MyBoards"
        "${CMAKE_CURRENT_SOURCE_DIR}/MyUsed/MyFacades"
        "${CMAKE_CURRENT_SOURCE_DIR}/MyUsed/MyInts"
        "${CMAKE_CURRENT_SOURCE_DIR}/MyUsed/MyDrivers"
        "${CMAKE_CURRENT_SOURCE_DIR}/MyUsed/MyLibs"
        "${CMAKE_CURRENT_SOURCE_DIR}/MyUsed/MyBSP"
        "${CMAKE_CURRENT_SOURCE_DIR}/." # 包含根目录下的 ti_msp_dl_config.h
)

# 💡 核心：让 CLion 知道哪些文件需要索引
file(GLOB_RECURSE SOURCES
        "source/*.c"
        "Mid/*.c"
        "MyUsed/*.c"
        "*.c"
)

add_executable(${PROJECT_NAME} ${SOURCES})
