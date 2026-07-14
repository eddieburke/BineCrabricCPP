#pragma once
#include <string>
namespace net::minecraft::client::gui::screen {
class LoadingDisplay {
public:
  virtual ~LoadingDisplay() = default;
  virtual void progressStart(const std::string& message) {
    title_ = message;
    progressStage_.clear();
    percentage_ = 0;
  }
  virtual void progressStage(const std::string& message) {
    progressStage_ = message;
  }
  virtual void progressStagePercentage(int percentage) {
    percentage_ = percentage;
  }
  [[nodiscard]] const std::string& title() const {
    return title_;
  }
  [[nodiscard]] const std::string& stage() const {
    return progressStage_;
  }
  [[nodiscard]] int percentage() const {
    return percentage_;
  }

private:
  std::string title_{};
  std::string progressStage_{};
  int percentage_ = 0;
};
} // namespace net::minecraft::client::gui::screen
