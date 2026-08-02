#pragma once
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include "net/minecraft/client/auth/microsoft/MicrosoftAuth.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
#include "net/minecraft/util/concurrent/Channel.hpp"
namespace net::minecraft::client::gui::auth {
class LoginScreen : public gui::screen::Screen {
 public:
 explicit LoginScreen(gui::screen::ScreenFactory returnFactory);
 ~LoginScreen() override;
 void init() override;
 void tick() override;
 void removed() override;
 void keyPressed(char character, int keyCode) override;
 void mouseClicked(int mouseX, int mouseY, int button) override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kLogin;
 }

 private:
 enum class Phase {
  Idle,
  RequestingCode,
  WaitingForUser,
  CreateProfile,
  Failed,
 };
 void beginSignIn();
 void beginCreateProfile();
 void openBrowser();
 void cancelSignIn();
 void mergePendingWork();
 void applySignInSuccess(const msauth::MicrosoftAccount& account);
 void showCreateProfile(const msauth::MicrosoftAccount& account);
 void showSignInError(std::string message);
 void updateCreateProfileButtonState();
 void updateAuthStatus();
 [[nodiscard]] msauth::AuthStage currentAuthStage() const;
 struct WorkResult {
  std::optional<msauth::DeviceCodeRequestResult> codeRequest;
  std::optional<msauth::AuthResult> authResult;
 };
 struct WorkState {
  std::atomic_bool canceled{false};
  std::atomic<msauth::AuthStage> stage{msauth::AuthStage::Idle};
  net::minecraft::util::concurrent::Channel<WorkResult> completed{1};
 };
 gui::screen::ScreenFactory returnFactory_;
 Phase phase_ = Phase::Idle;
 std::string statusLine1_;
 std::string statusLine2_;
 std::string errorMessage_;
 std::string userCode_;
 std::string loginUrl_;
 std::string clientId_;
 int expiresInSeconds_ = 0;
 int elapsedSeconds_ = 0;
 std::chrono::steady_clock::time_point waitingStarted_{};
 gui::widget::ActionButtonWidget* signInButton_ = nullptr;
 gui::widget::ActionButtonWidget* openBrowserButton_ = nullptr;
 gui::widget::ActionButtonWidget* createProfileButton_ = nullptr;
 std::unique_ptr<widget::TextFieldWidget> profileNameField_;
 msauth::MicrosoftAccount pendingProfileAccount_;
 std::atomic<bool> workerRunning_{false};
 std::atomic<msauth::AuthStage> authStage_{msauth::AuthStage::Idle};
 std::shared_ptr<WorkState> work_;
 msauth::DeviceCodeChallenge activeChallenge_;
 bool firstInit_ = true;
};
} // namespace net::minecraft::client::gui::auth
