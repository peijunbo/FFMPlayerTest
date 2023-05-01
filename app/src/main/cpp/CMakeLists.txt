cmake_minimum_required(VERSION 3.22.1)

project("player")

include_directories(
        ./include
)
link_directories(
        ../jniLibs/${CMAKE_ANDROID_ARCH_ABI}
)

aux_source_directory(${CMAKE_SOURCE_DIR}/src SRC_LIST)

set(ffmpeg-libs
        avformat
        avcodec
        avfilter
        swresample
        swscale
        avutil
        )
set(ndk-libs
        log
        nativewindow
        OpenSLES
        android)
add_library(
        player

        SHARED

        ${SRC_LIST})
# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        player

        ${ffmpeg-libs}
        # Links the target library to the log library
        # included in the NDK.
        ${ndk-libs})