#include "net/minecraft/client/lifecycle/ClientInitializer.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/SingleplayerInteractionManager.hpp"
#include "net/minecraft/client/color/world/FoliageColors.hpp"
#include "net/minecraft/client/color/world/GrassColors.hpp"
#include "net/minecraft/client/color/world/WaterColors.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/gui/screen/ConnectScreen.hpp"
#include "net/minecraft/client/gui/screen/TitleScreen.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/OpenGlCapabilities.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereRenderer.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/client/render/texture/ClockSprite.hpp"
#include "net/minecraft/client/render/texture/CompassSprite.hpp"
#include "net/minecraft/client/render/texture/FireSprite.hpp"
#include "net/minecraft/client/render/texture/LavaSideSprite.hpp"
#include "net/minecraft/client/render/texture/LavaSprite.hpp"
#include "net/minecraft/client/render/texture/NetherPortalSprite.hpp"
#include "net/minecraft/client/render/texture/WaterSideSprite.hpp"
#include "net/minecraft/client/render/texture/WaterSprite.hpp"
#include "net/minecraft/client/render/LoadingScreenRenderer.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/resource/language/TranslationStorage.hpp"
#include "net/minecraft/client/resource/pack/TexturePacks.hpp"
#include "net/minecraft/client/util/DisplayManager.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "msauth/SessionRestore.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/world/storage/RegionWorldStorageSource.hpp"
#include "net/minecraft/client/resource/ResourcePack.hpp"

#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <dbghelp.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>

#endif

namespace net::minecraft::client::lifecycle {

namespace {

const char* gStartupPhase = "before main";

#ifdef _WIN32
std::string executableDirectory()
{
    char path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return ".";
    }
    std::string directory(path, length);
    const std::size_t slash = directory.find_last_of("\\/");
    if (slash != std::string::npos) {
        directory.resize(slash);
    }
    return directory;
}

void ensureConsole()
{
    if (GetConsoleWindow() != nullptr) {
        return;
    }
    if (!AllocConsole()) {
        return;
    }
    FILE* stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);
    std::ios::sync_with_stdio();
}

void writeCrashReportFile(const std::string& details)
{
    const std::string path = executableDirectory() + "\\crash-report.txt";
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Failed to write crash report to " << path << '\n';
        return;
    }
    file << details;
    std::cerr << "Crash report written to " << path << '\n';
}

void showCrashMessageBox(const std::string& title, const std::string& details)
{
    std::string message = details;
    constexpr std::size_t kMaxMessageBoxChars = 2000;
    if (message.size() > kMaxMessageBoxChars) {
        message.resize(kMaxMessageBoxChars);
        message += "\n\n(truncated - see crash-report.txt)";
    }
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

std::string describeModuleAtAddress(const void* address)
{
    if (address == nullptr) {
        return "unknown module";
    }
    HMODULE module = nullptr;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(address),
            &module)
        || module == nullptr) {
        return "unknown module";
    }
    char path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(module, path, MAX_PATH);
    std::string description = length > 0 ? std::string(path, length) : "unknown module";
    const auto moduleBase = reinterpret_cast<std::uintptr_t>(module);
    const auto fault = reinterpret_cast<std::uintptr_t>(address);
    description += " +0x";
    std::ostringstream offset;
    offset << std::hex << (fault - moduleBase);
    description += offset.str();
    return description;
}

std::string captureStackTrace()
{
    void* frames[32] = {};
    const USHORT count = CaptureStackBackTrace(0, static_cast<DWORD>(std::size(frames)), frames, nullptr);
    if (count == 0) {
        return "  (no stack frames captured)\n";
    }
    std::ostringstream stream;
    stream << "Stack trace:\n";
    for (USHORT i = 0; i < count; ++i) {
        stream << "  #" << i << ' ' << frames[i] << ' ' << describeModuleAtAddress(frames[i]) << '\n';
    }
    return stream.str();
}

void appendRegisterDump(std::ostringstream& stream, const CONTEXT* context)
{
    if (context == nullptr) {
        return;
    }
    const auto reg = [&stream](const char* name, DWORD64 value) {
        stream << "  " << name << " = 0x" << std::hex << value << std::dec
               << " (" << describeModuleAtAddress(reinterpret_cast<const void*>(value)) << ")\n";
    };
    stream << "Registers:\n";
    reg("rip", context->Rip);
    reg("rsp", context->Rsp);
    reg("rbp", context->Rbp);
    reg("rax", context->Rax);
    reg("rbx", context->Rbx);
    reg("rcx", context->Rcx);
    reg("rdx", context->Rdx);
    reg("rsi", context->Rsi);
    reg("rdi", context->Rdi);
    reg("r8 ", context->R8);
    reg("r9 ", context->R9);
    reg("r10", context->R10);
    reg("r11", context->R11);
    reg("r12", context->R12);
    reg("r13", context->R13);
    reg("r14", context->R14);
    reg("r15", context->R15);
}

