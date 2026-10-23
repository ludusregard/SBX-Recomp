# sbx - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(sbx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

include(generated/rexglue.cmake)

# Sources
set(SBX_SOURCES
    src/main.cpp
    src/hooks/cut_car_wheels.cpp
    src/hooks/patch_loader.cpp
    src/hooks/post_race.cpp
    src/hooks/unlock_cut_cars.cpp
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
# Hooks in src/hooks/ override generated functions (docs/functions.md).

if(WIN32)
    add_executable(sbx WIN32 ${SBX_SOURCES})
else()
    add_executable(sbx ${SBX_SOURCES})
endif()

# The emulated GPU (the xenos plugin, loaded with --gpu_plugin=xenos).
rexglue_setup_target(sbx GPU_PLUGINS xenos)

target_include_directories(sbx PRIVATE src)

# rex::runtime doesn't propagate imgui's include path, which rex_app.cpp and
# the app header need. Headers only: the symbols already live in rexruntime.
target_include_directories(sbx PRIVATE
    $<TARGET_PROPERTY:imgui::imgui,INTERFACE_INCLUDE_DIRECTORIES>)
