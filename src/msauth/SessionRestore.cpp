#include "msauth/SessionRestore.hpp"

#include "msauth/AccountStorage.hpp"
#include "msauth/SecretProtection.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"

namespace msauth {

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

void tryApplySavedAccount(net::minecraft::client::Minecraft& client)
{
    const std::filesystem::path runDirectory =
        net::minecraft::client::util::MinecraftDirectories::getRunDirectory();
    const std::optional<MicrosoftAccount> saved = loadAccount(runDirectory);
    if (!saved.has_value()) {
        return;
    }
    if (!saved->restorable()) {
        return;
    }
    const AuthResult restored = restoreFromRefreshToken(*saved);
    if (!restored.ok) {
        return;
    }
    applyAccount(restored.account, client.session);
    (void)saveAccount(runDirectory, restored.account);
}

} // namespace msauth
