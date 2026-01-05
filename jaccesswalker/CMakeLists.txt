project(jaccesswalker)

# 生成资源文件
configure_file(${VERSION_FILE} VersionInfo.rc @ONLY)

# 添加资源编译
set_source_files_properties(VersionInfo.rc PROPERTIES LANGUAGE RC)

set(RES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/jaccesswalkerWindow.rc)
message(${RES_FILE})

add_executable(${PROJECT_NAME} WIN32 jaccesswalker.cpp ${RES_FILE})

if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
  set(MSVC_TOOLSET_VERSION 142)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup" ) 
  add_compile_options(/W4 /WX /GS)
else()
  set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -pthread -Wl,-rpath-link,${RPATH},--as-needed")
endif()

find_package(JNI REQUIRED)

target_sources(${PROJECT_NAME}
  PRIVATE
    jaccesswalker.cpp
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Version.lib
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