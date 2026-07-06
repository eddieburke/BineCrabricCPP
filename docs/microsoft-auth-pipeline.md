# Microsoft authentication pipeline

The in-game account flow follows the staged MultiMC pipeline rather than
treating Microsoft authentication as one opaque request.

| In-game stage | MultiMC reference | Result |
|---|---|---|
| Device code / refresh | `steps/MSAStep.cpp`, Katabasis `DeviceFlow` | Microsoft access and rotating refresh token |
| Xbox user login | `steps/XboxUserStep.cpp` | Xbox user token and user hash |
| Xbox authorization | `steps/XboxAuthorizationStep.cpp` with `http://xboxlive.com` | Xbox API token |
| Minecraft authorization | `steps/XboxAuthorizationStep.cpp` with `rp://api.minecraftservices.com/` | Minecraft-services XSTS token |
| Launcher login | `steps/LauncherLoginStep.cpp` | Minecraft access token and expiry |
| Xbox profile | `steps/XboxProfileStep.cpp` | Gamertag and account verification |
| Entitlements | `steps/EntitlementsStep.cpp` | Java Edition ownership |
| Minecraft profile | `steps/MinecraftProfileStep.cpp` | UUID, player name, active skin and cape |
| Profile creation | `steps/MinecraftProfileCreateStep.cpp` | First Java profile for an entitled account |

`MicrosoftAuth.cpp` reports each `AuthStage` to the title-screen login UI.
Interactive and silent refreshes use the same downstream Xbox/Minecraft stages.
Device polling honors the server interval, applies RFC `slow_down`, has a real
deadline, and is cancelable.

Unlike MultiMC, this client does not persist volatile Microsoft, Xbox, XSTS, or
Minecraft access tokens. Only the rotating Microsoft refresh token is stored,
protected with Windows DPAPI. Account writes use a replace-on-success temporary
file so a crash cannot truncate the only refresh credential.

Startup refresh is serialized with multiplayer connection. A token is never
redeemed concurrently by the startup worker and connector, and sign-out or a new
interactive login invalidates any unapplied startup result.
