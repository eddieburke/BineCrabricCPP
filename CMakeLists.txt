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

function(minecraft_generate_embedded_glsl output glsl_dir)
    file(GLOB glsl_files "${glsl_dir}/*.glsl")
    if(NOT glsl_files)
        message(FATAL_ERROR "No .glsl files found in ${glsl_dir}")
    endif()
    list(SORT glsl_files)
    set(generated "#include <string>\n#include <unordered_map>\n")
    string(APPEND generated "#include \"net/minecraft/client/render/shaders/GlslSnippets.hpp\"\n")
    string(APPEND generated "namespace net::minecraft::client::render {\n")
    string(APPEND generated "const std::unordered_map<std::string, std::string>& GlslSnippets::embeddedGlslSnippets() {\n")
    string(APPEND generated " static const std::unordered_map<std::string, std::string> kSnippets = {\n")
    foreach(glsl_file IN LISTS glsl_files)
        get_filename_component(filename "${glsl_file}" NAME)
        string(REGEX REPLACE "\\.glsl$" "" name "${filename}")
        file(READ "${glsl_file}" content)
        string(FIND "${content}" ")GLSL\"" terminator)
        if(NOT terminator EQUAL -1)
            message(FATAL_ERROR "GLSL snippet ${name} contains the raw-string terminator")
        endif()
        string(APPEND generated "  {\"${name}\", R\"GLSL(${content})GLSL\"},\n")
    endforeach()
    string(APPEND generated " };\n return kSnippets;\n}\n}\n")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
    file(WRITE "${output}" "${generated}")
endfunction()

function(minecraft_normalize_embedded_pack_path out path)
    string(REPLACE "\\" "/" normalized "${path}")
    string(REGEX REPLACE "/{2,}" "/" normalized "${normalized}")
    string(REPLACE "/" ";" parts "${normalized}")
    set(stack)
    foreach(part IN LISTS parts)
        if(part STREQUAL "." OR part STREQUAL "")
            continue()
        elseif(part STREQUAL "..")
            if(stack)
                list(POP_BACK stack)
            endif()
        else()
            list(APPEND stack "${part}")
        endif()
    endforeach()
    string(JOIN "/" result ${stack})
    set(${out} "${result}" PARENT_SCOPE)
endfunction()

function(minecraft_expand_embedded_pack_file out pack_dir path stack)
    if(path IN_LIST stack)
        message(FATAL_ERROR "Vanilla pack include cycle: ${path}")
    endif()
    list(APPEND stack "${path}")
    file(READ "${pack_dir}/${path}" content)
    string(REPLACE "\r\n" "\n" content "${content}")
    string(REPLACE "\r" "\n" content "${content}")
    set(result "")
    set(remaining "${content}")
    while(TRUE)
        string(FIND "${remaining}" "#include" include_position)
        if(include_position EQUAL -1)
            string(APPEND result "${remaining}")
            break()
        endif()
        string(SUBSTRING "${remaining}" 0 ${include_position} prefix)
        string(FIND "${prefix}" "\n" newline_position)
        if(newline_position EQUAL -1)
            set(prefix_whitespace "${prefix}")
        else()
            string(SUBSTRING "${prefix}" ${newline_position} -1 prefix_whitespace)
        endif()
        if(NOT prefix_whitespace MATCHES "^\n?[ \t]*$")
            string(FIND "${remaining}" "\n" rest_newline)
            if(rest_newline EQUAL -1)
                string(APPEND result "${remaining}")
                set(remaining "")
            else()
                string(SUBSTRING "${remaining}" 0 ${rest_newline} line)
                math(EXPR after_newline "${rest_newline} + 1")
                string(SUBSTRING "${remaining}" ${after_newline} -1 remaining)
                string(APPEND result "${line}\n")
            endif()
            continue()
        endif()
        string(SUBSTRING "${remaining}" ${include_position} -1 rest)
        string(REGEX MATCH "^[ \t]*#include[ \t]*[\"<]([^\"<>]+)[\">]" include_declaration "${rest}")
        if(NOT include_declaration)
            message(FATAL_ERROR "Malformed #include in ${path}")
        endif()
        set(include_path "${CMAKE_MATCH_1}")
        if(include_path MATCHES "^/")
            string(SUBSTRING "${include_path}" 1 -1 include_path)
            set(include_path "shaders/${include_path}")
        else()
            get_filename_component(current_directory "${path}" DIRECTORY)
            if(NOT current_directory STREQUAL "")
                set(include_path "${current_directory}/${include_path}")
            endif()
        endif()
        minecraft_normalize_embedded_pack_path(include_path "${include_path}")
        if(NOT EXISTS "${pack_dir}/${include_path}")
            message(FATAL_ERROR "Vanilla pack include missing: ${include_path}")
        endif()
        minecraft_expand_embedded_pack_file(include_content "${pack_dir}" "${include_path}" "${stack}")
        string(APPEND result "${prefix}${include_content}")
        string(FIND "${rest}" "\n" rest_newline)
        if(rest_newline EQUAL -1)
            set(remaining "")
        else()
            math(EXPR after_newline "${rest_newline} + 1")
            string(SUBSTRING "${rest}" ${after_newline} -1 remaining)
        endif()
    endwhile()
    set(${out} "${result}" PARENT_SCOPE)
