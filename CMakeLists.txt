cmake_minimum_required(VERSION 3.20)

project(minecraft_native VERSION 0.1.0 LANGUAGES CXX)



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

if (WIN32)
    enable_language(RC)
endif()
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
if (NOT WIN32)
    find_package(OpenGL REQUIRED)
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
option(MINECRAFT_USE_CCACHE "Use ccache when available" ON)
if (CCACHE_PROGRAM AND CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message(STATUS "ccache disabled (incompatible with LTO)")
elseif (CCACHE_PROGRAM AND MINECRAFT_USE_CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
else()
    set(CMAKE_CXX_COMPILER_LAUNCHER "")
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

# Engine GLSL snippets are embedded in the executable: bake src/.../shaders/glsl/*.glsl
# into a generated TU (EmbeddedGlslSnippets.cpp) at configure time. The shader text
# ships inside the binary; there is no resources/glsl runtime lookup.
set(MINECRAFT_GLSL_EMBED_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen-embedded-glsl.cmake")
set(MINECRAFT_GLSL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/client/render/shaders/glsl")
set(MINECRAFT_EMBEDDED_GLSL_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedGlslSnippets.cpp")
file(GLOB MINECRAFT_GLSL_FILES CONFIGURE_DEPENDS "${MINECRAFT_GLSL_DIR}/*.glsl")
if(MINECRAFT_GLSL_FILES)
    add_custom_command(
        OUTPUT "${MINECRAFT_EMBEDDED_GLSL_SOURCE}"
        COMMAND "${CMAKE_COMMAND}" "-DGLSL_DIR=${MINECRAFT_GLSL_DIR}" "-DOUTPUT=${MINECRAFT_EMBEDDED_GLSL_SOURCE}" -P "${MINECRAFT_GLSL_EMBED_GENERATOR}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS "${MINECRAFT_GLSL_EMBED_GENERATOR}" ${MINECRAFT_GLSL_FILES}
        VERBATIM
    )
    set_source_files_properties("${MINECRAFT_EMBEDDED_GLSL_SOURCE}" PROPERTIES GENERATED TRUE)
    list(APPEND MINECRAFT_CLIENT_SOURCES "${MINECRAFT_EMBEDDED_GLSL_SOURCE}")
endif()

# The built-in vanilla shader pack is embedded in the executable the same way:
# bake src/.../shaders/glsl/vanilla/** into a generated TU (EmbeddedVanillaPack.cpp)
# at configure time. The pack's sources ship inside the binary; the engine only
# reads the on-disk shaders/vanilla folder when it exists (so it can still be
# tweaked in place).
set(MINECRAFT_VANILLA_PACK_EMBED_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen-embedded-vanilla-pack.cmake")
set(MINECRAFT_VANILLA_PACK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/client/render/shaders/glsl/vanilla")
set(MINECRAFT_EMBEDDED_VANILLA_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedVanillaPack.cpp")
file(GLOB_RECURSE MINECRAFT_VANILLA_PACK_FILES CONFIGURE_DEPENDS "${MINECRAFT_VANILLA_PACK_DIR}/*")
if(MINECRAFT_VANILLA_PACK_FILES)
    add_custom_command(
        OUTPUT "${MINECRAFT_EMBEDDED_VANILLA_SOURCE}"
        COMMAND "${CMAKE_COMMAND}" "-DVANILLA_PACK_DIR=${MINECRAFT_VANILLA_PACK_DIR}" "-DOUTPUT=${MINECRAFT_EMBEDDED_VANILLA_SOURCE}" -P "${MINECRAFT_VANILLA_PACK_EMBED_GENERATOR}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS "${MINECRAFT_VANILLA_PACK_EMBED_GENERATOR}" ${MINECRAFT_VANILLA_PACK_FILES}
        VERBATIM
    )
    set_source_files_properties("${MINECRAFT_EMBEDDED_VANILLA_SOURCE}" PROPERTIES GENERATED TRUE)
    list(APPEND MINECRAFT_CLIENT_SOURCES "${MINECRAFT_EMBEDDED_VANILLA_SOURCE}")
endif()

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

option(MINECRAFT_FRAME_PROFILE "Enable frame-phase profiling" OFF)

option(VTUNE_ENABLED "Emit Intel VTune ITT trace markers (VT_TRACE_EVENT/COUNTER/FRAME)" OFF)
if (VTUNE_ENABLED)
    if (NOT DEFINED VTUNE_SDK_ROOT)
        set(VTUNE_SDK_ROOT "C:/Program Files (x86)/Intel/oneAPI/vtune/latest/sdk")
    endif()
    find_path(VTUNE_INCLUDE_DIR ittnotify.h PATHS "${VTUNE_SDK_ROOT}/include" NO_DEFAULT_PATH)
    find_library(VTUNE_LIBRARY NAMES libittnotify ittnotify PATHS "${VTUNE_SDK_ROOT}/lib64" NO_DEFAULT_PATH)
    if (NOT VTUNE_INCLUDE_DIR OR NOT VTUNE_LIBRARY)
        message(FATAL_ERROR "VTUNE_ENABLED=ON but VTune SDK not found under VTUNE_SDK_ROOT='${VTUNE_SDK_ROOT}' (set -DVTUNE_SDK_ROOT=... to override)")
    endif()
    message(STATUS "VTune ITT tracing enabled: ${VTUNE_INCLUDE_DIR}, ${VTUNE_LIBRARY}")
endif()

function(minecraft_configure_target target)
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${MINECRAFT_TOOLCHAIN_ROOT}/include"
    )
    target_compile_definitions(${target} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:MINECRAFT_NATIVE_SOURCE_DIR=\"${MINECRAFT_NATIVE_SOURCE_DIR}\">"
        "$<$<AND:$<BOOL:${MINECRAFT_FRAME_PROFILE}>,$<COMPILE_LANGUAGE:CXX>>:MINECRAFT_FRAME_PROFILE=1>"
    )
    if (VTUNE_ENABLED)
        target_include_directories(${target} PRIVATE "${VTUNE_INCLUDE_DIR}")
        target_compile_definitions(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:VTUNE_ENABLED=1>")
        target_link_libraries(${target} PRIVATE "${VTUNE_LIBRARY}")
    endif()
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /wd4996)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated-declarations)
    endif()
endfunction()

if (EXISTS "${MINECRAFT_TOOLCHAIN_ROOT}/lib/libz.a")
    set(MINECRAFT_ZLIB_LIBRARY "${MINECRAFT_TOOLCHAIN_ROOT}/lib/libz.a")
else()
    find_library(MINECRAFT_ZLIB_LIBRARY
        NAMES libz.a z libz
        PATHS "${MINECRAFT_TOOLCHAIN_ROOT}/lib"
        NO_DEFAULT_PATH
    )
endif()

function(minecraft_link_zlib target)
    if (MINECRAFT_ZLIB_LIBRARY)
        target_link_libraries(${target} PRIVATE "${MINECRAFT_ZLIB_LIBRARY}")
    else()
        target_compile_definitions(${target} PRIVATE MINECRAFT_DISABLE_ZLIB)
    endif()
endfunction()

set(MINECRAFT_MINGW_SYSROOT_LIB "${MINECRAFT_TOOLCHAIN_ROOT}/x86_64-w64-mingw32/lib")
find_library(MINECRAFT_OGG NAMES libogg.a ogg PATHS "${MINECRAFT_TOOLCHAIN_ROOT}/lib" NO_DEFAULT_PATH)
find_library(MINECRAFT_VORBIS NAMES libvorbis.a vorbis PATHS "${MINECRAFT_TOOLCHAIN_ROOT}/lib" NO_DEFAULT_PATH)
find_library(MINECRAFT_VORBISFILE NAMES libvorbisfile.a vorbisfile PATHS "${MINECRAFT_TOOLCHAIN_ROOT}/lib" NO_DEFAULT_PATH)
set(MINECRAFT_HAS_VORBIS FALSE)
if(MINECRAFT_OGG AND MINECRAFT_VORBIS AND MINECRAFT_VORBISFILE)
    set(MINECRAFT_HAS_VORBIS TRUE)
else()
    message(WARNING "libogg, libvorbis and libvorbisfile are required for OGG/.mus playback")
endif()
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
    if(MINECRAFT_HAS_VORBIS)
        target_link_libraries(${target} PRIVATE "${MINECRAFT_VORBISFILE}" "${MINECRAFT_VORBIS}" "${MINECRAFT_OGG}")
    endif()
endfunction()

function(minecraft_link_display target)
    target_link_libraries(${target} PRIVATE glfw)
    if (NOT WIN32)
        target_link_libraries(${target} PRIVATE OpenGL::GL)
    endif()
endfunction()

function(minecraft_link_platform target)
    if (WIN32)
        target_link_libraries(${target} PRIVATE ws2_32 winhttp crypt32 shell32 dbghelp)
        target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
    else()
        target_link_libraries(${target} PRIVATE ${CMAKE_DL_LIBS})
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

function(minecraft_link_server target)
    minecraft_link_core_whole(${target})
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_libraries(${target} PRIVATE -Wl,--whole-archive minecraft_dedicated_server_core -Wl,--no-whole-archive)
    else()
        target_link_libraries(${target} PRIVATE minecraft_dedicated_server_core)
    endif()
    minecraft_link_zlib(${target})
    minecraft_link_platform(${target})
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
if(MINECRAFT_HAS_VORBIS)
    target_compile_definitions(minecraft_client PRIVATE MINECRAFT_HAS_VORBIS=1)
endif()

target_compile_definitions(minecraft_client PRIVATE MINECRAFT_NATIVE_EXPORTS=1)
minecraft_link_display(minecraft_client)

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
target_compile_definitions(minecraft_native PRIVATE MINECRAFT_NATIVE_EXPORTS=1)
minecraft_link_platform(minecraft_native)
if (WIN32)
    target_link_libraries(minecraft_native PRIVATE comdlg32 gdiplus gdi32 user32 opengl32 winmm)
    # Debug builds keep the console subsystem so stdout/stderr logging and
    # ClientDiagnostics::ensureConsole() are visible. Release/RelWithDebInfo link as a
    # Windows-subsystem app (no console window on launch) to match ensureConsole()'s
    # #ifndef NDEBUG guard in ClientDiagnostics.cpp.
    target_link_options(minecraft_native PRIVATE $<$<NOT:$<CONFIG:Debug>>:-mwindows>)
endif()

add_executable(minecraft_server
    src/server-main.cpp
    src/net/minecraft/client/auth/microsoft/PlayerTextureUrls.cpp
    src/net/minecraft/client/diagnostics/ClientDiagnostics.cpp
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

file(GLOB MINECRAFT_TEST_GLOB_SOURCES CONFIGURE_DEPENDS tests/*_test.cpp)
set(MINECRAFT_TEST_SOURCES
    tests/test-main.cpp
    ${MINECRAFT_TEST_GLOB_SOURCES}
)

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
    target_link_libraries(minecraft_omega_tests PRIVATE opengl32)
endif()
target_link_libraries(minecraft_omega_tests PRIVATE GTest::gtest)

include(GoogleTest)
# TIMEOUT bounds each case so a wedged test cannot sit forever. That matters beyond the
# test run: while it hangs it holds minecraft_omega_tests.exe and the build directory,
# so the next build-omega.ps1 fails with "Already in use" / "Permission denied" and
# RESTORES THE PREVIOUS EXECUTABLE — the game then runs code that was never rebuilt.
# The server-spinning cases are the slow ones (MissingCameraModRejectsClientWithoutModList
# alone takes ~77s because it generates a world), hence a generous bound rather than a
# tight one.
gtest_discover_tests(minecraft_omega_tests PROPERTIES TIMEOUT 300)

if (WIN32)
    add_executable(minecraft_installer src/installer/installer.cpp)
    set_target_properties(minecraft_installer PROPERTIES OUTPUT_NAME "setup")
    minecraft_configure_target(minecraft_installer)
    minecraft_link_platform(minecraft_installer)
    target_link_libraries(minecraft_installer PRIVATE comctl32 comdlg32 urlmon ws2_32)
    target_link_options(minecraft_installer PRIVATE -mwindows)
    add_custom_command(TARGET minecraft_installer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:minecraft_installer>/resources")
endif()
