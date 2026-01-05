project(common)

set(STATUS_RES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/AccessBridgeStatusWindow.rc)

add_library(${PROJECT_NAME} ${STATUS_RES_FILE})
add_library(rpad::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
  set(MSVC_TOOLSET_VERSION 142)
  add_compile_options(/W4 /WX /GS)
else()
  set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -pthread -Wl,-rpath-link,${RPATH},--as-needed")
endif()

target_sources(${PROJECT_NAME}
  PRIVATE
    AccessBridgeDebug.cpp
    AccessBridgeMessages.cpp
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Version.lib
  )
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    # UNICODE _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
  )

