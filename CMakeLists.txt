cmake_minimum_required(VERSION 3.20)

project(minecraft_native VERSION 0.1.0 LANGUAGES CXX)



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

if (WIN32)
    enable_language(RC)
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

file(GLOB_RECURSE MINECRAFT_PORT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/*.cpp"
)

set(MINECRAFT_REGISTRY_ROOT_PATTERNS
    "MC_REGISTER_BLOCK"
    "MC_REGISTER_ITEM"
    "MC_REGISTER_ENTITY"
    "RegisterPhase<"
    "Registry::addBlock("
    "Registry::addItem("
    "Registry::addEntity("
)

set(MINECRAFT_REGISTRY_ROOT_SOURCES)
foreach(source ${MINECRAFT_PORT_SOURCES})
    if (NOT source MATCHES "/(block|item|entity|registry)/.*\\.cpp$")
        continue()
    endif()
    file(READ "${source}" MINECRAFT_REGISTRY_ROOT_CONTENTS)
    foreach(pattern ${MINECRAFT_REGISTRY_ROOT_PATTERNS})
        string(FIND "${MINECRAFT_REGISTRY_ROOT_CONTENTS}" "${pattern}" MINECRAFT_REGISTRY_ROOT_MATCH)
        if (NOT MINECRAFT_REGISTRY_ROOT_MATCH EQUAL -1)
            list(APPEND MINECRAFT_REGISTRY_ROOT_SOURCES "${source}")
            break()
        endif()
    endforeach()
endforeach()
list(REMOVE_DUPLICATES MINECRAFT_REGISTRY_ROOT_SOURCES)

set(MINECRAFT_CORE_SOURCES ${MINECRAFT_PORT_SOURCES})
list(FILTER MINECRAFT_CORE_SOURCES EXCLUDE REGEX "/client/|/server/|/net/minecraft/mod/")
list(REMOVE_ITEM MINECRAFT_CORE_SOURCES ${MINECRAFT_REGISTRY_ROOT_SOURCES})

set(MINECRAFT_DEDICATED_SERVER_SOURCES ${MINECRAFT_PORT_SOURCES})
list(FILTER MINECRAFT_DEDICATED_SERVER_SOURCES INCLUDE REGEX "/server/")
list(APPEND MINECRAFT_DEDICATED_SERVER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/world/ServerWorld.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/entity/player/ServerPlayerEntity.cpp"
)
list(REMOVE_ITEM MINECRAFT_CORE_SOURCES ${MINECRAFT_DEDICATED_SERVER_SOURCES})

set(MINECRAFT_CLIENT_ONLY_SOURCES ${MINECRAFT_PORT_SOURCES})
list(FILTER MINECRAFT_CLIENT_ONLY_SOURCES INCLUDE REGEX
    "/net/minecraft/(achievement/Achievements|entity/EntityTypes|entity/player/ClientPlayerEntity|world/ClientWorld)\\.cpp$"
)
list(REMOVE_ITEM MINECRAFT_CORE_SOURCES ${MINECRAFT_CLIENT_ONLY_SOURCES})

set(MINECRAFT_CLIENT_SOURCES ${MINECRAFT_PORT_SOURCES})
list(FILTER MINECRAFT_CLIENT_SOURCES INCLUDE REGEX "/client/|/net/minecraft/mod/")
list(FILTER MINECRAFT_CLIENT_SOURCES EXCLUDE REGEX "/net/minecraft/mod/runtime/LuaTerrainBindings\.cpp$")

list(APPEND MINECRAFT_CLIENT_SOURCES ${MINECRAFT_CLIENT_ONLY_SOURCES})
list(REMOVE_DUPLICATES MINECRAFT_CLIENT_SOURCES)

set(MINECRAFT_SERVER_MOD_SOURCES ${MINECRAFT_PORT_SOURCES})
list(FILTER MINECRAFT_SERVER_MOD_SOURCES INCLUDE REGEX "/net/minecraft/mod/")
list(FILTER MINECRAFT_SERVER_MOD_SOURCES EXCLUDE REGEX
    "/net/minecraft/mod/(HostScreenRegistry|ModClient|ScreenUi)\\.cpp$"
)
list(FILTER MINECRAFT_SERVER_MOD_SOURCES EXCLUDE REGEX
    "/net/minecraft/mod/model/.*\\.cpp$"
)
list(FILTER MINECRAFT_SERVER_MOD_SOURCES EXCLUDE REGEX
    "/net/minecraft/mod/runtime/(LuaCameraBindings|LuaGuiArgs|LuaInventoryBindings|LuaModelBindings|LuaRaycastBindings|LuaRenderBindings|LuaScreenBindings|LuaSoundBindings|LuaTextureBindings|ModRenderScope)\\.cpp$"
)

add_subdirectory(mods)

function(minecraft_configure_target target)
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${MINECRAFT_TOOLCHAIN_ROOT}/include"
    )
    target_compile_definitions(${target} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:MINECRAFT_NATIVE_SOURCE_DIR=\"${MINECRAFT_NATIVE_SOURCE_DIR}\">"
    )
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /wd4996)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated-declarations)
    endif()
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

function(minecraft_copy_lua_runtime target)
    if (WIN32)
        set(MINECRAFT_LUA_DLL "${MINECRAFT_TOOLCHAIN_ROOT}/bin/lua54.dll")
        if (EXISTS "${MINECRAFT_LUA_DLL}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MINECRAFT_LUA_DLL}"
                    "$<TARGET_FILE_DIR:${target}>"
            )
        else()
            message(WARNING "lua54.dll not found under ${MINECRAFT_TOOLCHAIN_ROOT}/bin; Lua mods will require it on PATH")
        endif()
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

function(minecraft_enable_glfw_compile target)
    if (WIN32)
        target_compile_definitions(${target} PRIVATE MINECRAFT_USE_GLFW=1)
        target_link_libraries(${target} PRIVATE glfw)
    endif()
endfunction()

function(minecraft_link_platform target)
    if (WIN32)
        target_link_libraries(${target} PRIVATE ws2_32 winhttp crypt32 shell32 dbghelp)
        target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endfunction()

function(minecraft_link_core target)
    target_link_libraries(${target} PRIVATE minecraft_core)
endfunction()

function(minecraft_link_registry_roots target)
    target_sources(${target} PRIVATE $<TARGET_OBJECTS:minecraft_registry_roots>)
endfunction()

function(minecraft_link_core_whole target)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_libraries(${target} PRIVATE -Wl,--whole-archive minecraft_core -Wl,--no-whole-archive)
    else()
        target_link_libraries(${target} PRIVATE minecraft_core)
    endif()
endfunction()

function(minecraft_link_core_client target)
    minecraft_link_core(${target})
    minecraft_link_zlib(${target})
    minecraft_link_audio(${target})
    minecraft_link_display(${target})
    minecraft_link_platform(${target})
    if (WIN32)
        target_link_libraries(${target} PRIVATE opengl32 glu32)
    endif()
endfunction()

function(minecraft_link_server target)
    minecraft_link_core_whole(${target})
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_libraries(${target} PRIVATE -Wl,--whole-archive minecraft_dedicated_server_core -Wl,--no-whole-archive)
    else()
        target_link_libraries(${target} PRIVATE minecraft_dedicated_server_core)
    endif()
    minecraft_link_zlib(${target})
    minecraft_link_platform(${target})
    minecraft_copy_lua_runtime(${target})
    if (WIN32)
        target_link_libraries(${target} PRIVATE comctl32 gdi32 user32 gdiplus)
    endif()
endfunction()

add_library(minecraft_core STATIC ${MINECRAFT_CORE_SOURCES})
minecraft_configure_target(minecraft_core)
minecraft_link_zlib(minecraft_core)
minecraft_link_platform(minecraft_core)

add_library(minecraft_dedicated_server_core STATIC ${MINECRAFT_DEDICATED_SERVER_SOURCES})
minecraft_configure_target(minecraft_dedicated_server_core)
minecraft_link_zlib(minecraft_dedicated_server_core)
minecraft_link_platform(minecraft_dedicated_server_core)

add_library(minecraft_registry_roots OBJECT ${MINECRAFT_REGISTRY_ROOT_SOURCES})
minecraft_configure_target(minecraft_registry_roots)

add_library(minecraft_client OBJECT ${MINECRAFT_CLIENT_SOURCES})
minecraft_configure_target(minecraft_client)

if (WIN32)
    target_compile_definitions(minecraft_client PRIVATE MINECRAFT_NATIVE_EXPORTS=1)
    minecraft_enable_glfw_compile(minecraft_client)
endif()

add_library(minecraft_server_mod OBJECT ${MINECRAFT_SERVER_MOD_SOURCES})
minecraft_configure_target(minecraft_server_mod)

if (WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/app.rc")
    add_library(minecraft_app_icon OBJECT resources/icon/app.rc)
    set(MINECRAFT_APP_ICON_OBJECTS $<TARGET_OBJECTS:minecraft_app_icon>)
else()
    set(MINECRAFT_APP_ICON_OBJECTS)
endif()

add_executable(minecraft_native
    src/main.cpp
    $<TARGET_OBJECTS:minecraft_client>
    ${MINECRAFT_APP_ICON_OBJECTS}
)
minecraft_link_core(minecraft_native)
minecraft_link_registry_roots(minecraft_native)
minecraft_configure_target(minecraft_native)
minecraft_link_zlib(minecraft_native)
minecraft_link_audio(minecraft_native)
minecraft_link_display(minecraft_native)
if (WIN32)
    target_compile_definitions(minecraft_native PRIVATE MINECRAFT_NATIVE_EXPORTS=1)
    target_link_libraries(minecraft_native PRIVATE comdlg32 gdiplus gdi32 user32 opengl32 glu32 winmm)
    minecraft_link_platform(minecraft_native)
    # Debug builds keep the console subsystem so stdout/stderr logging and
    # ClientDiagnostics::ensureConsole() are visible. Release/RelWithDebInfo link as a
    # Windows-subsystem app (no console window on launch) to match ensureConsole()'s
    # #ifndef NDEBUG guard in ClientDiagnostics.cpp.
    target_link_options(minecraft_native PRIVATE $<$<NOT:$<CONFIG:Debug>>:-mwindows>)
endif()
minecraft_copy_lua_runtime(minecraft_native)

add_executable(minecraft_server
    src/server-main.cpp
    src/net/minecraft/client/auth/microsoft/PlayerTextureUrls.cpp
    src/net/minecraft/client/resource/language/TranslationStorage.cpp
    src/net/minecraft/client/util/MinecraftDirectories.cpp
    $<TARGET_OBJECTS:minecraft_server_mod>
    ${MINECRAFT_APP_ICON_OBJECTS}
)
minecraft_configure_target(minecraft_server)
minecraft_link_registry_roots(minecraft_server)
minecraft_link_server(minecraft_server)
if (WIN32)
    target_link_libraries(minecraft_server PRIVATE winmm)
endif()

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    URL_HASH SHA256=1f357c27ca988c3f7c6b4bf68a9395005ac6761f034046e9dde0896e3aba00e4
    TLS_VERIFY OFF
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(MINECRAFT_TEST_SOURCES
    tests/packet_roundtrip_test.cpp
    tests/registry_bootstrap_test.cpp
    tests/lan_host_coordinator_test.cpp
    tests/world_required_mods_test.cpp
    tests/multiplayer_client_player_entity_test.cpp
    tests/mp_parity_updates_test.cpp
    tests/client_timer_test.cpp
    tests/resolved_render_options_test.cpp
    tests/pack_manifest_test.cpp
    tests/lua_runtime_prelude_test.cpp
    tests/mod_settings_registry_test.cpp
    tests/model_registry_parent_test.cpp
    tests/unified_light_registry_test.cpp
    tests/render_profiler_test.cpp
)



set(MINECRAFT_SERVER_TEST_SOURCES
    integrated_server_host_test.cpp
    mp_chunk_data_test.cpp
    mp_chunk_delivery_test.cpp
    server_properties_test.cpp
    server_command_handler_test.cpp
    server_chunk_cache_test.cpp
    server_world_events_test.cpp
    server_login_handler_test.cpp
    minecraft_server_tick_test.cpp
    player_manager_test.cpp
    chunk_map_test.cpp
    connection_listener_test.cpp
)

list(TRANSFORM MINECRAFT_SERVER_TEST_SOURCES PREPEND "tests/")
list(APPEND MINECRAFT_TEST_SOURCES ${MINECRAFT_SERVER_TEST_SOURCES})

add_executable(minecraft_omega_tests
    ${MINECRAFT_TEST_SOURCES}
    $<TARGET_OBJECTS:minecraft_client>
)
minecraft_configure_target(minecraft_omega_tests)
target_include_directories(minecraft_omega_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_definitions(minecraft_omega_tests PRIVATE MINECRAFT_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
minecraft_link_registry_roots(minecraft_omega_tests)
minecraft_link_server(minecraft_omega_tests)
minecraft_link_audio(minecraft_omega_tests)
minecraft_link_display(minecraft_omega_tests)
if (WIN32)
    target_link_libraries(minecraft_omega_tests PRIVATE opengl32 glu32)
endif()
target_link_libraries(minecraft_omega_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(minecraft_omega_tests)
