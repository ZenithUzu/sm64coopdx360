cmake_minimum_required(VERSION 3.25)
project(sm64coopdx360_platform C CXX)
if(NOT XSDK)
    message(FATAL_ERROR "Use -DCMAKE_TOOLCHAIN_FILE=<xsdk>/cmake/xsdk.cmake")
endif()
get_filename_component(COOPDX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(SDL3_DIR "C:/SDL-xenon/build-xsdk" CACHE PATH "Built SDL-xenon package")
find_package(SDL3 CONFIG REQUIRED)
add_library(coopdx_xsdk_platform STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/posix_xsdk.c
    ${CMAKE_CURRENT_SOURCE_DIR}/keyboard_xsdk.c
    ${CMAKE_CURRENT_SOURCE_DIR}/input_usb.c
    ${COOPDX_ROOT}/src/pc/audio/audio_sdl3.c
    ${COOPDX_ROOT}/src/pc/controller/controller_xsdk.c
    ${COOPDX_ROOT}/src/pc/gfx/gfx_xsdk.c
    ${COOPDX_ROOT}/src/pc/gfx/gfx_xenos.c
    ${COOPDX_ROOT}/src/pc/gfx/gfx_cc.c)
target_include_directories(coopdx_xsdk_platform PUBLIC
    ${COOPDX_ROOT}/include ${COOPDX_ROOT}/src ${COOPDX_ROOT} ${COOPDX_ROOT}/lib/xsdk/lua53 ${COOPDX_ROOT}/sound
    ${COOPDX_ROOT}/src/pc/audio ${COOPDX_ROOT}/src/pc/controller ${COOPDX_ROOT}/src/pc/gfx)
target_compile_definitions(coopdx_xsdk_platform PUBLIC
    TARGET_XBOX360=1 _LANGUAGE_C NON_MATCHING=1 AVOID_UB=1 VERSION_US=1 F3DEX_GBI_2E=1)
target_compile_features(coopdx_xsdk_platform PRIVATE c_std_17)
target_compile_options(coopdx_xsdk_platform PRIVATE -Wall -Wextra -Werror)
target_link_libraries(coopdx_xsdk_platform PUBLIC SDL3::SDL3-static)
# Real engine translation units: establishes PowerPC compile compatibility,
# not a standalone engine executable and not a replacement for the game build.
add_library(coopdx_xsdk_engine OBJECT
    ${COOPDX_ROOT}/src/engine/math_util.c
    ${COOPDX_ROOT}/src/engine/graph_node.c
    ${COOPDX_ROOT}/src/engine/geo_layout.c
    ${COOPDX_ROOT}/src/pc/controller/controller_entry_point.c)
target_include_directories(coopdx_xsdk_engine PRIVATE
    ${COOPDX_ROOT} ${COOPDX_ROOT}/lib/xsdk/lua53)

target_link_libraries(coopdx_xsdk_engine PRIVATE coopdx_xsdk_platform)


set(COOPDX_PYTHON "C:/msys64/ucrt64/bin/python.exe" CACHE FILEPATH "Host Python")
set(COOPDX_GLSLANG "${XSDK_TOOLCHAIN}/bin/glslang.exe" CACHE FILEPATH "Host glslang-xenon")
set(COOPDX_SHADER_HEADER "${CMAKE_CURRENT_BINARY_DIR}/coopdx_shaders.h")
add_custom_command(OUTPUT "${COOPDX_SHADER_HEADER}"
    COMMAND "${COOPDX_PYTHON}" "${XSDK_ROOT}/tools/build_shaders.py"
        --glslang "${COOPDX_GLSLANG}" --output "${COOPDX_SHADER_HEADER}"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/coopdx.vert" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/coopdx.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/upscale.vert" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/upscale.frag"
    DEPENDS shaders/coopdx.vert shaders/coopdx.frag shaders/upscale.vert shaders/upscale.frag "${XSDK_ROOT}/tools/build_shaders.py" "${XSDK_ROOT}/build/host/libxsc.dll"
    VERBATIM)
target_sources(coopdx_xsdk_platform PRIVATE "${COOPDX_SHADER_HEADER}")
target_include_directories(coopdx_xsdk_platform PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
set(COOPDX_MATERIAL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/materials/material_shaders.h")
add_custom_command(OUTPUT "${COOPDX_MATERIAL_HEADER}"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/materials/material_code.h"
    COMMAND "${COOPDX_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/build_material_shaders.py"
        --repo "${COOPDX_ROOT}" --out "${CMAKE_CURRENT_BINARY_DIR}/materials"
        --xsdk "${XSDK_ROOT}" --glslang "${COOPDX_GLSLANG}"
    DEPENDS build_material_shaders.py shaders/coopdx.frag "${COOPDX_ROOT}/src/pc/gfx/gfx_cc.c"
        "${XSDK_ROOT}/tools/build_shaders.py" "${XSDK_ROOT}/build/host/libxsc.dll"
    VERBATIM)
target_sources(coopdx_xsdk_platform PRIVATE "${COOPDX_MATERIAL_HEADER}")
target_include_directories(coopdx_xsdk_platform PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/materials")


file(GLOB LUA_SOURCES CONFIGURE_DEPENDS "${COOPDX_ROOT}/lib/xsdk/lua53/*.c")
list(FILTER LUA_SOURCES EXCLUDE REGEX "/(lua|ltests)\\.c$")
add_library(coopdx_lua STATIC ${LUA_SOURCES})
target_include_directories(coopdx_lua PUBLIC "${COOPDX_ROOT}/lib/xsdk/lua53")
set(ZLIB_ROOT "${COOPDX_ROOT}/lib/xsdk/zlib")
add_library(coopdx_zlib STATIC
    ${ZLIB_ROOT}/adler32.c ${ZLIB_ROOT}/compress.c ${ZLIB_ROOT}/crc32.c
    ${ZLIB_ROOT}/deflate.c ${ZLIB_ROOT}/infback.c ${ZLIB_ROOT}/inffast.c
    ${ZLIB_ROOT}/inflate.c ${ZLIB_ROOT}/inftrees.c ${ZLIB_ROOT}/trees.c
    ${ZLIB_ROOT}/uncompr.c ${ZLIB_ROOT}/zutil.c)
target_include_directories(coopdx_zlib PUBLIC "${ZLIB_ROOT}")
file(GLOB JUICE_SOURCES CONFIGURE_DEPENDS "${COOPDX_ROOT}/lib/xsdk/libjuice/src/*.c")
add_library(coopdx_juice STATIC ${JUICE_SOURCES})
target_compile_definitions(coopdx_juice PUBLIC JUICE_STATIC NO_IFADDRS)
target_include_directories(coopdx_juice PUBLIC "${COOPDX_ROOT}/lib/xsdk/libjuice/include" PRIVATE "${COOPDX_ROOT}/lib/xsdk/libjuice/include/juice")
file(GLOB COOPNET_SOURCES CONFIGURE_DEPENDS "${COOPDX_ROOT}/lib/xsdk/coopnet/*.cpp")
add_library(coopdx_coopnet STATIC ${COOPNET_SOURCES})
target_compile_features(coopdx_coopnet PRIVATE cxx_std_17)
target_include_directories(coopdx_coopnet PUBLIC "${COOPDX_ROOT}/lib/coopnet/include" PRIVATE "${COOPDX_ROOT}/src")
target_link_libraries(coopdx_coopnet PUBLIC coopdx_juice)

# Compile every engine source with the real target ABI, beyond the initial probes.
file(GLOB ENGINE_SOURCES CONFIGURE_DEPENDS "${COOPDX_ROOT}/src/engine/*.c")
target_sources(coopdx_xsdk_engine PRIVATE ${ENGINE_SOURCES})

option(COOPDX_BUILD_GAME "Build the complete Xbox 360 game target" OFF)
if(COOPDX_BUILD_GAME)
    set(GAME_DIRS src src/engine src/game src/audio src/menu src/buffers actors levels bin data assets lib sound
        src/pc src/pc/gfx src/pc/audio src/pc/controller src/pc/fs src/pc/fs/packtypes
        src/pc/mods src/pc/dev src/pc/network src/pc/network/packets src/pc/network/socket
        src/pc/network/coopnet src/pc/utils src/pc/utils/miniz src/pc/djui src/pc/lua
        src/pc/lua/utils src/pc/os src/pc/mumble src/goddard src/goddard/dynlists)
    set(GAME_SOURCES)
    foreach(dir IN LISTS GAME_DIRS)
        file(GLOB files CONFIGURE_DEPENDS "${COOPDX_ROOT}/${dir}/*.c" "${COOPDX_ROOT}/${dir}/*.cpp")
        list(APPEND GAME_SOURCES ${files})
    endforeach()
    file(GLOB level_files CONFIGURE_DEPENDS "${COOPDX_ROOT}/levels/*/leveldata.c" "${COOPDX_ROOT}/levels/*/script.c" "${COOPDX_ROOT}/levels/*/geo.c")
    list(APPEND GAME_SOURCES ${level_files})
    list(REMOVE_ITEM GAME_SOURCES "${COOPDX_ROOT}/src/game/main.c")
    list(FILTER GAME_SOURCES EXCLUDE REGEX "/(gfx_opengl|gfx_sdl|gfx_dxgi|gfx_direct3d11|gfx_direct3d_common|gfx_xsdk|gfx_xenos|gfx_cc|audio_sdl|audio_sdl3|controller_sdl|controller_xsdk|socket_windows)\\.(c|cpp)$")
    foreach(source alBnkfNew guLookAtRef guMtxF2L guNormalize guOrthoF guPerspectiveF guRotateF guScaleF guTranslateF ldiv)
        list(APPEND GAME_SOURCES "${COOPDX_ROOT}/lib/src/${source}.c")
    endforeach()
    set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
    add_executable(sm64coopdx ${GAME_SOURCES} "${GENERATED}/assets/mario_anim_data.c" "${GENERATED}/assets/demo_data.c")
    set_source_files_properties("${GENERATED}/assets/mario_anim_data.c" "${GENERATED}/assets/demo_data.c" PROPERTIES GENERATED TRUE)
    target_include_directories(sm64coopdx BEFORE PRIVATE "${GENERATED}" "${GENERATED}/include")
    target_include_directories(sm64coopdx PRIVATE "${COOPDX_ROOT}/lib/coopnet/include")
    # Long file names on FATX (fatx_names.c): every file call of the game, newlib and xSDK goes through it.
    target_sources(sm64coopdx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fatx_names.c)
    target_link_options(sm64coopdx PRIVATE -Wl,--wrap=open,--wrap=stat,--wrap=mkdir,--wrap=unlink,--wrap=access,--wrap=rename,--wrap=opendir,--wrap=readdir,--wrap=closedir)
    target_compile_options(sm64coopdx PRIVATE -fno-strict-aliasing -include "${CMAKE_CURRENT_SOURCE_DIR}/compat.h" "$<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types>" "$<$<COMPILE_LANGUAGE:C>:-Wno-int-conversion>")
    target_compile_definitions(sm64coopdx PRIVATE STB_VORBIS_BIG_ENDIAN COOPNET=1 DYNOS=1 ENHANCE_LEVEL_TEXTURES=1 MA_NO_RUNTIME_LINKING MA_NO_DEVICE_IO MINIZ_NO_TIME)
    set_source_files_properties("${COOPDX_ROOT}/src/audio/seqplayer.c" PROPERTIES COMPILE_DEFINITIONS BITS_32=1)
    target_compile_features(sm64coopdx PRIVATE cxx_std_17)
    target_link_libraries(sm64coopdx PRIVATE coopdx_xsdk_platform coopdx_lua coopdx_zlib coopdx_coopnet)
    # Asset headers may shadow the PC headers; model their dependency explicitly.
    add_custom_command(OUTPUT "${GENERATED}/assets.stamp"
        "${GENERATED}/assets/mario_anim_data.c" "${GENERATED}/assets/demo_data.c"
        "${GENERATED}/sound/samples_offsets.h" "${GENERATED}/sound/sequences_offsets.h"
        COMMAND "${COOPDX_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_assets.py"
            --repo "${COOPDX_ROOT}" --out "${GENERATED}" --compiler "${CMAKE_C_COMPILER}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/prepare_assets.py" "${COOPDX_ROOT}/tools/assemble_sound.py"
            "${COOPDX_ROOT}/sound/sequences.json" "${COOPDX_ROOT}/assets/demo_data.json"
        VERBATIM)
    add_custom_target(coopdx_assets DEPENDS "${GENERATED}/assets.stamp")
    add_dependencies(sm64coopdx coopdx_assets)
    set_source_files_properties("${COOPDX_ROOT}/sound/samples_assets.c" PROPERTIES OBJECT_DEPENDS "${GENERATED}/sound/samples_offsets.h")
    set_source_files_properties("${COOPDX_ROOT}/sound/sequences_assets.c" PROPERTIES OBJECT_DEPENDS "${GENERATED}/sound/sequences_offsets.h")

    # Explicit host Python avoids Windows App Execution Alias resolution.
    add_custom_command(TARGET sm64coopdx POST_BUILD COMMAND "${COOPDX_PYTHON}" "${XSDK_ROOT}/tools/elf2xex.py"
        --title-name "sm64coopdx360" "$<TARGET_FILE:sm64coopdx>" "${CMAKE_CURRENT_BINARY_DIR}/sm64coopdx.xex" VERBATIM)
endif()
