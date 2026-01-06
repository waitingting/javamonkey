project(WINDOWSACCESSBRIDGE)

# 检测系统位数
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(DLL_POSTFIX "-64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(DLL_POSTFIX "-32")
else()
  message(WARNING "Unknown architecture")
endif()

add_library(${PROJECT_NAME} SHARED AccessBridgeWindowsEntryPoints.cpp)
# add_library(rpad::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_options(${PROJECT_NAME} PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/WinAccessBridge.DEF")

# 设置输出路径（可选）
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME "${PROJECT_NAME}${DLL_POSTFIX}"  # 关键设置
)

if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
  set(MSVC_TOOLSET_VERSION 142)
  add_compile_options(/W4 /WX /GS)
else()
  set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -pthread -Wl,-rpath-link,${RPATH},--as-needed")
endif()

target_sources(${PROJECT_NAME}
  PRIVATE
    AccessBridgeEventHandler.cpp
    AccessBridgeJavaVMInstance.cpp
    AccessBridgeMessageQueue.cpp
    AccessBridgeWindowsEntryPoints.cpp
    WinAccessBridge.cpp
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    rpad::common
  )

target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(${PROJECT_NAME} PRIVATE "/Zc:wchar_t-" /W4 /WX- /GS)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    # UNICODE _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
  )



