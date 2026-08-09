#include "net/minecraft/client/multiplayer/ClientLoginNetworkHandler.hpp"
#include <chrono>
#include <utility>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/auth/microsoft/SecretProtection.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/client/gui/screen/DisconnectedScreen.hpp"
#include "net/minecraft/client/gui/screen/ServerModDownloadScreen.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkBridge.hpp"
#include "net/minecraft/client/session/OfflineIdentity.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/runtime/ModPackageIo.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include "net/minecraft/network/HandshakeMetadata.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/util/http/HttpClient.hpp"

namespace net::minecraft::client::multiplayer {

namespace http = ::net::minecraft::util::http;

namespace {
LuaModSyncPacket makeLuaModListPacket(const std::string& csv) {
  LuaModSyncPacket packet;
  packet.kind = LuaModSyncKind::ClientModList;
  packet.payload.assign(csv.begin(), csv.end());
  return packet;
}
} // namespace

ClientLoginNetworkHandler::ClientLoginNetworkHandler(client::Minecraft* minecraft, ClientNetworkBridge* bridge)
    : minecraft(minecraft), bridge_(bridge) {
}

ClientLoginNetworkHandler::~ClientLoginNetworkHandler() {
  joinServerWork_->cancelled = true;
  joinServerWork_->completed.request_stop();
  if(pendingModWork_ != nullptr) {
    pendingModWork_->cancelled = true;
    pendingModWork_->events.request_stop();
  }
}

void ClientLoginNetworkHandler::processPendingJoinServer() {
  if(minecraft == nullptr || disconnected) {
    return;
  }
  auth::JoinServerResult result;
  if(!joinServerWork_->completed.tryPop(result)) {
    return;
  }
  if(result.ok) {
    sendPacket(LoginHelloPacket{::net::minecraft::client::session::resolveJoinUsername(minecraft->session),
                                kProtocolVersionBeta173});
    return;
  }
  if(connection_ == nullptr) {
    return;
  }
  if(!result.error.empty()) {
    connection_->disconnect("disconnect.genericReason", {"Internal client error: " + result.error});
    return;
  }
  connection_->disconnect("disconnect.loginFailedInfo", {result.responseLine});
}

void ClientLoginNetworkHandler::tick() {
  processPendingJoinServer();
  if(disconnected || connection_ == nullptr) {
    return;
  }
  // A remote (Java-MP) server streams chunk data in bursts; cap the per-tick
  // drain so a large join backlog cannot freeze the frame loop.
  const Connection::DrainLimit drainLimit{std::chrono::steady_clock::now() + std::chrono::milliseconds(3), 100};
  connection_->setDrainLimit(drainLimit);
  connection_->tick();
  connection_->clearDrainLimit();
  connection_->interrupt();
}

void ClientLoginNetworkHandler::disconnect(const std::string& reason) {
  (void)reason;
  disconnected = true;
  joinServerWork_->cancelled = true;
}

void ClientLoginNetworkHandler::onDisconnected(const std::string& reason, const std::vector<std::string>& objects) {
  if(disconnected || minecraft == nullptr) {
    return;
  }
  disconnected = true;
  joinServerWork_->cancelled = true;
  minecraft->setWorld(nullptr);
  minecraft->setScreen(std::make_unique<client::gui::screen::DisconnectedScreen>("disconnect.lost", reason, objects));
}

void ClientLoginNetworkHandler::onHandshake(const HandshakePacket& packet) {
  if(minecraft == nullptr || disconnected) {
    return;
  }
  if(!minecraft->options.modsEnabled) {
    remoteServerKind_ = RemoteServerKind::JavaCompatible;
    remoteLuaModsEnabled_ = false;
    pendingMissingMods_.clear();
    pendingRequiredMods_.clear();
    pendingDownloadUrls_.clear();
    waitingForModDownloadAcceptance_ = false;
    const network::HandshakeMetadata metadata = network::parseHandshakeMetadata(packet.name);
    beginPendingLogin(metadata.serverId);
    return;
  }

  pendingMissingMods_.clear();
  pendingRequiredMods_.clear();
  pendingDownloadUrls_.clear();
  waitingForModDownloadAcceptance_ = false;

  const network::HandshakeMetadata metadata = network::parseHandshakeMetadata(packet.name);
  remoteServerKind_ = metadata.nativeCppMods ? RemoteServerKind::NativeCppMods : RemoteServerKind::JavaCompatible;
  remoteLuaModsEnabled_ = metadata.nativeCppMods && metadata.luaModsEnabled;

  if(!metadata.requiredMods.empty()) {
    const std::vector<std::string> missing = mod::runtime::WorldRequiredMods::missingMods(metadata.requiredMods);
    if(!missing.empty()) {
      pendingMissingMods_ = missing;
      pendingRequiredMods_ = metadata.requiredMods;
      pendingDownloadUrls_ = metadata.downloadUrls;

      bool canDownloadAll = metadata.nativeCppMods;
      for(const std::string& modId : missing) {
        if(!pendingDownloadUrls_.contains(modId)) {
          canDownloadAll = false;
          break;
        }
      }

      if(canDownloadAll) {
        waitingForModDownloadAcceptance_ = true;
        pendingServerId_ = metadata.serverId;
        minecraft->setScreen(std::make_unique<client::gui::screen::ServerModDownloadScreen>(this, missing));
        return;
      }
      
      disconnected = true;
      joinServerWork_->cancelled = true;
      minecraft->setWorld(nullptr);
      minecraft->setScreen(std::make_unique<client::gui::screen::DisconnectedScreen>(
          "disconnect.disconnected",
          "disconnect.genericReason",
          std::vector<std::string>{mod::runtime::WorldRequiredMods::requirementMessage(missing)}));
      return;
    }
  }

  if(remoteServerKind_ == RemoteServerKind::NativeCppMods) {
    sendPacket(makeLuaModListPacket(mod::runtime::WorldRequiredMods::joinCsv(activeClientMods())));
  }

  beginPendingLogin(metadata.serverId);
}

void ClientLoginNetworkHandler::onHello(const LoginHelloPacket& packet) {
  if(disconnected || minecraft == nullptr || bridge_ == nullptr) {
    return;
  }
  const bool remoteLuaModsEnabled = packet.protocolVersion == kProtocolVersionNativeCppMods;

  auto playHandler = std::make_unique<ClientNetworkHandler>(minecraft);
  playHandler->onHello(packet.worldSeed, packet.dimensionId, packet.protocolVersion, remoteLuaModsEnabled);
  bridge_->setHandler(std::move(playHandler));
}

void ClientLoginNetworkHandler::onDisconnect(const DisconnectPacket& packet) {
  disconnected = true;
  joinServerWork_->cancelled = true;
  if(minecraft != nullptr) {
    minecraft->setWorld(nullptr);
    minecraft->setScreen(std::make_unique<client::gui::screen::DisconnectedScreen>(
        "disconnect.disconnected", "disconnect.genericReason", std::vector<std::string>{packet.reason}));
  }
}

void ClientLoginNetworkHandler::onKeepAlive(const KeepAlivePacket& packet) {
  sendPacket(packet);
}

std::vector<std::string> ClientLoginNetworkHandler::activeClientMods() const {
  std::vector<std::string> activeMods;
  for(const mod::runtime::ModPackage& pkg : mod::runtime::host().packageMods()) {
    if(pkg.active) {
      activeMods.push_back(pkg.id);
    }
  }
  return activeMods;
}

void ClientLoginNetworkHandler::beginPendingLogin(const std::string& serverId) {
  pendingServerId_ = serverId;
  if(serverId == "-") {
    sendPacket(LoginHelloPacket{::net::minecraft::client::session::resolveJoinUsername(minecraft->session),
                                kProtocolVersionBeta173});
    return;
  }
  const client::util::Session session = minecraft->session;
  const auto work = joinServerWork_;
  if(work->inFlight.exchange(true, std::memory_order_acq_rel)) {
    return;
  }
  work->cancelled = false;
  net::minecraft::util::concurrent::ThreadCoordinator::instance()
      .pool(net::minecraft::util::concurrent::Domain::Io)
      .submit([work, session = std::move(session), serverId]() mutable {
        auth::JoinServerResult result = auth::verifyJoinServer(session, serverId, &work->cancelled);
        msauth::secret::wipeString(session.mpPass);
        if(!work->cancelled.load(std::memory_order_acquire)) {
          work->completed.push(std::move(result));
        }
        work->inFlight.store(false, std::memory_order_release);
      });
}

bool ClientLoginNetworkHandler::startPendingModDownload() {
  if(!waitingForModDownloadAcceptance_) {
    return false;
  }
  if(pendingModWork_ != nullptr) {
    return true;
  }
  pendingModWork_ = std::make_shared<PendingModWork>();
  net::minecraft::util::concurrent::ThreadCoordinator::instance()
      .pool(net::minecraft::util::concurrent::Domain::Io)
      .submit([work = pendingModWork_,
               missing = pendingMissingMods_,
               urls = pendingDownloadUrls_,
               temp = std::filesystem::temp_directory_path() / "minecraft_omega_mod_downloads",
               modsDirectory = mod::runtime::host().modsDirectory()]() mutable {
        downloadPendingMods(work, std::move(missing), std::move(urls), temp, modsDirectory);
      });
  return true;
}

ClientLoginNetworkHandler::PendingModDownloadState ClientLoginNetworkHandler::pollPendingModDownload(
    std::string& status,
    std::string& error) {
  if(pendingModWork_ == nullptr) {
    return PendingModDownloadState::Idle;
  }
  PendingModEvent event;
  while(pendingModWork_->events.tryPop(event)) {
    if(event.kind == PendingModEvent::Kind::Failed) {
      error = event.text;
      return PendingModDownloadState::Failed;
    }
    if(event.kind == PendingModEvent::Kind::Progress) {
      status = event.text;
    }
    if(event.kind == PendingModEvent::Kind::Complete) {
      for(const PendingModFile& file : event.files) {
        std::error_code ec;
        std::filesystem::rename(file.temporary, file.destination, ec);
      }
      mod::runtime::host().rescan();
      return PendingModDownloadState::Succeeded;
    }
  }
  return PendingModDownloadState::Running;
}

void ClientLoginNetworkHandler::continuePendingLogin() {
  if(!waitingForModDownloadAcceptance_) {
    return;
  }
  waitingForModDownloadAcceptance_ = false;
  if(remoteServerKind_ == RemoteServerKind::NativeCppMods) {
    sendPacket(makeLuaModListPacket(mod::runtime::WorldRequiredMods::joinCsv(activeClientMods())));
  }
  beginPendingLogin(pendingServerId_);
}

void ClientLoginNetworkHandler::cancelPendingModPrompt() {
  waitingForModDownloadAcceptance_ = false;
  pendingMissingMods_.clear();
  pendingRequiredMods_.clear();
  pendingDownloadUrls_.clear();
  if(pendingModWork_ != nullptr) {
    pendingModWork_->cancelled = true;
    pendingModWork_ = nullptr;
  }
}

void ClientLoginNetworkHandler::downloadPendingMods(const std::shared_ptr<PendingModWork>& work,
                                                    std::vector<std::string> missing,
                                                    std::unordered_map<std::string, std::string> urls,
                                                    std::filesystem::path temporaryDirectory,
                                                    std::filesystem::path modsDirectory) {
  work->running = true;
  std::vector<PendingModFile> files;
  std::error_code filesystemError;
  std::filesystem::create_directories(temporaryDirectory, filesystemError);
  if(filesystemError) {
    work->events.push(PendingModEvent{PendingModEvent::Kind::Failed, "Could not create mod download directory", {}});
    work->running = false;
    return;
  }
  for(const std::string& modId : missing) {
    if(work->cancelled.load(std::memory_order_acquire)) {
      work->running = false;
      return;
    }
    work->events.tryPush(PendingModEvent{PendingModEvent::Kind::Progress, "Downloading " + modId + "...", {}});
    const auto url = urls.find(modId);
    if(url == urls.end()) {
      work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                        "No download URL available for required mod " + modId, {}});
      work->running = false;
      return;
    }
    http::HttpRequest request;
    request.url = url->second;
    request.maxResponseBytes = static_cast<std::size_t>(mod::lua::kMaxModArchiveBytes);
    const http::HttpResponse response = http::httpRequest(request);
    if(!response.ok() || response.body.empty()) {
      work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                        "Failed to download required mod " + modId, {}});
      work->running = false;
      return;
    }
    const std::filesystem::path temporary = temporaryDirectory / (mod::lua::sanitizeName(modId) + ".zip.tmp");
    std::vector<mod::runtime::ZipEntry> entries;
    if(!mod::runtime::buildZipIndex(response.body, entries)) {
      work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                        "Downloaded file for " + modId + " is not a valid mod zip", {}});
      work->running = false;
      return;
    }
    const mod::runtime::ZipEntry* manifestEntry = mod::runtime::findZipEntry(entries, "mod.json");
    if(manifestEntry == nullptr) {
      work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                        "Downloaded file for " + modId + " is missing mod.json", {}});
      work->running = false;
      return;
    }
    mod::runtime::ModPackage package;
    const std::vector<std::uint8_t> manifest = mod::runtime::readZipEntryData(response.body, *manifestEntry);
    if(!mod::runtime::parseManifestJson(std::string(manifest.begin(), manifest.end()),
                                        package,
                                        temporary,
                                        mod::runtime::ModPackageSource::Zip,
                                        "mod.json: ") ||
       package.id != modId) {
      work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                        "Downloaded file does not match required mod " + modId, {}});
      work->running = false;
      return;
    }
    if(!mod::lua::writeFileBytes(temporary, response.body)) {
      work->events.push(PendingModEvent{PendingModEvent::Kind::Failed,
                                        "Could not write temporary download for " + modId, {}});
      work->running = false;
      return;
    }
    files.push_back(PendingModFile{temporary, modsDirectory / (mod::lua::sanitizeName(modId) + ".zip")});
  }
  work->events.push(PendingModEvent{PendingModEvent::Kind::Complete, "Installing...", std::move(files)});
  work->running = false;
}

} // namespace net::minecraft::client::multiplayer
