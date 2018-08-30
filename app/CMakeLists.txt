cmake_minimum_required(VERSION 3.4.1)

set(DIR_OS ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI})
set(DIR_FFMPEG_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/ffmpeg)
set(DIR_DEMUX ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/demux)
set(DIR_COMMON ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/common)

# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DIR_OS})

include_directories(
        ${DIR_FFMPEG_INCLUDE}
        ${DIR_DEMUX}
        ${DIR_COMMON}
        )

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${DIR_OS}/libavcodec.so)

add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${DIR_OS}/libavfilter.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${DIR_OS}/libavformat.so)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${DIR_OS}/libavutil.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${DIR_OS}/libswresample.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${DIR_OS}/libswscale.so)

add_library(
             native-lib
             SHARED

             ${DIR_DEMUX}/demux_media.cpp

             src/main/cpp/native-lib.cpp )

target_link_libraries(
                       native-lib
                       avcodec avfilter avformat avutil swresample swscale
                       log )