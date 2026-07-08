#include "net/minecraft/client/gui/auth/LoginScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/auth/microsoft/AccountStorage.hpp"
#include "net/minecraft/client/auth/microsoft/SecretProtection.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/platform/Browser.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"

namespace net::minecraft::client::gui::auth {
LoginScreen::LoginScreen(gui::screen::ScreenFactory returnFactory) : returnFactory_(std::move(returnFactory)) {
    if (std::optional<msauth::MicrosoftAccount> account = msauth::pendingProfileCreationAccount()) {
        phase_ = Phase::CreateProfile;
        pendingProfileAccount_ = std::move(*account);
        statusLine1_ = "This account has no Minecraft profile yet.";
        statusLine2_ = "Choose a username to create one.";
    }
}

LoginScreen::~LoginScreen() {
    cancelSignIn();
}

void LoginScreen::init() {
    buttons_.clear();
    signInButton_ = nullptr;
    openBrowserButton_ = nullptr;
    createProfileButton_ = nullptr;
    const bool resizing = !firstInit_;
    firstInit_ = false;
    clientId_ = msauth::loadMicrosoftClientId(util::MinecraftDirectories::getRunDirectory());
    if (!resizing && phase_ == Phase::Idle) {
        statusLine1_ = "Sign in with your Microsoft account.";
        statusLine2_ = "Your browser will open with a one-time code.";
        errorMessage_.clear();
        userCode_.clear();
        loginUrl_.clear();
    }
    const int top = height() / 4;
    if (phase_ == Phase::Idle || phase_ == Phase::Failed) {
        signInButton_ = &addCenteredActionButton(top + 78, "Sign in with Microsoft", [this] { beginSignIn(); });
        signInButton_->active = !workerRunning_.load();
        addCenteredActionButton(top + 102, "Cancel", [this] { navigateTo(returnFactory_); });
        return;
    }
    if (phase_ == Phase::RequestingCode) {
        addCenteredActionButton(top + 102, "Cancel", [this] {
            cancelSignIn();
            navigateTo(returnFactory_);
        });
        return;
    }
    if (phase_ == Phase::CreateProfile) {
        enableTextInput();
        if (textRenderer() != nullptr && profileNameField_ == nullptr) {
            profileNameField_ = std::make_unique<widget::TextFieldWidget>(
                this, textRenderer(), width() / 2 - 100, top + 36, 200, 20, "");
            profileNameField_->focused = true;
            profileNameField_->setMaxLength(16);
        }
        createProfileButton_ = &addCenteredActionButton(top + 78, "Create profile", [this] { beginCreateProfile(); });
        addCenteredActionButton(top + 102, "Cancel", [this] {
            cancelSignIn();
            navigateTo(returnFactory_);
        });
        updateCreateProfileButtonState();
        return;
    }
    openBrowserButton_ = &addCenteredActionButton(top + 78, "Open browser to sign in", [this] { openBrowser(); });
    openBrowserButton_->active = !loginUrl_.empty();
    addCenteredActionButton(top + 102, "Cancel", [this] {
        cancelSignIn();
        navigateTo(returnFactory_);
    });
}

void LoginScreen::removed() {
    disableTextInput();
}

void LoginScreen::beginSignIn() {
    if (minecraft() == nullptr || workerRunning_.load()) {
        return;
    }
    msauth::cancelSavedAccountRestore();
    cancelSignIn();
    phase_ = Phase::RequestingCode;
    errorMessage_.clear();
    userCode_.clear();
    loginUrl_.clear();
    expiresInSeconds_ = 0;
    elapsedSeconds_ = 0;
    statusLine1_ = "Requesting Microsoft sign-in code...";
    statusLine2_.clear();
    if (signInButton_ != nullptr) {
        signInButton_->active = false;
    }
    workerRunning_ = true;
    workerFinished_ = false;
    cancelRequested_ = false;
    authStage_ = msauth::AuthStage::RequestingDeviceCode;
    const std::string clientId = clientId_;
    workerThread_ = std::thread([this, clientId]() {
        const auto progress = [this](msauth::AuthStage stage) { authStage_.store(stage, std::memory_order_release); };
        msauth::DeviceCodeRequestResult codeRequest = msauth::requestDeviceCode(clientId, &cancelRequested_, progress);
        {
            std::lock_guard<std::mutex> lock(workerMutex_);
            pendingCodeRequest_ = std::move(codeRequest);
        }
        workerFinished_ = true;
    });
}

void LoginScreen::beginCreateProfile() {
    if (minecraft() == nullptr || workerRunning_.load() || profileNameField_ == nullptr ||
        profileNameField_->getText().empty()) {
        return;
    }
    errorMessage_.clear();
    if (createProfileButton_ != nullptr) {
        createProfileButton_->active = false;
    }
    workerRunning_ = true;
    workerFinished_ = false;
    cancelRequested_ = false;
    authStage_ = msauth::AuthStage::CreatingMinecraftProfile;
    statusLine1_ = "Creating Minecraft profile...";
    statusLine2_.clear();
    const msauth::MicrosoftAccount account = pendingProfileAccount_;
    const std::string profileName = profileNameField_->getText();
    workerThread_ = std::thread([this, account, profileName]() {
        const auto progress = [this](msauth::AuthStage stage) { authStage_.store(stage, std::memory_order_release); };
        msauth::AuthResult authResult =
            msauth::createMinecraftProfile(account, profileName, &cancelRequested_, progress);
        {
            std::lock_guard<std::mutex> lock(workerMutex_);
            pendingAuthResult_ = std::move(authResult);
        }
        workerFinished_ = true;
    });
}

void LoginScreen::openBrowser() {
    if (loginUrl_.empty()) {
        return;
    }
    if (!platform::openUrlInBrowser(loginUrl_)) {
        showSignInError("Could not open the default browser.");
    }
}

void LoginScreen::cancelSignIn() {
    cancelRequested_ = true;
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    workerRunning_ = false;
    workerFinished_ = false;
    authStage_ = msauth::AuthStage::Idle;
    {
        std::lock_guard<std::mutex> lock(workerMutex_);
        pendingCodeRequest_.reset();
        pendingAuthResult_.reset();
    }
    activeChallenge_ = {};
    msauth::secret::wipeString(pendingProfileAccount_.accessToken);
    msauth::secret::wipeString(pendingProfileAccount_.refreshToken);
    pendingProfileAccount_ = {};
    profileNameField_.reset();
}

void LoginScreen::showSignInError(std::string message) {
    authStage_ = msauth::AuthStage::Idle;
    phase_ = Phase::Failed;
    statusLine1_ = "Microsoft sign-in failed";
    statusLine2_.clear();
    errorMessage_ = std::move(message);
    userCode_.clear();
    loginUrl_.clear();
    msauth::secret::wipeString(pendingProfileAccount_.accessToken);
    msauth::secret::wipeString(pendingProfileAccount_.refreshToken);
    pendingProfileAccount_ = {};
    profileNameField_.reset();
    if (signInButton_ != nullptr) {
        signInButton_->active = true;
    }
}

void LoginScreen::showCreateProfile(const msauth::MicrosoftAccount& account) {
    authStage_ = msauth::AuthStage::Idle;
    phase_ = Phase::CreateProfile;
    pendingProfileAccount_ = account;
    statusLine1_ = "This account has no Minecraft profile yet.";
    statusLine2_ = "Choose a username to create one.";
    errorMessage_.clear();
    userCode_.clear();
    loginUrl_.clear();
    profileNameField_.reset();
    init();
}

void LoginScreen::applySignInSuccess(const msauth::MicrosoftAccount& account) {
    msauth::cancelSavedAccountRestore();
    msauth::applyAccountToClient(*minecraft(), account);
    if (!msauth::saveAccount(util::MinecraftDirectories::getRunDirectory(), account)) {
        showSignInError("Signed in, but could not save the refresh token.");
        return;
    }
    navigateTo(returnFactory_);
}

void LoginScreen::updateCreateProfileButtonState() {
    if (createProfileButton_ != nullptr) {
        createProfileButton_->active =
            profileNameField_ != nullptr && !profileNameField_->getText().empty() && !workerRunning_.load();
    }
}

void LoginScreen::mergePendingWork() {
    if (!workerFinished_.exchange(false)) {
        if (phase_ == Phase::WaitingForUser &&
            authStage_.load(std::memory_order_acquire) == msauth::AuthStage::WaitingForUser && expiresInSeconds_ > 0) {
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - waitingStarted_);
            elapsedSeconds_ = static_cast<int>(elapsed.count());
            if (elapsedSeconds_ >= expiresInSeconds_) {
                cancelSignIn();
                showSignInError("Microsoft sign-in timed out. Try again.");
                init();
            }
        }
        if (phase_ == Phase::CreateProfile) {
            updateCreateProfileButtonState();
        }
        return;
    }
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    if (phase_ == Phase::RequestingCode) {
        std::optional<msauth::DeviceCodeRequestResult> codeRequest;
        {
            std::lock_guard<std::mutex> lock(workerMutex_);
            codeRequest = std::move(pendingCodeRequest_);
            pendingCodeRequest_.reset();
        }
        if (!codeRequest.has_value() || !codeRequest->ok) {
            workerRunning_ = false;
            showSignInError(codeRequest.has_value() ? codeRequest->error : "Could not request a sign-in code.");
            init();
            return;
        }
        activeChallenge_ = codeRequest->challenge;
        userCode_ = activeChallenge_.userCode;
        loginUrl_ = !activeChallenge_.verificationUriComplete.empty()
                        ? activeChallenge_.verificationUriComplete
                        : platform::deviceCodeLoginUrl(activeChallenge_.verificationUri, activeChallenge_.userCode);
        expiresInSeconds_ = activeChallenge_.expiresIn;
        elapsedSeconds_ = 0;
        waitingStarted_ = std::chrono::steady_clock::now();
        phase_ = Phase::WaitingForUser;
        statusLine1_ = "Enter this code at microsoft.com/link";
        statusLine2_ = userCode_;
        errorMessage_.clear();
        init();
        (void) platform::openUrlInBrowser(loginUrl_);
        workerRunning_ = true;
        workerFinished_ = false;
        cancelRequested_ = false;
        authStage_ = msauth::AuthStage::WaitingForUser;
        const std::string clientId = clientId_;
        const msauth::DeviceCodeChallenge challenge = activeChallenge_;
        workerThread_ = std::thread([this, clientId, challenge]() {
            const auto progress = [this](msauth::AuthStage stage) {
                authStage_.store(stage, std::memory_order_release);
            };
            msauth::AuthResult authResult =
                msauth::loginWithDeviceCode(clientId, challenge, &cancelRequested_, progress);
            {
                std::lock_guard<std::mutex> lock(workerMutex_);
                pendingAuthResult_ = std::move(authResult);
            }
            workerFinished_ = true;
        });
        return;
    }
    if (phase_ == Phase::WaitingForUser || phase_ == Phase::CreateProfile) {
        std::optional<msauth::AuthResult> authResult;
        {
            std::lock_guard<std::mutex> lock(workerMutex_);
            authResult = std::move(pendingAuthResult_);
            pendingAuthResult_.reset();
        }
        workerRunning_ = false;
        if (!authResult.has_value()) {
            showSignInError(phase_ == Phase::CreateProfile ? "Could not create Minecraft profile."
                                                           : "Microsoft sign-in failed.");
            init();
            return;
        }
        if (authResult->needsProfileCreation) {
            showCreateProfile(authResult->account);
            return;
        }
        if (!authResult->ok) {
            if (phase_ == Phase::CreateProfile) {
                authStage_ = msauth::AuthStage::Idle;
                errorMessage_ = authResult->error.empty() ? "Could not create Minecraft profile." : authResult->error;
                statusLine1_ = "Profile creation failed";
                statusLine2_ = "Try another username.";
                updateCreateProfileButtonState();
                init();
                return;
            }
            showSignInError(authResult->error.empty() ? "Microsoft sign-in failed." : authResult->error);
            init();
            return;
        }
        applySignInSuccess(authResult->account);
    }
}

