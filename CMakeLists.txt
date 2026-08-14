cmake_minimum_required(VERSION 3.20)

project(VideoEditor
    VERSION 0.2.0
    LANGUAGES CXX
    DESCRIPTION "Cross-platform video editor (Linux + Windows) - Kdenlive-inspired architecture"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# ---- Find Qt6 ----------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Xml)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# ---- Find FFmpeg -------------------------------------------------------------
find_package(PkgConfig QUIET)
set(FFMPEG_FOUND FALSE)
if(PkgConfig_FOUND)
    pkg_check_modules(AVFORMAT IMPORTED_TARGET libavformat)
    pkg_check_modules(AVCODEC  IMPORTED_TARGET libavcodec)
    pkg_check_modules(AVUTIL   IMPORTED_TARGET libavutil)
    pkg_check_modules(SWSCALE  IMPORTED_TARGET libswscale)
    pkg_check_modules(SWRESAMPLE IMPORTED_TARGET libswresample)
    if(AVFORMAT_FOUND AND AVCODEC_FOUND AND AVUTIL_FOUND AND SWSCALE_FOUND AND SWRESAMPLE_FOUND)
        set(FFMPEG_FOUND TRUE)
        set(FFMPEG_LIBS PkgConfig::AVFORMAT PkgConfig::AVCODEC PkgConfig::AVUTIL
                        PkgConfig::SWSCALE PkgConfig::SWRESAMPLE)
    endif()
endif()

if(NOT FFMPEG_FOUND)
    set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/third_party/ffmpeg" CACHE PATH "Path to FFmpeg root")
    if(EXISTS "${FFMPEG_ROOT}/include")
        set(FFMPEG_INCLUDE_DIR "${FFMPEG_ROOT}/include")
        set(FFMPEG_LIB_DIR     "${FFMPEG_ROOT}/lib")
        set(FFMPEG_BIN_DIR     "${FFMPEG_ROOT}/bin" CACHE PATH "FFmpeg dll location")
        set(FFMPEG_FOUND TRUE)
        set(FFMPEG_LIBS
            "${FFMPEG_LIB_DIR}/avformat.lib"
            "${FFMPEG_LIB_DIR}/avcodec.lib"
            "${FFMPEG_LIB_DIR}/avutil.lib"
            "${FFMPEG_LIB_DIR}/swscale.lib"
            "${FFMPEG_LIB_DIR}/swresample.lib"
        )
        message(STATUS "FFmpeg found via manual root: ${FFMPEG_ROOT}")
    endif()
endif()

if(NOT FFMPEG_FOUND)
    message(FATAL_ERROR
        "FFmpeg not found. On Linux install libavformat-dev libavcodec-dev "
        "libavutil-dev libswscale-dev libswresample-dev; on Windows place a "
        "prebuilt FFmpeg under third_party/ffmpeg (include/ + lib/ + bin/).")
endif()

# ---- Application sources -----------------------------------------------------
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)
list(FILTER APP_SOURCES EXCLUDE REGEX "/main\\.cpp$")

qt_add_executable(VideoEditor
    src/main.cpp
    ${APP_SOURCES}
)

target_include_directories(VideoEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIR}
)

target_link_libraries(VideoEditor PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Xml
    ${FFMPEG_LIBS}
)

if(WIN32 AND FFMPEG_BIN_DIR AND EXISTS "${FFMPEG_BIN_DIR}")
    add_custom_command(TARGET VideoEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${FFMPEG_BIN_DIR}"
            "$<TARGET_FILE_DIR:VideoEditor>"
        COMMENT "Copying FFmpeg DLLs to output directory"
    )
endif()

qt_add_resources(APP_QRC resources/resources.qrc)
target_sources(VideoEditor PRIVATE ${APP_QRC})

install(TARGETS VideoEditor RUNTIME DESTINATION bin)

message(STATUS "== VideoEditor ${PROJECT_VERSION} (Kdenlive-inspired) ==")
message(STATUS "  Qt6 version: ${Qt6Widgets_VERSION}")
message(STATUS "  Compiler:    ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Platform:    ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
