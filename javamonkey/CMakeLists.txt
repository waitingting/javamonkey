project(javamonkey)
set(RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(RES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/monkeyWindow.RC)
# set(JAVA_HOME "C:/workspace/1.tools/javatools/jdk/jdk-8u461-windows-x64/jdk1.8.0_461")
set(JAB_HOME ${RESOURCE_DIR}/accessbridge2_0_2)
add_executable(${PROJECT_NAME} WIN32
JavaMonkey.cpp
${RES_FILE}
)

# add_library(${PROJECT_NAME})
# add_library(rpad::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# 设置输出路径（可选）
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)


if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
  set(MSVC_TOOLSET_VERSION 142)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup" ) 
  # add_compile_definitions(_USE_64BIT_TIME_T)
  add_compile_options(/W4 /WX /GS)
else()
  set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -pthread -Wl,-rpath-link,${RPATH},--as-needed")
endif()

if (NOT DEFINED JAB_HOME)
  message(SEND_ERROR "JAB_HOME not set, please set to java accessbridge directory.")
endif()

find_package(Java COMPONENTS Runtime REQUIRED)
find_package(JNI)


target_sources(${PROJECT_NAME}
  PRIVATE
    JavaMonkey.cpp
    utils.cc
    highlighter.cc
    string_win.cc
    misc_win.cc
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
    rpad::bridge
    rpad::toolscommon
    rpad::common
    )

target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        # UNICODE _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        )

math(EXPR architecture "${CMAKE_SIZEOF_VOID_P} * 8" OUTPUT_FORMAT DECIMAL)
target_compile_definitions(${PROJECT_NAME} PRIVATE ACCESSBRIDGE_ARCH_${architecture})

