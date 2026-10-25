# sbx - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(sbx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Native Vulkan renderer (src/native/, from ITC-Recomp; work in progress, see
# docs/native-renderer-port.md). Off by default: the emulated GPU draws.
option(SBX_NATIVE_RENDERER "Native Vulkan renderer (work in progress)" OFF)

# Apply this project's patches to the rexglue-sdk submodule, skipping any
# already applied. See patches/rexglue-sdk/ for what each one works around.
if(REXSDK_DIR)
    file(GLOB SBX_SDK_PATCHES "${CMAKE_CURRENT_SOURCE_DIR}/../patches/rexglue-sdk/*.patch")
    list(SORT SBX_SDK_PATCHES)
    if(SBX_SDK_PATCHES)
        # Patches touching nearby lines can't each be checked alone once a
        # later one is applied. They're always applied in order, so if the
        # last one is applied, they all are.
        list(GET SBX_SDK_PATCHES -1 last_patch)
        execute_process(
            COMMAND git -C "${REXSDK_DIR}" apply --reverse --check "${last_patch}"
            RESULT_VARIABLE all_applied OUTPUT_QUIET ERROR_QUIET)
        if(all_applied EQUAL 0)
            set(SBX_SDK_PATCHES "")
        endif()
    endif()
    foreach(patch IN LISTS SBX_SDK_PATCHES)
        execute_process(
            COMMAND git -C "${REXSDK_DIR}" apply --reverse --check "${patch}"
            RESULT_VARIABLE already_applied OUTPUT_QUIET ERROR_QUIET)
        if(NOT already_applied EQUAL 0)
            execute_process(
                COMMAND git -C "${REXSDK_DIR}" apply "${patch}"
                RESULT_VARIABLE apply_result)
            if(NOT apply_result EQUAL 0)
                message(FATAL_ERROR "Failed to apply SDK patch ${patch}")
            endif()
            message(STATUS "Applied SDK patch ${patch}")
        endif()
    endforeach()
endif()

# ThinLTO, Linux (ld.lld) and Windows (lld-link) take different link options.
option(SBX_LTO "Linux: build with ThinLTO (needs lld)" OFF)
if(SBX_LTO AND NOT WIN32)
    add_compile_options(-flto=thin)
    add_link_options(-flto=thin -fuse-ld=lld
        -Wl,--thinlto-cache-dir=${CMAKE_BINARY_DIR}/thinlto-cache)
endif()
option(SBX_WIN_LTO "Windows: build with ThinLTO (lld-link)" OFF)
if(WIN32 AND SBX_WIN_LTO)
    add_compile_options(-flto=thin)
    add_link_options(-flto=thin "LINKER:/lldltocache:${CMAKE_BINARY_DIR}/thinlto-cache")
endif()

# The SDL patches (patches/rexglue-sdk-native/), applied in both builds, each
# skipped if already applied: SDL as a DLL on Windows, so the game and
# rexruntime.dll share one SDL (our audio output, src/audio/, and the native
# renderer call it), with SDL's Vulkan support on.
if(REXSDK_DIR)
    file(GLOB SBX_NATIVE_SDK_PATCHES "${CMAKE_CURRENT_SOURCE_DIR}/../patches/rexglue-sdk-native/*.patch")
    list(SORT SBX_NATIVE_SDK_PATCHES)
    foreach(patch IN LISTS SBX_NATIVE_SDK_PATCHES)
        execute_process(
            COMMAND git -C "${REXSDK_DIR}" apply --reverse --check "${patch}"
            RESULT_VARIABLE already_applied OUTPUT_QUIET ERROR_QUIET)
        if(NOT already_applied EQUAL 0)
            execute_process(
                COMMAND git -C "${REXSDK_DIR}" apply "${patch}"
                RESULT_VARIABLE apply_result)
            if(NOT apply_result EQUAL 0)
                message(FATAL_ERROR "Failed to apply SDK patch ${patch}")
            endif()
            message(STATUS "Applied SDK patch ${patch}")
        endif()
    endforeach()
endif()

# The native renderer and the shader translator need the SDK's Vulkan
# headers, which a D3D12 build leaves out (the SDK defaults to D3D12 on
# Windows). Forced while the option is on; with it off again, delete the
# build folder to go back to the SDK's defaults.
if(SBX_NATIVE_RENDERER)
    set(REXGLUE_USE_VULKAN ON CACHE BOOL "" FORCE)
    set(REXGLUE_USE_D3D12 OFF CACHE BOOL "" FORCE)
endif()

include(generated/rexglue.cmake)

# Sources
set(SBX_SOURCES
    src/main.cpp
    src/audio/native_audio.cpp
    src/audio/output_mix.cpp
    src/debug/frame_state_diff.cpp
    src/hooks/create_file.cpp
    src/hooks/cut_car_wheels.cpp
    src/hooks/lights.cpp
    src/hooks/patch_loader.cpp
    src/hooks/post_effects.cpp
    src/hooks/post_race.cpp
    src/hooks/shader_compiles.cpp
    src/hooks/title_screen.cpp
    src/hooks/unlock_cut_cars.cpp
    src/hooks/voice.cpp
    src/hooks/xma.cpp
    src/subtick/car_transforms.cpp
    src/subtick/frame_pacer.cpp
    src/subtick/game_messages.cpp
    src/subtick/input.cpp
    src/subtick/input_weights.cpp
    src/subtick/per_frame_logic.cpp
    src/subtick/renderer_hooks.cpp
    src/tick_rate/tick_rate.cpp
    src/resources/app.rc
)
if(SBX_NATIVE_RENDERER)
    list(APPEND SBX_SOURCES
        src/base/log.cpp
        src/base/log_sdk.cpp
        src/helpers/executable.cpp
        src/native/app_window.cpp
        src/native/buffers.cpp
        src/native/disk_cache.cpp
        src/native/draw_packets.cpp
        src/native/gpu_link.cpp
        src/native/passes.cpp
        src/native/renderer.cpp
        src/native/replay_hooks.cpp
        src/native/shaders.cpp
        src/native/state.cpp
        src/native/textures.cpp
        src/native/vulkan_functions.cpp
        src/movies/movie.cpp
        src/hooks/movies.cpp
    )
