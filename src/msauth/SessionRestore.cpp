#include "msauth/SessionRestore.hpp"

#include "msauth/AccountStorage.hpp"
#include "msauth/SecretProtection.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"

#include <atomic>
#include <mutex>
#include <thread>

namespace msauth {
namespace {

struct SavedAccountRestoreState {
    std::atomic<bool> running {false};
    std::atomic<bool> finished {false};
    std::mutex mutex;
    std::optional<AuthResult> result;
    std::thread worker;
};

SavedAccountRestoreState gSavedAccountRestore;

void restoreSavedAccountInto(net::minecraft::client::Minecraft& client)
{
    const std::filesystem::path runDirectory =
        net::minecraft::client::util::MinecraftDirectories::getRunDirectory();
    const std::optional<MicrosoftAccount> saved = loadAccount(runDirectory);
    if (!saved.has_value() || !saved->restorable()) {
        return;
    }
    const AuthResult restored = restoreFromRefreshToken(*saved);
    if (!restored.ok) {
        return;
    }
    applyAccount(restored.account, client.session);
    (void)saveAccount(runDirectory, restored.account);
}

void startSavedAccountRestoreWorker(const MicrosoftAccount& savedAccount)
{
    if (gSavedAccountRestore.running.load()) {
        return;
    }
    if (gSavedAccountRestore.worker.joinable()) {
        gSavedAccountRestore.worker.join();
    }

    gSavedAccountRestore.running = true;
    gSavedAccountRestore.finished = false;
    {
        std::lock_guard<std::mutex> lock(gSavedAccountRestore.mutex);
        gSavedAccountRestore.result.reset();
    }

    MicrosoftAccount account = savedAccount;
    gSavedAccountRestore.worker = std::thread([account = std::move(account)]() mutable {
        AuthResult restored = restoreFromRefreshToken(account);
        secret::wipeString(account.refreshToken);
        secret::wipeString(account.accessToken);
        {
            std::lock_guard<std::mutex> lock(gSavedAccountRestore.mutex);
            gSavedAccountRestore.result = std::move(restored);
        }
        gSavedAccountRestore.finished = true;
        gSavedAccountRestore.running = false;
    });
}

} // namespace

bool isAuthenticated(const net::minecraft::client::util::Session& session)
{
    return session.sessionId.rfind("msa:", 0) == 0 && !session.mpPass.empty();
}

void applyAccount(const MicrosoftAccount& account, net::minecraft::client::util::Session& session)
{
    session.username = account.profileName;
    session.sessionId = account.sessionId();
    secret::wipeString(session.mpPass);
    session.mpPass = account.accessToken;
}

void clearSession(net::minecraft::client::util::Session& session)
{
    session.username.clear();
    session.sessionId = "-";
    secret::wipeString(session.mpPass);
}

bool hasRestorableSavedAccount(const std::filesystem::path& runDirectory)
{
    const std::optional<MicrosoftAccount> saved = loadAccount(runDirectory);
    return saved.has_value() && saved->restorable();
}

std::optional<std::string> savedAccountProfileName(const std::filesystem::path& runDirectory)
{
    const std::optional<MicrosoftAccount> saved = loadAccount(runDirectory);
    if (!saved.has_value() || !saved->hasProfile()) {
        return std::nullopt;
    }
    return saved->profileName;
}

bool isSavedAccountRestorePending()
{
    return gSavedAccountRestore.running.load();
}

void tryApplySavedAccount(net::minecraft::client::Minecraft& client)
{
    restoreSavedAccountInto(client);
}

void beginRestoreSavedAccount(net::minecraft::client::Minecraft& client)
{
    if (isAuthenticated(client.session) || gSavedAccountRestore.running.load()) {
        return;
    }

    const std::filesystem::path runDirectory =
        net::minecraft::client::util::MinecraftDirectories::getRunDirectory();
    const std::optional<MicrosoftAccount> saved = loadAccount(runDirectory);
    if (!saved.has_value() || !saved->restorable()) {
        return;
    }

    startSavedAccountRestoreWorker(*saved);
}

void tickRestoreSavedAccount(net::minecraft::client::Minecraft& client)
{
    if (!gSavedAccountRestore.finished.exchange(false)) {
        return;
    }
    if (gSavedAccountRestore.worker.joinable()) {
        gSavedAccountRestore.worker.join();
    }

    std::optional<AuthResult> restored;
    {
        std::lock_guard<std::mutex> lock(gSavedAccountRestore.mutex);
        restored = std::move(gSavedAccountRestore.result);
        gSavedAccountRestore.result.reset();
    }

    if (!restored.has_value() || !restored->ok) {
        return;
    }

    applyAccount(restored->account, client.session);
    const std::filesystem::path runDirectory =
        net::minecraft::client::util::MinecraftDirectories::getRunDirectory();
    (void)saveAccount(runDirectory, restored->account);
}

} // namespace msauth
