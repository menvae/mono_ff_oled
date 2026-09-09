cmake_minimum_required(VERSION 4.0)
project(mono_ff_oled CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(SDL3_ROOT "${CMAKE_SOURCE_DIR}/SDL3")
set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/ffmpeg")
list(APPEND CMAKE_PREFIX_PATH "${SDL3_ROOT}")

find_package(SDL3 3.2.0 REQUIRED CONFIG)

add_executable(mono_ff_oled main.cpp
        VideoDecoder.cpp
        VideoDecoder.h
        Preview.h
        utils.h
        SerialPort.h)

set_target_properties(mono_ff_oled PROPERTIES
        CXX_SCAN_FOR_MODULES OFF
)

target_compile_definitions(mono_ff_oled PRIVATE
        __STDC_CONSTANT_MACROS
        __STDC_LIMIT_MACROS
)

target_include_directories(mono_ff_oled PRIVATE "${FFMPEG_ROOT}/include")
target_link_directories(mono_ff_oled PRIVATE "${FFMPEG_ROOT}/lib")

target_link_libraries(mono_ff_oled PRIVATE
        SDL3::SDL3
        avformat
        avcodec
        swscale
        avutil
)

add_custom_command(TARGET mono_ff_oled POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${FFMPEG_ROOT}/bin"
        "$<TARGET_FILE_DIR:mono_ff_oled>"
        COMMENT "Copying FFmpeg DLLs to output directory..."
)

add_custom_command(TARGET mono_ff_oled POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_ROOT}/bin/SDL3.dll"
        "$<TARGET_FILE_DIR:mono_ff_oled>"
        COMMENT "Copying SDL3 DLL to output directory..."
)

add_custom_command(TARGET mono_ff_oled POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/examples"
        "$<TARGET_FILE_DIR:mono_ff_oled>"
        COMMENT "Copying Videos to output directory..."
)