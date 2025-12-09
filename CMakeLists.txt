cmake_minimum_required(VERSION 3.20)
project(JavaMonkey)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(BUILD_SHARED_LIBS OFF)
set(RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(RES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/monkeyWindow.RC)
set(JAVA_HOME "C:/jdk11")
set(JAB_HOME ${RESOURCE_DIR}/accessbridge2_0_2)
add_executable(${PROJECT_NAME} WIN32
JavaMonkey.cpp
${RES_FILE}
)

# add_library(${PROJECT_NAME})
# add_library(rpad::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

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
find_package(JNI REQUIRED)


target_sources(${PROJECT_NAME}
  PRIVATE
    AccessInfo.cpp
    JavaMonkey.cpp
    AccessBridgeCalls.cc
    AccessBridgeDebug.cpp
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
        )

target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        # UNICODE _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        )

math(EXPR architecture "${CMAKE_SIZEOF_VOID_P} * 8" OUTPUT_FORMAT DECIMAL)
target_compile_definitions(${PROJECT_NAME} PRIVATE ACCESSBRIDGE_ARCH_${architecture})

function(CopyJavaAccessBridge)
  include(CMakeParseArguments)
  set(_options)
  set(_singleargs TARGET)
  set(_multiargs)
  cmake_parse_arguments(CopyJavaAccessBridge "${_options}" "${_singleargs}" "${_multiargs}" "${ARGN}")

  if(NOT CopyJavaAccessBridge_TARGET)
    message(SEND_ERROR "Error: CopyJavaAccessBridge called without any targets")
    return()
  endif()

  math(EXPR architecture "${CMAKE_SIZEOF_VOID_P} * 8" OUTPUT_FORMAT DECIMAL)

  file(TO_CMAKE_PATH ${JAB_HOME} JAB_HOME)
  add_custom_command(TARGET ${CopyJavaAccessBridge_TARGET}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${JAB_HOME}/WindowsAccessBridge-${architecture}.dll $<TARGET_FILE_DIR:${CopyJavaAccessBridge_TARGET}>
        WORKING_DIRECTORY ${JAB_HOME}
        COMMENT "Copy WindowsAccessBridge-${architecture}.dll to binary directory."
        VERBATIM
  )
  install(FILES ${JAB_HOME}/WindowsAccessBridge-${architecture}.dll DESTINATION bin)
endfunction()