endif()
# Hooks in src/hooks/ override generated functions (docs/functions.md).

if(WIN32)
    add_executable(sbx WIN32 ${SBX_SOURCES})
else()
    add_executable(sbx ${SBX_SOURCES})
endif()

# The emulated GPU (the xenos plugin, loaded with --gpu_plugin=xenos). Still
# built with the native renderer, for comparisons; leave --gpu_plugin unset
# to run the native renderer alone.
rexglue_setup_target(sbx GPU_PLUGINS xenos)

target_include_directories(sbx PRIVATE src)

# Upstream FFmpeg from sbx_recomp/thirdparty, built with ITC-Recomp's configs
# (cmake/ffmpeg.cmake), in both builds: our XMA decoding (src/audio/xma_*,
# src/hooks/xma.cpp; ITC's B2) uses its xma2 decoder, in a library of its own
# so only our FFmpeg's headers reach it (the SDK has its own FFmpeg).
include(cmake/ffmpeg.cmake)
add_library(sbx_xma STATIC src/audio/xma_packets.cpp src/audio/xma_stream.cpp)
target_include_directories(sbx_xma PUBLIC src)
target_link_libraries(sbx_xma PRIVATE sbx_avcodec)
target_link_libraries(sbx PRIVATE sbx_xma)

if(SBX_NATIVE_RENDERER)
    # The SDK's Xenos shader translator (to SPIR-V), built into our binary for
    # the native renderer, as in ITC-Recomp (the plugin has its own copy with
    # hidden symbols).
    set(SBX_SDK_GRAPHICS ${REXSDK_DIR}/src/graphics)
    add_library(sbx_xenos_shaders STATIC
        ${SBX_SDK_GRAPHICS}/pipeline/shader/shader.cpp
        ${SBX_SDK_GRAPHICS}/pipeline/shader/translator.cpp
        ${SBX_SDK_GRAPHICS}/pipeline/shader/translator_disasm.cpp
        ${SBX_SDK_GRAPHICS}/pipeline/shader/spirv_builder.cpp
        ${SBX_SDK_GRAPHICS}/pipeline/shader/spirv.cpp
        ${SBX_SDK_GRAPHICS}/pipeline/shader/spirv_translator.cpp
        ${SBX_SDK_GRAPHICS}/pipeline/shader/spirv_translator_alu.cpp
        ${SBX_SDK_GRAPHICS}/pipeline/shader/spirv_translator_fetch.cpp
        ${SBX_SDK_GRAPHICS}/pipeline/shader/spirv_translator_memexport.cpp
        ${SBX_SDK_GRAPHICS}/pipeline/shader/spirv_translator_rb.cpp
        ${SBX_SDK_GRAPHICS}/format/ucode.cpp
        ${SBX_SDK_GRAPHICS}/util/draw.cpp
        ${SBX_SDK_GRAPHICS}/registers.cpp
        ${SBX_SDK_GRAPHICS}/register_file.cpp
    )
    target_include_directories(sbx_xenos_shaders PUBLIC ${REXSDK_DIR}/thirdparty/renderdoc)
    set_target_properties(sbx_xenos_shaders PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
    target_compile_options(sbx_xenos_shaders PRIVATE -Wno-unused-variable)
    target_link_libraries(sbx_xenos_shaders PUBLIC
        rex::runtime xxHash::xxhash Vulkan::Headers glslang::SPIRV glslang
        MachineIndependent GenericCodeGen OSDependent OGLCompiler)

    target_link_libraries(sbx PRIVATE sbx_xenos_shaders)

    # The movies played through our own FFmpeg (src/movies/, from ITC-Recomp;
    # its docs/native-movies.md). The player is its own library, as ITC's
    # itc_movies, so only it sees FFmpeg's headers.
    add_library(sbx_movies STATIC src/movies/player.cpp)
    target_include_directories(sbx_movies PUBLIC src)
    target_link_libraries(sbx_movies PRIVATE sbx_avformat)
    target_link_libraries(sbx PRIVATE sbx_movies)
    target_compile_definitions(sbx PRIVATE SBX_NATIVE_RENDERER=1)
    # Vulkan (loaded at runtime): headers only, the SDK's third-party copy;
    # xxHash header-only for src/base/hash.h.
    target_include_directories(sbx PRIVATE
        ${REXSDK_DIR}/thirdparty/vulkan-headers/include
        ${REXSDK_DIR}/thirdparty/xxHash)
endif()

# SDL (SDL3.dll, shared with rexruntime): the SDK's headers, for our audio
# output (src/audio/) and the native renderer.
target_include_directories(sbx PRIVATE ${REXSDK_DIR}/thirdparty/sdl3/include)

# rex::runtime doesn't propagate imgui's include path, which rex_app.cpp and
# the app header need. Headers only: the symbols already live in rexruntime.
target_include_directories(sbx PRIVATE
    $<TARGET_PROPERTY:imgui::imgui,INTERFACE_INCLUDE_DIRECTORIES>)