void writeMinidump(EXCEPTION_POINTERS* info)
{
    const std::string path = executableDirectory() + "\\crash.dmp";
    const HANDLE file = CreateFileA(
        path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open " << path << " for minidump\n";
        return;
    }
    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo {};
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = info;
    exceptionInfo.ClientPointers = FALSE;
    const auto dumpType = static_cast<MINIDUMP_TYPE>(
        MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo);
    const BOOL ok = MiniDumpWriteDump(
        GetCurrentProcess(), GetCurrentProcessId(), file, dumpType,
        info != nullptr ? &exceptionInfo : nullptr, nullptr, nullptr);
    CloseHandle(file);
    if (ok) {
        std::cerr << "Minidump written to " << path << '\n';
    } else {
        std::cerr << "MiniDumpWriteDump failed: " << GetLastError() << '\n';
    }
}

std::string formatUnhandledException(EXCEPTION_POINTERS* info)
{
    std::ostringstream stream;
    stream << "Unhandled native exception.\n";
    stream << "Startup phase: " << (gStartupPhase != nullptr ? gStartupPhase : "(null)") << '\n';
    stream << "Faulting thread: " << GetCurrentThreadId() << '\n';
    if (info != nullptr && info->ExceptionRecord != nullptr) {
        const EXCEPTION_RECORD* record = info->ExceptionRecord;
        stream << "Exception code: 0x" << std::hex << record->ExceptionCode << std::dec << '\n';
        stream << "Fault address: " << record->ExceptionAddress << '\n';
        stream << "Fault module: " << describeModuleAtAddress(record->ExceptionAddress) << '\n';
        if (record->NumberParameters >= 2 && record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
            stream << "Access type: " << (record->ExceptionInformation[0] == 0 ? "read" : "write") << '\n';
            stream << "Target address: 0x" << std::hex << record->ExceptionInformation[1] << std::dec << '\n';
        }
    }
    if (info != nullptr) {
        appendRegisterDump(stream, info->ContextRecord);
    }
    stream << captureStackTrace();
    return stream.str();
}

LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* info)
{
    writeMinidump(info);
    reportFatalError("Minecraft Native - fatal crash", formatUnhandledException(info));
    pauseBeforeExit();
    return EXCEPTION_EXECUTE_HANDLER;
}

[[noreturn]] void terminateHandler()
{
    const char* what = "unknown";
    try {
        const std::exception_ptr current = std::current_exception();
        if (current) {
            std::rethrow_exception(current);
        }
    } catch (const std::exception& ex) {
        what = ex.what();
    } catch (...) {
    }
    reportFatalError("Minecraft Native - std::terminate", what);
    pauseBeforeExit();
    std::abort();
}
#endif // _WIN32

} // namespace

void setStartupPhase(const char* phase)
{
    gStartupPhase = phase != nullptr ? phase : "(null)";
}

#ifdef _WIN32
void installCrashDiagnostics()
{
    ensureConsole();
    SetUnhandledExceptionFilter(unhandledExceptionFilter);
    std::set_terminate(terminateHandler);
}

void reportFatalError(const std::string& title, const std::string& details)
{
    std::cerr << title << '\n' << details << std::endl;
    writeCrashReportFile(details);
    showCrashMessageBox(title, details);
}

void pauseBeforeExit()
{
    if (GetConsoleWindow() != nullptr) {
        std::cerr << "\nPress Enter to exit...\n";
        std::cin.clear();
        std::cin.get();
        return;
    }
    MessageBoxA(
        nullptr,
        "The application has stopped. Press OK to exit.",
        "Minecraft Native",
        MB_OK | MB_SETFOREGROUND);
}
#endif // _WIN32

