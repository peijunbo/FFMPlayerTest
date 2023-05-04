cmake_minimum_required(VERSION 3.22.1)

project("ffmpeg-player")
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
        EGL
        GLESv3
        android)
include_directories(
        ./include
        ./glm
        ./utils
)
link_directories(
        ../jniLibs/${CMAKE_ANDROID_ARCH_ABI}
)
link_libraries(
        ${ndk-libs}
        ${ffmpeg-libs}
)
add_subdirectory(player)
add_subdirectory(utils)

aux_source_directory(. SRC_LIST)
add_library(
        ffmpeg-player

        SHARED

        ${SRC_LIST})


target_include_directories(
        ffmpeg-player
        PRIVATE
        ./player/include
)
target_link_libraries( # Specifies the target library.
        ffmpeg-player
        player
        utils)