endfunction()

function(minecraft_minify_embedded_shader out text)
    string(REGEX REPLACE "//[^\n]*" "" text "${text}")
    set(result "")
    set(remaining "${text}")
    while(TRUE)
        string(FIND "${remaining}" "\n" newline_position)
        if(newline_position EQUAL -1)
            set(line "${remaining}")
            set(remaining "")
        else()
            string(SUBSTRING "${remaining}" 0 ${newline_position} line)
            math(EXPR after_newline "${newline_position} + 1")
            string(SUBSTRING "${remaining}" ${after_newline} -1 remaining)
        endif()
        string(REGEX REPLACE "[ \t]+$" "" line "${line}")
        if(NOT line STREQUAL "")
            string(APPEND result "${line}\n")
        endif()
        if(remaining STREQUAL "")
            break()
        endif()
    endwhile()
    set(${out} "${result}" PARENT_SCOPE)
endfunction()

function(minecraft_generate_embedded_vanilla_pack output pack_dir)
    file(GLOB_RECURSE pack_files RELATIVE "${pack_dir}" "${pack_dir}/*")
    set(real_files)
    foreach(pack_file IN LISTS pack_files)
        if(NOT IS_DIRECTORY "${pack_dir}/${pack_file}")
            list(APPEND real_files "${pack_file}")
        endif()
    endforeach()
    if(NOT real_files)
        message(FATAL_ERROR "No files found in ${pack_dir}")
    endif()
    list(SORT real_files)
    set(generated "#include <string>\n#include <unordered_map>\n")
    string(APPEND generated "#include \"net/minecraft/client/render/shaderpack/VanillaPackEmbed.hpp\"\n")
    string(APPEND generated "namespace net::minecraft::client::render {\n")
    string(APPEND generated "const std::unordered_map<std::string, std::string>& VanillaPackEmbed::embeddedResources() {\n")
    string(APPEND generated " static const std::unordered_map<std::string, std::string> kResources = {\n")
    foreach(pack_file IN LISTS real_files)
        get_filename_component(extension "${pack_file}" EXT)
        if(extension STREQUAL ".vsh" OR extension STREQUAL ".fsh" OR extension STREQUAL ".gsh" OR
           extension STREQUAL ".csh" OR extension STREQUAL ".tcs" OR extension STREQUAL ".tes" OR extension STREQUAL ".glsl")
            minecraft_expand_embedded_pack_file(content "${pack_dir}" "${pack_file}" "")
            minecraft_minify_embedded_shader(content "${content}")
        else()
            file(READ "${pack_dir}/${pack_file}" content)
            string(REPLACE "\r\n" "\n" content "${content}")
            string(REPLACE "\r" "\n" content "${content}")
        endif()
        string(FIND "${content}" ")GLSL\"" terminator)
        if(NOT terminator EQUAL -1)
            message(FATAL_ERROR "Vanilla pack file ${pack_file} contains the raw-string terminator")
        endif()
        string(APPEND generated "  {\"${pack_file}\", R\"GLSL(${content})GLSL\"},\n")
    endforeach()
    string(APPEND generated " };\n return kResources;\n}\n}\n")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
    file(WRITE "${output}" "${generated}")
endfunction()

set(MINECRAFT_GLSL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/client/render/shaders/glsl")
set(MINECRAFT_EMBEDDED_GLSL_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedGlslSnippets.cpp")
minecraft_generate_embedded_glsl("${MINECRAFT_EMBEDDED_GLSL_SOURCE}" "${MINECRAFT_GLSL_DIR}")
set_source_files_properties("${MINECRAFT_EMBEDDED_GLSL_SOURCE}" PROPERTIES GENERATED TRUE)
list(APPEND MINECRAFT_CLIENT_SOURCES "${MINECRAFT_EMBEDDED_GLSL_SOURCE}")

set(MINECRAFT_VANILLA_PACK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/net/minecraft/client/render/shaders/glsl/vanilla")
set(MINECRAFT_EMBEDDED_VANILLA_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedVanillaPack.cpp")
minecraft_generate_embedded_vanilla_pack("${MINECRAFT_EMBEDDED_VANILLA_SOURCE}" "${MINECRAFT_VANILLA_PACK_DIR}")
set_source_files_properties("${MINECRAFT_EMBEDDED_VANILLA_SOURCE}" PROPERTIES GENERATED TRUE)
list(APPEND MINECRAFT_CLIENT_SOURCES "${MINECRAFT_EMBEDDED_VANILLA_SOURCE}")

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

function(minecraft_link_audio target)
    if (WIN32)
        target_link_libraries(${target} PRIVATE winmm)
    elseif (APPLE)
        target_link_libraries(${target} PRIVATE "-framework AudioUnit" "-framework AudioToolbox")
    elseif (UNIX)
        target_link_libraries(${target} PRIVATE ${CMAKE_DL_LIBS})
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