void LoginScreen::updateAuthStatus() {
    const msauth::AuthStage stage = authStage_.load(std::memory_order_acquire);
    if (stage == msauth::AuthStage::Idle || stage == msauth::AuthStage::WaitingForUser ||
        stage == msauth::AuthStage::Complete) {
        return;
    }
    statusLine1_ = msauth::authStageMessage(stage);
    if (phase_ == Phase::WaitingForUser) {
        statusLine2_.clear();
        if (openBrowserButton_ != nullptr) {
            openBrowserButton_->active = false;
        }
    }
}

void LoginScreen::tick() {
    if (phase_ == Phase::CreateProfile) {
        tickTextFields({profileNameField_.get()});
    }
    updateAuthStatus();
    mergePendingWork();
}

void LoginScreen::keyPressed(char character, int keyCode) {
    if (phase_ == Phase::CreateProfile) {
        handleFormKeyPress(character, keyCode, {profileNameField_.get()}, [this] { beginCreateProfile(); });
        updateCreateProfileButtonState();
    }
}

void LoginScreen::mouseClicked(int mouseX, int mouseY, int button) {
    Screen::mouseClicked(mouseX, mouseY, button);
    if (phase_ == Phase::CreateProfile) {
        clickTextFields(mouseX, mouseY, button, {profileNameField_.get()});
    }
}