void ClientInitializer::bootstrap(Minecraft& client)
{
    setStartupPhase("init: directories");
    client.runDirectory_ = Minecraft::getRunDirectory();
    client.options.optionsFile = client.runDirectory_ / "options.txt";
    client.options.bindMinecraft(&client);
    option::OptionRegistry::registerAll();
    client.options.load();
    client.worldStorageSource = std::make_unique<net::minecraft::RegionWorldStorageSource>(client.runDirectory_ / "saves");
    const net::minecraft::ResourcePack resources(std::filesystem::path(MINECRAFT_NATIVE_RESOURCE_DIR));
    client.translationStorage_ = std::make_unique<resource::language::TranslationStorage>(resources);
    resource::language::I18n::setTranslations(client.translationStorage_.get());
    client.texturePacksStorage_ = std::make_unique<resource::pack::TexturePacks>(
        std::filesystem::path(MINECRAFT_NATIVE_RESOURCE_DIR), client.runDirectory_, &client.options, &client.textureManager);
    client.texturePacks = client.texturePacksStorage_.get();
    client.textureManager.setTexturePacks(client.texturePacks);
    setStartupPhase("init: text renderer");
    client.textRenderer = font::TextRenderer::create(client.options, client.textureManager, "font/default.png");
    color::world::WaterColors::setColorMap(client.textureManager.getColors("/misc/watercolor.png"));
    color::world::GrassColors::setColorMap(client.textureManager.getColors("/misc/grasscolor.png"));
    color::world::FoliageColors::setColorMap(client.textureManager.getColors("/misc/foliagecolor.png"));
    setStartupPhase("init: game renderer");
    client.gameRenderer = std::make_unique<render::GameRenderer>(&client);
    render::entity::EntityRenderDispatcher::instance().setHeldItemRenderer(
        std::make_unique<render::item::HeldItemRenderer>(&client));
    setStartupPhase("init: player stats");
    client.statsStorage_ = std::make_unique<stat::PlayerStats>(client.session, client.runDirectory_);
    client.stats = client.statsStorage_.get();
    setStartupPhase("init: loading screen");
    render::LoadingScreenRenderer::renderLoadingScreen(client);
#ifdef _WIN32
    input::InputSystem::init(util::DisplayManager::hwnd());
#endif
    util::DisplayManager::logGlError(client, "Pre startup");
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
    gl::GL11::glShadeModel(gl::GL11::GL_SMOOTH);
    gl::GL11::glClearDepth(1.0);
    gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glDepthFunc(gl::GL11::GL_LEQUAL);
    gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
    gl::GL11::glAlphaFunc(gl::GL11::GL_GREATER, 0.1f);
    gl::GL11::glCullFace(gl::GL11::GL_BACK);
    gl::GL11::glMatrixMode(gl::GL11::GL_PROJECTION);
    gl::GL11::glLoadIdentity();
    gl::GL11::glMatrixMode(gl::GL11::GL_MODELVIEW);
    util::DisplayManager::logGlError(client, "Startup");
    client.openGlCapabilities = new render::OpenGlCapabilities();
    client.audio.start(&client.options);
    client.textureManager.addDynamicTexture(&client.lavaSprite_);
    client.textureManager.addDynamicTexture(&client.waterSprite_);
    client.textureManager.addDynamicTexture(new render::texture::NetherPortalSprite());
    client.textureManager.addDynamicTexture(new render::texture::CompassSprite(client));
    client.textureManager.addDynamicTexture(new render::texture::ClockSprite(client));
    client.textureManager.addDynamicTexture(new render::texture::WaterSideSprite());
    client.textureManager.addDynamicTexture(new render::texture::LavaSideSprite());
    client.textureManager.addDynamicTexture(new render::texture::FireSprite(0));
    client.textureManager.addDynamicTexture(new render::texture::FireSprite(1));
    client.particleManager.setTextureManager(&client.textureManager);
    client.worldRenderer = std::make_unique<render::WorldRenderer>(&client, &client.textureManager);
    client.atmosphereRenderer = std::make_unique<render::atmosphere::AtmosphereRenderer>();
    client.atmosphereRenderer->rebuildStaticGeometry();
    gl::GL11::glViewport(0, 0, client.displayWidth, client.displayHeight);
    client.resourceDownloadThread = std::make_unique<resource::ResourceDownloadThread>(client.runDirectory_, &client);
    client.resourceDownloadThread->start();
    util::DisplayManager::logGlError(client, "Post startup");
    client.interactionManager = std::make_unique<SingleplayerInteractionManager>(&client);
    client.inGameHud.setClient(&client);
    client.toast.setClient(&client);
    setStartupPhase("init: title screen");
    if (!client.startupServerAddress_.empty()) {
        client.setScreen(std::make_unique<gui::screen::ConnectScreen>(&client, client.startupServerAddress_, client.startupServerPort));
    } else {
        client.setScreen(std::make_unique<gui::screen::TitleScreen>());
    }
    setStartupPhase("init: complete");
    msauth::tryApplySavedAccount(client);
}

} // namespace net::minecraft::client::lifecycle
