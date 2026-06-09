cmake_minimum_required(VERSION 3.20)

project(minecraft_native VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" MINECRAFT_NATIVE_SOURCE_DIR)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources" MINECRAFT_NATIVE_RESOURCE_DIR)
set(MINECRAFT_ZLIB_ROOT "C:/msys64/mingw64")

file(GLOB_RECURSE MINECRAFT_PORT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/*.cpp"
)

file(GLOB SEEDFINDER_GUI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/gui/*.cpp"
)

# Compiled into LTO consumers directly so world headers stay in one link unit.
set(SEEDFINDER_PROBE_ADAPTER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/probe/WorldGenProbeAdapter.cpp"
)

function(minecraft_configure_target target)
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_compile_definitions(${target} PRIVATE
        MINECRAFT_NATIVE_SOURCE_DIR="${MINECRAFT_NATIVE_SOURCE_DIR}"
        MINECRAFT_NATIVE_RESOURCE_DIR="${MINECRAFT_NATIVE_RESOURCE_DIR}"
    )
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        target_compile_definitions(${target} PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated-declarations)
    endif()
endfunction()

find_library(MINECRAFT_ZLIB_LIBRARY
    NAMES z libz
    PATHS "${MINECRAFT_ZLIB_ROOT}/lib"
    NO_DEFAULT_PATH
)

function(minecraft_link_zlib target)
    if (MINECRAFT_ZLIB_LIBRARY)
        target_link_libraries(${target} PRIVATE "${MINECRAFT_ZLIB_LIBRARY}")
        set(MINECRAFT_ZLIB_DLL "C:/msys64/mingw64/bin/zlib1.dll")
        if (EXISTS "${MINECRAFT_ZLIB_DLL}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MINECRAFT_ZLIB_DLL}"
                    "$<TARGET_FILE_DIR:${target}>"
            )
        endif()
    else()
        target_compile_definitions(${target} PRIVATE MINECRAFT_DISABLE_ZLIB)
    endif()
endfunction()

function(minecraft_link_audio target)
    if (WIN32)
        target_link_libraries(${target} PRIVATE ole32 uuid avrt ksuser)
    elseif (APPLE)
        find_library(MINECRAFT_AUDIOTOOLBOX AudioToolbox)
        find_library(MINECRAFT_COREAUDIO CoreAudio)
        find_library(MINECRAFT_COREFOUNDATION CoreFoundation)
        target_link_libraries(${target} PRIVATE
            ${MINECRAFT_AUDIOTOOLBOX}
            ${MINECRAFT_COREAUDIO}
            ${MINECRAFT_COREFOUNDATION}
        )
    else()
        find_package(Threads REQUIRED)
        find_library(MINECRAFT_ASOUND asound)
        if (NOT MINECRAFT_ASOUND)
            message(FATAL_ERROR "ALSA (libasound) required for Linux audio playback")
        endif()
        target_link_libraries(${target} PRIVATE Threads::Threads m ${MINECRAFT_ASOUND})
    endif()
endfunction()

add_executable(minecraft_native
    src/main.cpp
    src/net/minecraft/client/platform/audio/decode/vorbiscompiler.c
    ${MINECRAFT_PORT_SOURCES}
    ${SEEDFINDER_GUI_SOURCES}
    ${SEEDFINDER_PROBE_ADAPTER_SOURCE}
)
minecraft_configure_target(minecraft_native)
minecraft_link_zlib(minecraft_native)
minecraft_link_audio(minecraft_native)
if (WIN32)
    target_link_libraries(minecraft_native PRIVATE ws2_32 winhttp gdiplus gdi32 user32 opengl32 glu32)
endif()
if (NOT MSVC)
    target_link_options(minecraft_native PRIVATE -static -static-libgcc -static-libstdc++)
endif()

add_executable(block_registry_properties_test
    tests/block_registry_properties_test.cpp
    src/net/minecraft/client/platform/audio/decode/vorbiscompiler.c
    ${MINECRAFT_PORT_SOURCES}
    ${SEEDFINDER_GUI_SOURCES}
    ${SEEDFINDER_PROBE_ADAPTER_SOURCE}
)
minecraft_configure_target(block_registry_properties_test)
minecraft_link_zlib(block_registry_properties_test)
minecraft_link_audio(block_registry_properties_test)
if (WIN32)
    target_link_libraries(block_registry_properties_test PRIVATE ws2_32 winhttp gdiplus gdi32 user32 opengl32 glu32)
endif()
if (NOT MSVC)
    target_link_options(block_registry_properties_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME block_registry_properties COMMAND block_registry_properties_test)

add_executable(java_random_golden_test tests/java_random_golden_test.cpp)
minecraft_configure_target(java_random_golden_test)
add_test(NAME JavaRandomGoldenTest COMMAND java_random_golden_test)

add_executable(stereo_transform_color_golden_test
    tests/stereo_transform_color_golden_test.cpp
    src/net/minecraft/client/render/platform/StereoRendering.cpp
)
minecraft_configure_target(stereo_transform_color_golden_test)
if (WIN32)
    target_link_libraries(stereo_transform_color_golden_test PRIVATE opengl32 glu32)
endif()
add_test(NAME StereoTransformColorGoldenTest COMMAND stereo_transform_color_golden_test)

add_library(seedfinder_engine STATIC
    src/seedfinder/config/JsonConfig.cpp
    src/seedfinder/config/CliArgs.cpp
    src/seedfinder/engine/BiomeCatalog.cpp
    src/seedfinder/engine/SearchEngine.cpp
    src/seedfinder/engine/Scorer.cpp
    src/seedfinder/engine/TierPlanner.cpp
    src/seedfinder/engine/Checkpoint.cpp
    src/seedfinder/runtime/RuntimeInit.cpp
)
minecraft_configure_target(seedfinder_engine)
target_include_directories(seedfinder_engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(minecraft_native PRIVATE seedfinder_engine)
target_link_libraries(block_registry_properties_test PRIVATE seedfinder_engine)

file(GLOB_RECURSE SEEDFINDER_WORLD_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/world/*.cpp")
list(REMOVE_ITEM SEEDFINDER_WORLD_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/world/ClientWorld.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/world/ServerWorld.cpp")

file(GLOB_RECURSE SEEDFINDER_ENTITY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/entity/*.cpp")
list(REMOVE_ITEM SEEDFINDER_ENTITY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/entity/player/ClientPlayerEntity.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/entity/player/ServerPlayerEntity.cpp")

file(GLOB_RECURSE SEEDFINDER_BLOCK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/block/*.cpp")
file(GLOB_RECURSE SEEDFINDER_ITEM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/item/*.cpp")
file(GLOB_RECURSE SEEDFINDER_NBT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/nbt/*.cpp")
file(GLOB_RECURSE SEEDFINDER_INVENTORY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/inventory/*.cpp")
file(GLOB_RECURSE SEEDFINDER_RECIPE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/recipe/*.cpp")
file(GLOB_RECURSE SEEDFINDER_UTIL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/util/*.cpp")
file(GLOB_RECURSE SEEDFINDER_STAT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/stat/*.cpp")
file(GLOB_RECURSE SEEDFINDER_ACHIEVEMENT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/achievement/*.cpp")
file(GLOB_RECURSE SEEDFINDER_SCREEN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/screen/*.cpp")

set(SEEDFINDER_PORT_SOURCES
    ${SEEDFINDER_PROBE_ADAPTER_SOURCE}
    src/net/minecraft/client/resource/language/TranslationStorage.cpp
    ${SEEDFINDER_WORLD_SOURCES}
    ${SEEDFINDER_BLOCK_SOURCES}
    ${SEEDFINDER_ENTITY_SOURCES}
    ${SEEDFINDER_ITEM_SOURCES}
    ${SEEDFINDER_NBT_SOURCES}
    ${SEEDFINDER_INVENTORY_SOURCES}
    ${SEEDFINDER_RECIPE_SOURCES}
    ${SEEDFINDER_UTIL_SOURCES}
    ${SEEDFINDER_STAT_SOURCES}
    ${SEEDFINDER_ACHIEVEMENT_SOURCES}
    ${SEEDFINDER_SCREEN_SOURCES}
)

add_executable(seedfinder src/seedfinder/main.cpp ${SEEDFINDER_PORT_SOURCES})
target_link_libraries(seedfinder PRIVATE seedfinder_engine)
minecraft_configure_target(seedfinder)
minecraft_link_zlib(seedfinder)
if (NOT MSVC)
    target_link_options(seedfinder PRIVATE -static -static-libgcc -static-libstdc++)
endif()

add_executable(seed_probe_golden_test tests/seed_probe_golden_test.cpp ${SEEDFINDER_PORT_SOURCES})
target_link_libraries(seed_probe_golden_test PRIVATE seedfinder_engine)
minecraft_configure_target(seed_probe_golden_test)
minecraft_link_zlib(seed_probe_golden_test)
if (NOT MSVC)
    target_link_options(seed_probe_golden_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME SeedProbeGoldenTest COMMAND seed_probe_golden_test)