void LoginScreen::render(int mouseX, int mouseY, float tickDelta) {
    renderBackground();
    if (textRenderer() == nullptr) {
        return;
    }
    const int centerX = width() / 2;
    const int top = height() / 4;
    drawCenteredTextWithShadow(*textRenderer(), "Microsoft Account", centerX, top - 30, 0xFFFFFF);
    drawCenteredTextWithShadow(*textRenderer(), statusLine1_, centerX, top + 8, 0xFFFFFF);
    if (!statusLine2_.empty()) {
        const int codeColor = phase_ == Phase::WaitingForUser ? 0xFFFF55 : 0xA0A0A0;
        drawCenteredTextWithShadow(*textRenderer(), statusLine2_, centerX, top + 22, codeColor);
    }
    if (phase_ == Phase::CreateProfile && profileNameField_ != nullptr) {
        profileNameField_->render();
    }
    if (phase_ == Phase::WaitingForUser &&
        authStage_.load(std::memory_order_acquire) == msauth::AuthStage::WaitingForUser && expiresInSeconds_ > 0) {
        const int remaining = expiresInSeconds_ - elapsedSeconds_;
        const std::string timerLine = "Waiting for sign-in... " + std::to_string(remaining > 0 ? remaining : 0) + "s";
        drawCenteredTextWithShadow(*textRenderer(), timerLine, centerX, top + 48, 0xA0A0A0);
    }
    if (!errorMessage_.empty()) {
        drawCenteredTextWithShadow(*textRenderer(), errorMessage_, centerX, top + 64, 0xFF5555);
    }
    Screen::render(mouseX, mouseY, tickDelta);
}
}  // namespace net::minecraft::client::gui::auth
