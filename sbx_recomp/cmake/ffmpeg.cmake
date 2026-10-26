# Upstream FFmpeg (sbx_recomp/thirdparty/FFmpeg, a submodule pinned to the
# release ITC-Recomp uses, n9.0.2) with only what we need: the asf demuxer
# with the wmv3 and wmav2 decoders for the game's movies (ITC's
# docs/native-movies.md), the xma2 decoder for its XMA sounds (ITC's B2, later
# here), and for custom music (src/audio/music/) the ogg, flac, mp3 and wav
# demuxers, the vorbis, opus, flac, mp3 and PCM decoders, and libswresample.
# Built from the files configure made, first copied from ITC-Recomp into
# sbx_recomp/thirdparty/ffmpeg-config/<platform>/ (configure never runs here).
# The custom music components were added by running configure (n9.0.2, Linux)
# with the old and the new options and carrying the difference over to both
# platforms; the new options are in each config.h's FFMPEG_CONFIGURATION.
# Static libraries with hidden symbols: the SDK's own FFmpeg (Xenia's fork, in
# rexruntime) is separate and exports none of its symbols, so the two don't
# meet. Ported from ITC-Recomp's itc_recomp/cmake/ffmpeg.cmake.
#
# Targets: sbx_avutil, sbx_avcodec (links sbx_avutil), sbx_avformat
# (links sbx_avcodec) and sbx_swresample (links sbx_avutil). Named apart
# from the SDK's libavutil and libavcodec targets.

enable_language(C)

set(SBX_FFMPEG_DIR ${CMAKE_CURRENT_LIST_DIR}/../thirdparty/FFmpeg)
if(WIN32)
    set(SBX_FFMPEG_PLATFORM windows-x86_64)
else()
    set(SBX_FFMPEG_PLATFORM linux-x86_64)
endif()
set(SBX_FFMPEG_CONFIG ${CMAKE_CURRENT_LIST_DIR}/../thirdparty/ffmpeg-config/${SBX_FFMPEG_PLATFORM})
if(NOT EXISTS ${SBX_FFMPEG_DIR}/configure)
    message(FATAL_ERROR "FFmpeg submodule missing: git submodule update --init sbx_recomp/thirdparty/FFmpeg")
endif()

file(STRINGS ${SBX_FFMPEG_CONFIG}/sources.txt SBX_FFMPEG_SOURCES)
set(SBX_AVUTIL_SOURCES ${SBX_FFMPEG_SOURCES})
list(FILTER SBX_AVUTIL_SOURCES INCLUDE REGEX "^libavutil/")
set(SBX_AVCODEC_SOURCES ${SBX_FFMPEG_SOURCES})
list(FILTER SBX_AVCODEC_SOURCES INCLUDE REGEX "^libavcodec/")
set(SBX_AVFORMAT_SOURCES ${SBX_FFMPEG_SOURCES})
list(FILTER SBX_AVFORMAT_SOURCES INCLUDE REGEX "^libavformat/")
set(SBX_SWRESAMPLE_SOURCES ${SBX_FFMPEG_SOURCES})
list(FILTER SBX_SWRESAMPLE_SOURCES INCLUDE REGEX "^libswresample/")
# The generated lists are #included from the config directory (allcodecs.c
# and friends), not compiled on their own.
set(SBX_FFMPEG_LISTS "/(codec|parser|bsf|demuxer|muxer|protocol)_list\\.c$")
list(FILTER SBX_AVCODEC_SOURCES EXCLUDE REGEX ${SBX_FFMPEG_LISTS})
list(FILTER SBX_AVFORMAT_SOURCES EXCLUDE REGEX ${SBX_FFMPEG_LISTS})
list(TRANSFORM SBX_AVUTIL_SOURCES PREPEND ${SBX_FFMPEG_DIR}/)
list(TRANSFORM SBX_AVCODEC_SOURCES PREPEND ${SBX_FFMPEG_DIR}/)
list(TRANSFORM SBX_AVFORMAT_SOURCES PREPEND ${SBX_FFMPEG_DIR}/)
list(TRANSFORM SBX_SWRESAMPLE_SOURCES PREPEND ${SBX_FFMPEG_DIR}/)

add_library(sbx_avutil STATIC ${SBX_AVUTIL_SOURCES})
add_library(sbx_avcodec STATIC ${SBX_AVCODEC_SOURCES})
target_link_libraries(sbx_avcodec PUBLIC sbx_avutil)
add_library(sbx_avformat STATIC ${SBX_AVFORMAT_SOURCES})
target_link_libraries(sbx_avformat PUBLIC sbx_avcodec)
add_library(sbx_swresample STATIC ${SBX_SWRESAMPLE_SOURCES})
target_link_libraries(sbx_swresample PUBLIC sbx_avutil)

foreach(target sbx_avutil sbx_avcodec sbx_avformat sbx_swresample)
    # As FFmpeg's own build: the generated files first, then the sources.
    target_include_directories(${target} BEFORE PUBLIC ${SBX_FFMPEG_CONFIG} ${SBX_FFMPEG_DIR})
    # configure's CPPFLAGS and CFLAGS (ffbuild/config.mak), without its warnings.
    target_compile_definitions(${target} PRIVATE
        HAVE_AV_CONFIG_H _ISOC11_SOURCE _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE PIC)
    target_compile_options(${target} PRIVATE
        -std=c17 -fomit-frame-pointer -fno-math-errno -fno-signed-zeros -w)
    if(WIN32)
        target_compile_definitions(${target} PRIVATE
            WIN32_LEAN_AND_MEAN _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_WARNINGS _WIN32_WINNT=0x0600)
        target_include_directories(${target} PRIVATE ${SBX_FFMPEG_DIR}/compat/stdbit)
    else()
        target_compile_definitions(${target} PRIVATE _POSIX_C_SOURCE=200112 _XOPEN_SOURCE=600)
        target_compile_options(${target} PRIVATE -pthread)
    endif()
    set_target_properties(${target} PROPERTIES
        C_VISIBILITY_PRESET hidden
        POSITION_INDEPENDENT_CODE ON)
endforeach()

# configure's EXTRALIBS for them.
if(WIN32)
    target_link_libraries(sbx_avutil PUBLIC user32 bcrypt)
    target_link_libraries(sbx_avcodec PUBLIC ole32 user32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(sbx_avutil PUBLIC m Threads::Threads)
endif()
