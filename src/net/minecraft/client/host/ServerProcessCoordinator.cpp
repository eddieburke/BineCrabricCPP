#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif
#include "net/minecraft/client/host/ServerProcessCoordinator.hpp"
#include <sstream>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/core/WorldSession.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
namespace net::minecraft::client::host {
namespace {
#ifdef _WIN32
std::wstring quoteArg(const std::wstring& value) {
  std::wstring result = L"\"";
  std::size_t slashes = 0;
  for(wchar_t character : value) {
    if(character == L'\\') {
      ++slashes;
    } else if(character == L'\"') {
      result.append(slashes * 2 + 1, L'\\');
      result.push_back(character);
      slashes = 0;
    } else {
      result.append(slashes, L'\\');
      slashes = 0;
      result.push_back(character);
    }
  }
  result.append(slashes * 2, L'\\');
  result.push_back(L'\"');
  return result;
}
std::filesystem::path serverExecutable() {
  std::wstring buffer(32768, L'\0');
  const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  buffer.resize(length);
  return std::filesystem::path(buffer).parent_path() / "minecraft_server.exe";
}
#endif
} // namespace
ServerProcessCoordinator::ServerProcessCoordinator(client::Minecraft* minecraft) : minecraft_(minecraft) {
}
ServerProcessCoordinator::~ServerProcessCoordinator() {
  shutdown();
}
bool ServerProcessCoordinator::canHostWorld(const World* world) {
  if(world == nullptr || world->isRemote()) {
    return false;
  }
  const WorldStorage* storage = world->getDimensionData();
  return storage != nullptr && !storage->worldDirectory().empty() && !storage->worldName().empty();
}
bool ServerProcessCoordinator::canStartServer() const {
  return minecraft_ != nullptr && state_ == State::Inactive && canHostWorld(minecraft_->world);
}
bool ServerProcessCoordinator::isStarting() const noexcept {
  return state_ == State::Starting;
}
bool ServerProcessCoordinator::isAwaitingLoopback() const noexcept {
  return state_ == State::AwaitingLoopback;
}
bool ServerProcessCoordinator::isActive() const noexcept {
  return state_ == State::Active;
}
bool ServerProcessCoordinator::isHostedWorld(const World* world) const noexcept {
  return state_ == State::Active && world != nullptr && world == remoteWorld_;
}
std::uint16_t ServerProcessCoordinator::port() const noexcept {
  return settings_.port;
}
const ServerConnectionInfo& ServerProcessCoordinator::connectionInfo() const noexcept {
  return connectionInfo_;
}
const std::string& ServerProcessCoordinator::lastError() const noexcept {
  return lastError_;
}
bool ServerProcessCoordinator::fail(const std::string& error, std::string& errorOut) {
  lastError_ = error;
  errorOut = error;
  return false;
}
bool ServerProcessCoordinator::captureWorld(World& world, std::string& errorOut) {
  WorldStorage* storage = world.getDimensionData();
  if(storage == nullptr) {
    return fail("Current world storage is unavailable.", errorOut);
  }
  storageRoot_ = storage->worldDirectory().parent_path();
  worldName_ = storage->worldName();
  worldSeed_ = world.getSeed();
  if(storageRoot_.empty() || worldName_.empty()) {
    return fail("Could not resolve the current world's save directory.", errorOut);
  }
  return true;
}
bool ServerProcessCoordinator::launch(std::string& errorOut) {
#ifdef _WIN32
  const std::filesystem::path executable = serverExecutable();
  if(!std::filesystem::is_regular_file(executable)) {
    return fail("Dedicated server executable not found beside the client: " + executable.string(), errorOut);
  }
  SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
  HANDLE stdinRead = nullptr;
  if(!CreatePipe(&stdinRead, &stdinWrite_, &attributes, 0)) {
    return fail("Could not create dedicated server control pipe.", errorOut);
  }
  SetHandleInformation(stdinWrite_, HANDLE_FLAG_INHERIT, 0);
  std::wostringstream command;
  command << quoteArg(executable.wstring()) << L" nogui"
          << L" --storage-root " << quoteArg(storageRoot_.wstring())
          << L" --level-name " << quoteArg(std::filesystem::path(worldName_).wstring())
          << L" --level-seed " << worldSeed_
          << L" --server-ip " << quoteArg(L"")
          << L" --server-port " << settings_.port
          << L" --ready-file " << quoteArg(readyFile_.wstring())
          << L" --online-mode false"
          << L" --spawn-animals " << (settings_.spawnAnimals ? L"true" : L"false")
          << L" --pvp " << (settings_.pvpEnabled ? L"true" : L"false")
          << L" --allow-flight " << (settings_.flightEnabled ? L"true" : L"false")
          << L" --allow-nether " << (settings_.allowNether ? L"true" : L"false")
          << L" --mods-enabled " << (settings_.modsEnabled ? L"true" : L"false");
  std::wstring commandLine = command.str();
  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  startup.dwFlags = STARTF_USESTDHANDLES;
  startup.hStdInput = stdinRead;
  startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  PROCESS_INFORMATION processInfo{};
  const std::wstring workingDirectory = executable.parent_path().wstring();
  const BOOL created = CreateProcessW(executable.c_str(),
                                      commandLine.data(),
                                      nullptr,
                                      nullptr,
                                      TRUE,
                                      CREATE_NO_WINDOW,
                                      nullptr,
                                      workingDirectory.c_str(),
                                      &startup,
                                      &processInfo);
  CloseHandle(stdinRead);
  if(!created) {
    closeProcessHandles();
    return fail("Could not launch minecraft_server.exe (Windows error " + std::to_string(GetLastError()) + ").",
                errorOut);
  }
  process_ = processInfo.hProcess;
  processThread_ = processInfo.hThread;
  return true;
#else
  return fail("External dedicated-server hosting is unavailable on this platform.", errorOut);
#endif
}
bool ServerProcessCoordinator::start(const ServerProcessSettings& settings, std::string& errorOut) {
  errorOut.clear();
  lastError_.clear();
  if(!canStartServer()) {
    return fail("A local saved world is required and no managed server may already be running.", errorOut);
  }
  if(settings.port == 0) {
    return fail("External server port must be from 1 to 65535.", errorOut);
  }
  if(!captureWorld(*minecraft_->world, errorOut)) {
    return false;
  }
  settings_ = settings;
  readyFile_ = storageRoot_ / (".minecraft-server-ready-" + std::to_string(settings_.port));
  std::error_code readyFileError;
  std::filesystem::remove(readyFile_, readyFileError);
  if(!minecraft_->worldSession().parkLocalWorldForRemoteHandoff(*minecraft_)) {
    return fail("Could not save and release the world for dedicated-server transfer.", errorOut);
  }
  if(!launch(errorOut)) {
    minecraft_->worldSession().restoreParkedLocalWorld(*minecraft_);
    return false;
  }
  connectionInfo_ = ServerAddressResolver::resolve(settings_.port);
  state_ = State::Starting;
  return true;
}
bool ServerProcessCoordinator::processRunning() const {
#ifdef _WIN32
  return process_ != nullptr && WaitForSingleObject(process_, 0) == WAIT_TIMEOUT;
#else
  return false;
#endif
}
bool ServerProcessCoordinator::pollStart(std::string& errorOut) {
  errorOut.clear();
  if(state_ != State::Starting) {
    return false;
  }
  if(!processRunning()) {
    fail("Dedicated server exited before accepting connections.", errorOut);
    requestStop(true);
    return true;
  }
  if(!std::filesystem::is_regular_file(readyFile_)) {
    return false;
  }
  state_ = State::AwaitingLoopback;
  return true;
}
void ServerProcessCoordinator::requestStop(bool restoreLocalWorld) {
  restoreLocalWorld_ = restoreLocalWorld;
#ifdef _WIN32
  if(stdinWrite_ != nullptr) {
    static constexpr char stopCommand[] = "stop\r\n";
    DWORD written = 0;
    WriteFile(stdinWrite_, stopCommand, sizeof(stopCommand) - 1, &written, nullptr);
    CloseHandle(stdinWrite_);
    stdinWrite_ = nullptr;
  }
#endif
  state_ = State::Stopping;
}
void ServerProcessCoordinator::finishStop() {
  closeProcessHandles();
  if(restoreLocalWorld_ && minecraft_ != nullptr) {
    minecraft_->worldSession().restoreParkedLocalWorld(*minecraft_);
  }
  restoreLocalWorld_ = false;
  remoteWorld_ = nullptr;
  std::error_code readyFileError;
  std::filesystem::remove(readyFile_, readyFileError);
  readyFile_.clear();
  connectionInfo_ = {};
  state_ = State::Inactive;
}
void ServerProcessCoordinator::tick() {
  if(state_ == State::Stopping && !processRunning()) {
    finishStop();
  } else if(state_ == State::Active && !processRunning()) {
    lastError_ = "Dedicated server process stopped.";
    finishStop();
  }
}
void ServerProcessCoordinator::onConnectCanceledOrFailed(const std::string& error) {
  if(state_ != State::Starting && state_ != State::AwaitingLoopback) {
    return;
  }
  if(!error.empty()) {
    lastError_ = error;
  }
  requestStop(true);
}
void ServerProcessCoordinator::afterWorldChange(World* world) {
  if(state_ == State::AwaitingLoopback && world != nullptr && world->isRemote()) {
    remoteWorld_ = world;
    state_ = State::Active;
    minecraft_->worldSession().commitRemoteHandoff();
  } else if(state_ == State::Active && world != remoteWorld_) {
    requestStop(false);
  }
}
void ServerProcessCoordinator::closeProcessHandles() {
#ifdef _WIN32
  if(stdinWrite_ != nullptr) {
    CloseHandle(stdinWrite_);
    stdinWrite_ = nullptr;
  }
  if(processThread_ != nullptr) {
    CloseHandle(processThread_);
    processThread_ = nullptr;
  }
  if(process_ != nullptr) {
    CloseHandle(process_);
    process_ = nullptr;
  }
#endif
}
void ServerProcessCoordinator::shutdown() {
  if(processRunning()) {
    requestStop(false);
#ifdef _WIN32
    if(WaitForSingleObject(process_, 10000) == WAIT_TIMEOUT) {
      TerminateProcess(process_, 1);
      WaitForSingleObject(process_, 2000);
    }
#endif
  }
  finishStop();
}
} // namespace net::minecraft::client::host
