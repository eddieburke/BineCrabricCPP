cmake_minimum_required(VERSION 3.20)

project(minecraft_native VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

if (WIN32)
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    find_package(glfw3 CONFIG QUIET)
    if (NOT glfw3_FOUND)
        FetchContent_Declare(
            glfw
            URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip
            URL_HASH SHA256=a133ddc3d3c66143eba9035621db8e0bcf34dba1ee9514a9e23e96afd39fd57a
            TLS_VERIFY OFF
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(glfw)
    endif()
endif()

if (NOT DEFINED MINECRAFT_TOOLCHAIN_ROOT)
    set(MINECRAFT_TOOLCHAIN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/toolchain/mingw64")
endif()
if (NOT IS_ABSOLUTE "${MINECRAFT_TOOLCHAIN_ROOT}")
    set(MINECRAFT_TOOLCHAIN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/${MINECRAFT_TOOLCHAIN_ROOT}")
endif()
file(TO_CMAKE_PATH "${MINECRAFT_TOOLCHAIN_ROOT}" MINECRAFT_TOOLCHAIN_ROOT)

find_program(CCACHE_PROGRAM ccache
    PATHS "${MINECRAFT_TOOLCHAIN_ROOT}/bin"
    NO_DEFAULT_PATH
)
if (CCACHE_PROGRAM AND CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message(STATUS "ccache disabled (incompatible with LTO)")
elseif (CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" MINECRAFT_NATIVE_SOURCE_DIR)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources" MINECRAFT_NATIVE_RESOURCE_DIR)

file(GLOB_RECURSE MINECRAFT_PORT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/*.cpp"
)

file(GLOB SEEDFINDER_GUI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/gui/*.cpp"
)

file(GLOB_RECURSE MSAUTH_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/msauth/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/msauth/**/*.cpp"
)

# Game-coupled seedfinder sources (probes, graph, metrics, live engine). Compiled into
# LTO consumers directly so world headers stay in one link unit.
set(SEEDFINDER_LIVE_PROBE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/probe/ScanProbe.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/probe/DecorateProbe.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/graph/NodeGraph.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/graph/GraphBuilder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/metric/Metric.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/config/ScanConfigBuilder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/engine/SearchEngine.cpp"
)

# Legacy C-ABI probe adapter, retained only for the golden faithfulness test.
set(SEEDFINDER_GOLDEN_ADAPTER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/seedfinder/probe/WorldGenProbeAdapter.cpp"
)

function(minecraft_configure_target target)
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${MINECRAFT_TOOLCHAIN_ROOT}/include"
    )
    target_compile_definitions(${target} PRIVATE
        MINECRAFT_NATIVE_SOURCE_DIR="${MINECRAFT_NATIVE_SOURCE_DIR}"
        MINECRAFT_NATIVE_RESOURCE_DIR="${MINECRAFT_NATIVE_RESOURCE_DIR}"
    )
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated-declarations)
endfunction()

find_library(MINECRAFT_ZLIB_LIBRARY
    NAMES z libz
    PATHS "${MINECRAFT_TOOLCHAIN_ROOT}/lib"
    NO_DEFAULT_PATH
)

function(minecraft_link_zlib target)
    if (MINECRAFT_ZLIB_LIBRARY)
        target_link_libraries(${target} PRIVATE "${MINECRAFT_ZLIB_LIBRARY}")
        set(MINECRAFT_ZLIB_DLL "${MINECRAFT_TOOLCHAIN_ROOT}/bin/zlib1.dll")
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

find_library(MINECRAFT_VORBISFILE
    NAMES vorbisfile libvorbisfile
    PATHS "${MINECRAFT_TOOLCHAIN_ROOT}/lib"
    NO_DEFAULT_PATH
)
find_library(MINECRAFT_VORBIS
    NAMES vorbis libvorbis
    PATHS "${MINECRAFT_TOOLCHAIN_ROOT}/lib"
    NO_DEFAULT_PATH
)
find_library(MINECRAFT_OGG
    NAMES ogg libogg
    PATHS "${MINECRAFT_TOOLCHAIN_ROOT}/lib"
    NO_DEFAULT_PATH
)

set(MINECRAFT_HAS_VORBIS_LIBS FALSE)
if (MINECRAFT_VORBISFILE AND MINECRAFT_VORBIS AND MINECRAFT_OGG)
    set(MINECRAFT_HAS_VORBIS_LIBS TRUE)
    message(STATUS "Vorbis audio enabled from ${MINECRAFT_TOOLCHAIN_ROOT}")
else()
    message(WARNING "libvorbis not found under ${MINECRAFT_TOOLCHAIN_ROOT}; OGG/.mus playback disabled")
endif()

set(MINECRAFT_MINGW_SYSROOT_LIB "${MINECRAFT_TOOLCHAIN_ROOT}/x86_64-w64-mingw32/lib")
find_library(MINECRAFT_XAUDIO2
    NAMES xaudio2_9 xaudio2_8 xaudio2
    PATHS "${MINECRAFT_MINGW_SYSROOT_LIB}"
    NO_DEFAULT_PATH
)

function(minecraft_link_audio target)
    if (WIN32)
        target_link_libraries(${target} PRIVATE ole32 uuid avrt)
        if (MINECRAFT_XAUDIO2)
            target_link_libraries(${target} PRIVATE "${MINECRAFT_XAUDIO2}")
        else()
            message(WARNING "XAudio2 import library not found under ${MINECRAFT_MINGW_SYSROOT_LIB}")
        endif()
    endif()
    if (MINECRAFT_HAS_VORBIS_LIBS)
        target_link_libraries(${target} PRIVATE
            "${MINECRAFT_VORBISFILE}"
            "${MINECRAFT_VORBIS}"
            "${MINECRAFT_OGG}"
        )
        target_compile_definitions(${target} PRIVATE MINECRAFT_HAS_VORBIS=1)
        if (WIN32)
            foreach(_dll IN ITEMS libogg-0.dll libvorbis-0.dll libvorbisfile-3.dll)
                set(_dll_path "${MINECRAFT_TOOLCHAIN_ROOT}/bin/${_dll}")
                if (EXISTS "${_dll_path}")
                    add_custom_command(TARGET ${target} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${_dll_path}"
                            "$<TARGET_FILE_DIR:${target}>"
                    )
                endif()
            endforeach()
        endif()
    endif()
endfunction()

function(minecraft_link_display target)
    if (WIN32)
        target_compile_definitions(${target} PRIVATE MINECRAFT_USE_GLFW=1)
        target_link_libraries(${target} PRIVATE glfw)
    endif()
endfunction()

add_executable(minecraft_native
    src/main.cpp
    ${MINECRAFT_PORT_SOURCES}
    ${SEEDFINDER_GUI_SOURCES}
    ${SEEDFINDER_LIVE_PROBE_SOURCES}
    ${MSAUTH_SOURCES}
)
minecraft_configure_target(minecraft_native)
minecraft_link_zlib(minecraft_native)
minecraft_link_audio(minecraft_native)
minecraft_link_display(minecraft_native)
if (WIN32)
    target_link_libraries(minecraft_native PRIVATE ws2_32 winhttp crypt32 shell32 comdlg32 gdiplus gdi32 user32 opengl32 glu32 dbghelp)
    target_link_options(minecraft_native PRIVATE -static -static-libgcc -static-libstdc++)
endif()

add_executable(block_registry_properties_test
    tests/block_registry_properties_test.cpp
    ${MINECRAFT_PORT_SOURCES}
    ${SEEDFINDER_GUI_SOURCES}
    ${SEEDFINDER_LIVE_PROBE_SOURCES}
    ${MSAUTH_SOURCES}
)
minecraft_configure_target(block_registry_properties_test)
minecraft_link_zlib(block_registry_properties_test)
minecraft_link_audio(block_registry_properties_test)
minecraft_link_display(block_registry_properties_test)
if (WIN32)
    target_link_libraries(block_registry_properties_test PRIVATE ws2_32 winhttp shell32 comdlg32 crypt32 gdiplus gdi32 user32 opengl32 glu32 dbghelp)
    target_link_options(block_registry_properties_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME block_registry_properties COMMAND block_registry_properties_test)

add_executable(mod_lifecycle_registry_test
    tests/mod_lifecycle_registry_test.cpp
    ${MINECRAFT_PORT_SOURCES}
    ${SEEDFINDER_GUI_SOURCES}
    ${SEEDFINDER_LIVE_PROBE_SOURCES}
    ${MSAUTH_SOURCES}
)
minecraft_configure_target(mod_lifecycle_registry_test)
minecraft_link_zlib(mod_lifecycle_registry_test)
minecraft_link_audio(mod_lifecycle_registry_test)
minecraft_link_display(mod_lifecycle_registry_test)
if (WIN32)
    target_link_libraries(mod_lifecycle_registry_test PRIVATE ws2_32 winhttp shell32 comdlg32 crypt32 gdiplus gdi32 user32 opengl32 glu32 dbghelp)
    target_link_options(mod_lifecycle_registry_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME ModLifecycleRegistryTest COMMAND mod_lifecycle_registry_test)

add_executable(java_random_golden_test tests/java_random_golden_test.cpp)
minecraft_configure_target(java_random_golden_test)
add_test(NAME JavaRandomGoldenTest COMMAND java_random_golden_test)

add_executable(msauth_account_storage_test
    tests/msauth_account_storage_test.cpp
    src/msauth/AccountStorage.cpp
    src/msauth/MicrosoftAuth.cpp
)
minecraft_configure_target(msauth_account_storage_test)
if (WIN32)
    target_link_libraries(msauth_account_storage_test PRIVATE crypt32)
    target_link_options(msauth_account_storage_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME MsauthAccountStorageTest COMMAND msauth_account_storage_test)

file(GLOB_RECURSE NBT_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/nbt/*.cpp")
add_executable(nbt_roundtrip_test tests/nbt_roundtrip_test.cpp ${NBT_TEST_SOURCES})
minecraft_configure_target(nbt_roundtrip_test)
minecraft_link_zlib(nbt_roundtrip_test)
if (WIN32)
    target_link_options(nbt_roundtrip_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME NbtRoundtripTest COMMAND nbt_roundtrip_test)

add_executable(stereo_transform_color_golden_test
    tests/stereo_transform_color_golden_test.cpp
    src/net/minecraft/client/render/platform/StereoRendering.cpp
)
minecraft_configure_target(stereo_transform_color_golden_test)
if (WIN32)
    target_link_libraries(stereo_transform_color_golden_test PRIVATE opengl32 glu32)
endif()
add_test(NAME StereoTransformColorGoldenTest COMMAND stereo_transform_color_golden_test)

add_executable(view_distance_test
    tests/view_distance_test.cpp
    src/net/minecraft/client/render/ViewDistance.cpp
)
minecraft_configure_target(view_distance_test)
if (WIN32)
    target_link_options(view_distance_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME ViewDistanceTest COMMAND view_distance_test)

add_library(seedfinder_engine STATIC
    src/seedfinder/config/JsonConfig.cpp
    src/seedfinder/config/CliArgs.cpp
    src/seedfinder/engine/BiomeCatalog.cpp
    src/seedfinder/engine/Checkpoint.cpp
    src/seedfinder/runtime/RuntimeInit.cpp
)
minecraft_configure_target(seedfinder_engine)
target_include_directories(seedfinder_engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(minecraft_native PRIVATE seedfinder_engine)
target_link_libraries(block_registry_properties_test PRIVATE seedfinder_engine)
target_link_libraries(mod_lifecycle_registry_test PRIVATE seedfinder_engine)

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
file(GLOB_RECURSE SEEDFINDER_REGISTRY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/registry/*.cpp")

set(SEEDFINDER_PORT_SOURCES
    ${SEEDFINDER_LIVE_PROBE_SOURCES}
    src/net/minecraft/client/resource/language/TranslationStorage.cpp
    ${SEEDFINDER_REGISTRY_SOURCES}
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
if (WIN32)
    target_link_options(seedfinder PRIVATE -static -static-libgcc -static-libstdc++)
endif()

add_executable(seed_probe_golden_test tests/seed_probe_golden_test.cpp ${SEEDFINDER_PORT_SOURCES} ${SEEDFINDER_GOLDEN_ADAPTER_SOURCE})
target_link_libraries(seed_probe_golden_test PRIVATE seedfinder_engine)
minecraft_configure_target(seed_probe_golden_test)
minecraft_link_zlib(seed_probe_golden_test)
if (WIN32)
    target_link_options(seed_probe_golden_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME SeedProbeGoldenTest COMMAND seed_probe_golden_test)

add_executable(world_gen_parity_test tests/world_gen_parity_test.cpp ${SEEDFINDER_PORT_SOURCES})
target_link_libraries(world_gen_parity_test PRIVATE seedfinder_engine)
minecraft_configure_target(world_gen_parity_test)
minecraft_link_zlib(world_gen_parity_test)
if (WIN32)
    target_link_options(world_gen_parity_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME WorldGenParityTest COMMAND world_gen_parity_test)

add_executable(biome_temperature_parity_test tests/biome_temperature_parity_test.cpp ${SEEDFINDER_PORT_SOURCES})
target_link_libraries(biome_temperature_parity_test PRIVATE seedfinder_engine)
minecraft_configure_target(biome_temperature_parity_test)
minecraft_link_zlib(biome_temperature_parity_test)
if (WIN32)
    target_link_options(biome_temperature_parity_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME BiomeTemperatureParityTest COMMAND biome_temperature_parity_test)

add_executable(generator_rng_parity_test tests/generator_rng_parity_test.cpp)
minecraft_configure_target(generator_rng_parity_test)
if (WIN32)
    target_link_options(generator_rng_parity_test PRIVATE -static -static-libgcc -static-libstdc++)
endif()
add_test(NAME GeneratorRngParityTest COMMAND generator_rng_parity_test)
