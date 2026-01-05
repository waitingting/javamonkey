cmake_minimum_required(VERSION 3.20)
project(jdk.accessibility)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(BUILD_SHARED_LIBS OFF)
set(VERSION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/VersionInfo.rc.in)
message(${VERSION_FILE})

include_directories(include/bridge)
include_directories(include/java.base/share/native/include)
include_directories(include/java.base/windows/native/include)
include_directories(common)
include_directories(toolscommon)

math(EXPR architecture "${CMAKE_SIZEOF_VOID_P} * 8" OUTPUT_FORMAT DECIMAL)
# 检测系统位数
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Compiling for 64-bit system")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "Compiling for 32-bit system")
else()
    message(WARNING "Unknown architecture")
endif()

add_subdirectory(bridge)
add_subdirectory(common)
add_subdirectory(toolscommon)

add_subdirectory(jabswitch)
add_subdirectory(javamonkey)
add_subdirectory(jaccesswalker)

