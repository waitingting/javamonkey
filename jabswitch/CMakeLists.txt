project(jabswitch)

# 生成资源文件
configure_file(${VERSION_FILE} VersionInfo.rc @ONLY)

# 添加资源编译
set_source_files_properties(VersionInfo.rc PROPERTIES LANGUAGE RC)

add_executable(${PROJECT_NAME} jabswitch.cpp VersionInfo.rc)

if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
  set(MSVC_TOOLSET_VERSION 142)
  add_compile_options(/W4 /WX /GS)
else()
  set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -pthread -Wl,-rpath-link,${RPATH},--as-needed")
endif()

target_sources(${PROJECT_NAME}
  PRIVATE
    jabswitch.cpp
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Version.lib
  )
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    UNICODE _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
  )

