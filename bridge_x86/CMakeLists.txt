cmake_minimum_required(VERSION 3.20)
project(WindowsAccessBridge)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(BUILD_SHARED_LIBS OFF)
set(JAVA_HOME "C:/workspace/1.tools/javatools/jdk/jdk-8u461-windows-x64/jdk1.8.0_461")
add_library(${PROJECT_NAME}
WinAccessBridge.cpp
${RES_FILE}
)

# set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
# set(MSVC_TOOLSET_VERSION 142)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(${PROJECT_NAME} PRIVATE "/Zc:wchar_t-" /W4 /WX)
target_compile_options(${PROJECT_NAME} PRIVATE /wd4505 /wd4706 /wd4702 /wd4100 /wd4456)

find_package(Java COMPONENTS Runtime REQUIRED)
find_package(JNI REQUIRED)


target_sources(${PROJECT_NAME}
  PRIVATE
    # AccessInfo.cpp
    # JavaMonkey.cpp
    # AccessBridgeCalls.cc
    # AccessBridgeDebug.cpp
    # utils.cc
    # highlighter.cc
    # string_win.cc
    # misc_win.cc
    WinAccessBridge.cpp
    AccessBridgeEventHandler.cpp
    AccessBridgeJavaVMInstance.cpp
    AccessBridgeMessageQueue.cpp
    AccessBridgeMessages.cpp
    AccessBridgeJavaEntryPoints.cpp
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${JNI_INCLUDE_DIRS}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        )

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        user32.lib           # Windows 窗口管理库（必须，WinMain 依赖）
        gdi32.lib            # 图形设备接口库（如需绘图）
        advapi32.lib         # 高级 API（如注册表操作）
        shell32.lib          # Shell 操作（如打开文件对话框）
        winmm.lib
        comctl32.lib
        )
