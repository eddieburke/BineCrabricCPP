#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/client/auth/microsoft/AccountStorage.hpp"
#include "net/minecraft/client/auth/microsoft/PlayerTextures.hpp"
#include "net/minecraft/client/auth/microsoft/SecretProtection.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
namespace msauth {
namespace {
struct SavedAccountRestoreState {
  std::atomic<bool> running{false};
  std::atomic<bool> finished{false};
  std::atomic_bool canceled{false};
  std::atomic<AuthStage> stage{AuthStage::Idle};
  std::mutex mutex;
  std::optional<AuthResult> result;
  std::thread worker;
  ~SavedAccountRestoreState() {
    canceled = true;
    if(worker.joinable()) {
      worker.join();
    }
  }
};
std::mutex gLastRestoreErrorMutex;
std::optional<std::string> gLastRestoreError;
std::mutex gPendingProfileMutex;
std::optional<MicrosoftAccount> gPendingProfileAccount;
SavedAccountRestoreState gSavedAccountRestore;
void rememberRestoreError(const AuthResult& restored) {
  std::lock_guard<std::mutex> lock(gLastRestoreErrorMutex);
  if(restored.needsProfileCreation) {
    gLastRestoreError = "This account owns Minecraft but still needs a Java profile.";
  } else if(!restored.error.empty()) {
    gLastRestoreError = restored.error;
  } else {
    gLastRestoreError = "Could not refresh Microsoft session; sign in again";
  }
}
void clearRestoreError() {
  std::lock_guard<std::mutex> lock(gLastRestoreErrorMutex);
  gLastRestoreError.reset();
}
void rememberPendingProfile(const MicrosoftAccount& account) {
  std::lock_guard<std::mutex> lock(gPendingProfileMutex);
  if(gPendingProfileAccount.has_value()) {
    secret::wipeString(gPendingProfileAccount->accessToken);
    secret::wipeString(gPendingProfileAccount->refreshToken);
  }
  gPendingProfileAccount = account;
}
void startSavedAccountRestoreWorker(const MicrosoftAccount& savedAccount) {
  if(gSavedAccountRestore.running.load()) {
    return;
  }
  if(gSavedAccountRestore.worker.joinable()) {
    gSavedAccountRestore.worker.join();
  }
  gSavedAccountRestore.running = true;
  gSavedAccountRestore.finished = false;
  gSavedAccountRestore.canceled = false;
  gSavedAccountRestore.stage = AuthStage::RefreshingMicrosoftToken;
  {
    std::lock_guard<std::mutex> lock(gSavedAccountRestore.mutex);
    gSavedAccountRestore.result.reset();
  }
  MicrosoftAccount account = savedAccount;
  gSavedAccountRestore.worker = std::thread([account = std::move(account)]() mutable {
    const auto progress = [](AuthStage stage) {
      gSavedAccountRestore.stage.store(stage, std::memory_order_release);
    };
    AuthResult restored = restoreFromRefreshToken(account, &gSavedAccountRestore.canceled, progress);
    secret::wipeString(account.refreshToken);
    secret::wipeString(account.accessToken);
    if(restored.canceled) {
      // Cancellation is user intent, not an account failure.
    } else if(!restored.ok) {
      rememberRestoreError(restored);
    } else {
      clearRestoreError();
    }
    {
      std::lock_guard<std::mutex> lock(gSavedAccountRestore.mutex);
      gSavedAccountRestore.result = std::move(restored);
    }
    gSavedAccountRestore.finished = true;
    gSavedAccountRestore.running = false;
  });
}
} // namespace
bool isAuthenticated(const net::minecraft::client::util::Session& session) {
  const std::int64_t now =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  return session.sessionId.rfind("msa:", 0) == 0 && !session.mpPass.empty() &&
         (session.mpPassExpiresAt == 0 || session.mpPassExpiresAt > now + 60);
}
void applyAccount(const MicrosoftAccount& account, net::minecraft::client::util::Session& session) {
  if(account.accessToken.empty()) {
    return;
  }
  session.username = account.profileName;
  session.sessionId = account.sessionId();
  session.skinUrl = account.skinUrl;
  session.capeUrl = account.capeUrl;
  secret::wipeString(session.mpPass);
  session.mpPass = account.accessToken;
  session.mpPassExpiresAt = account.accessTokenExpiresAt;
}
void applyAccountToClient(net::minecraft::client::Minecraft& client, const MicrosoftAccount& account) {
  applyAccount(account, client.session);
  refreshPlayerTextures(client);
}
void clearSession(net::minecraft::client::util::Session& session) {
  session.username.clear();
  session.sessionId = "-";
  session.skinUrl.clear();
  session.capeUrl.clear();
  secret::wipeString(session.mpPass);
  session.mpPassExpiresAt = 0;
}
bool isSavedAccountRestorePending() {
  return gSavedAccountRestore.running.load() || gSavedAccountRestore.finished.load();
}
AuthStage savedAccountRestoreStage() {
  return gSavedAccountRestore.stage.load(std::memory_order_acquire);
}
void cancelSavedAccountRestore() {
  gSavedAccountRestore.canceled = true;
  clearRestoreError();
  {
    std::lock_guard<std::mutex> lock(gPendingProfileMutex);
    if(gPendingProfileAccount.has_value()) {
      secret::wipeString(gPendingProfileAccount->accessToken);
      secret::wipeString(gPendingProfileAccount->refreshToken);
    }
    gPendingProfileAccount.reset();
  }
  if(!gSavedAccountRestore.running.load() && gSavedAccountRestore.worker.joinable()) {
    gSavedAccountRestore.worker.join();
  }
  {
    std::lock_guard<std::mutex> lock(gSavedAccountRestore.mutex);
    gSavedAccountRestore.result.reset();
  }
  gSavedAccountRestore.finished = false;
  gSavedAccountRestore.stage = AuthStage::Idle;
}
bool ensureAuthenticatedForJoin(net::minecraft::client::Minecraft& client, const std::atomic_bool* canceled) {
  if(isAuthenticated(client.session)) {
    return true;
  }
  const std::filesystem::path runDirectory = net::minecraft::client::util::MinecraftDirectories::getRunDirectory();
  const std::optional<MicrosoftAccount> saved = loadAccount(runDirectory);
  if(!saved.has_value() || !saved->restorable()) {
    // No saved account: allow an offline join under whatever name is set.
    return true;
  }
  // Wait out any in-flight async startup restore so we do not redeem the rotating MSA
  // refresh token twice at once (one redemption invalidates the other's token).
  while(gSavedAccountRestore.running.load()) {
    if(canceled != nullptr && canceled->load(std::memory_order_acquire)) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
  // The async worker may have finished with a result the main-loop tick has not applied
  // yet. Claim it here (whoever wins the exchange owns the join) and apply it.
  if(gSavedAccountRestore.finished.exchange(false)) {
    if(gSavedAccountRestore.worker.joinable()) {
      gSavedAccountRestore.worker.join();
    }
    std::optional<AuthResult> restored;
    {
      std::lock_guard<std::mutex> lock(gSavedAccountRestore.mutex);
      restored = std::move(gSavedAccountRestore.result);
      gSavedAccountRestore.result.reset();
    }
    if(restored.has_value() && restored->ok) {
      applyAccount(restored->account, client.session);
      if(saveAccount(runDirectory, restored->account)) {
        clearRestoreError();
      } else {
        AuthResult saveFailure;
        saveFailure.error = "Microsoft account refreshed, but the rotated refresh token could not be saved.";
        rememberRestoreError(saveFailure);
      }
      return true;
    } else if(restored.has_value() && !restored->ok && !restored->canceled) {
      if(restored->needsProfileCreation) {
        rememberPendingProfile(restored->account);
      }
      rememberRestoreError(*restored);
      return false;
    } else if(restored.has_value() && restored->canceled) {
      return false;
    }
  }
  if(isAuthenticated(client.session)) {
    return true;
  }
  // No usable async result: refresh synchronously on this (worker) thread.
  const AuthResult restored = restoreFromRefreshToken(*saved, canceled);
  if(restored.canceled) {
    return false;
  }
  if(!restored.ok) {
    if(restored.needsProfileCreation) {
      rememberPendingProfile(restored.account);
    }
    rememberRestoreError(restored);
    return false;
  }
  applyAccount(restored.account, client.session);
  if(!saveAccount(runDirectory, restored.account)) {
    AuthResult saveFailure;
    saveFailure.error = "Microsoft account refreshed, but the rotated refresh token could not be saved.";
    rememberRestoreError(saveFailure);
  } else {
    clearRestoreError();
  }
  return isAuthenticated(client.session);
}
void beginRestoreSavedAccount(net::minecraft::client::Minecraft& client) {
  if(isAuthenticated(client.session) || gSavedAccountRestore.running.load()) {
    return;
  }
  const std::filesystem::path runDirectory = net::minecraft::client::util::MinecraftDirectories::getRunDirectory();
  const std::optional<MicrosoftAccount> saved = loadAccount(runDirectory);
  if(!saved.has_value() || !saved->restorable()) {
    return;
  }
  startSavedAccountRestoreWorker(*saved);
}
void tickRestoreSavedAccount(net::minecraft::client::Minecraft& client) {
  if(!gSavedAccountRestore.finished.exchange(false)) {
    return;
  }
  if(gSavedAccountRestore.worker.joinable()) {
    gSavedAccountRestore.worker.join();
  }
  std::optional<AuthResult> restored;
  {
    std::lock_guard<std::mutex> lock(gSavedAccountRestore.mutex);
    restored = std::move(gSavedAccountRestore.result);
    gSavedAccountRestore.result.reset();
  }
  if(!restored.has_value() || !restored->ok) {
    if(restored.has_value() && !restored->canceled) {
      if(restored->needsProfileCreation) {
        rememberPendingProfile(restored->account);
      }
      rememberRestoreError(*restored);
    }
    return;
  }
  gSavedAccountRestore.stage = AuthStage::Complete;
  clearRestoreError();
  applyAccountToClient(client, restored->account);
  const std::filesystem::path runDirectory = net::minecraft::client::util::MinecraftDirectories::getRunDirectory();
  if(!saveAccount(runDirectory, restored->account)) {
    AuthResult saveFailure;
    saveFailure.error = "Microsoft account refreshed, but the rotated refresh token could not be saved.";
    rememberRestoreError(saveFailure);
  }
}
std::optional<std::string> lastSavedAccountRestoreError() {
  std::lock_guard<std::mutex> lock(gLastRestoreErrorMutex);
  return gLastRestoreError;
}
std::optional<MicrosoftAccount> pendingProfileCreationAccount() {
  std::lock_guard<std::mutex> lock(gPendingProfileMutex);
  return gPendingProfileAccount;
}
bool hasPendingProfileCreationAccount() {
  std::lock_guard<std::mutex> lock(gPendingProfileMutex);
  return gPendingProfileAccount.has_value();
}
} // namespace msauth
