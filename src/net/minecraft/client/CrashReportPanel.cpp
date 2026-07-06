#include "net/minecraft/client/CrashReportPanel.hpp"
#include <chrono>
#include <cstdint>
#include <ctime>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <GL/gl.h>
namespace net::minecraft::client {
namespace {
std::string currentTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
  const std::tm localTime = *std::localtime(&nowTime);
  std::ostringstream stream;
  stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
  return stream.str();
}
std::string glString(const GLenum name) {
  const GLubyte* value = glGetString(name);
  if(value == nullptr) {
    return {};
  }
  return reinterpret_cast<const char*>(value);
}
} // namespace
CrashReportPanel::CrashReportPanel(const net::minecraft::util::crash::CrashReport& report) {
  std::string errorText = stringifyException(report.exception);
  std::string reportBody;
  reportBody += "Generated ";
  reportBody += currentTimestamp();
  reportBody += "\n\n";
  reportBody += "Minecraft: Minecraft Beta 1.7.3\n";
  reportBody += "OS: Windows\n";
  reportBody += "OpenGL: ";
  const std::string vendor = glString(GL_VENDOR);
  const std::string renderer = glString(GL_RENDERER);
  const std::string version = glString(GL_VERSION);
  reportBody += renderer.empty() ? "unknown" : renderer;
  reportBody += " version ";
  reportBody += version.empty() ? "unknown" : version;
  if(!vendor.empty()) {
    reportBody += ", ";
    reportBody += vendor;
  }
  reportBody += "\n\n";
  if(!report.description.empty()) {
    reportBody += "Description: ";
    reportBody += report.description;
    reportBody += "\n\n";
  }
  reportBody += errorText;
  std::string prefix = "\n\n";
  if(errorText.find("Pixel format not accelerated") != std::string::npos) {
    prefix += "      Bad video card drivers!      \n";
    prefix += "      -----------------------      \n\n";
    prefix += "Minecraft was unable to start because it failed to find an accelerated OpenGL mode.\n";
    prefix += "This can usually be fixed by updating the video card drivers.\n";
    const std::string vendorLower = vendor;
    if(vendorLower.find("nvidia") != std::string::npos || vendorLower.find("NVIDIA") != std::string::npos) {
      prefix += "\nYou might be able to find drivers for your video card here:\n  http://www.nvidia.com/\n";
    } else if(vendorLower.find("ati") != std::string::npos || vendorLower.find("ATI") != std::string::npos) {
      prefix += "\nYou might be able to find drivers for your video card here:\n  http://www.amd.com/\n";
    }
  } else {
    prefix += "      Minecraft has crashed!      \n";
    prefix += "      ----------------------      \n\n";
    prefix += "Minecraft has stopped running because it encountered a problem.\n\n";
    prefix += "If you wish to report this, please copy this entire text and email it to support@mojang.com.\n";
    prefix += "Please include a description of what you did when the error occured.\n";
  }
  prefix += "\n\n\n";
  const std::string beginHash = javaStyleHashHex(prefix + reportBody);
  std::string finalText = prefix;
  finalText += "--- BEGIN ERROR REPORT ";
  finalText += beginHash;
  finalText += " --------\n";
  finalText += reportBody;
  finalText += "--- END ERROR REPORT ";
  finalText += javaStyleHashHex(finalText);
  finalText += " ----------\n\n";
  reportText_ = std::move(finalText);
}
const std::string& CrashReportPanel::reportText() const noexcept {
  return reportText_;
}
int CrashReportPanel::backgroundColor() const noexcept {
  return backgroundColor_;
}
const LogoCanvas& CrashReportPanel::logoCanvas() const noexcept {
  return logoCanvas_;
}
const BorderCanvas& CrashReportPanel::eastBorder() const noexcept {
  return eastBorder_;
}
const BorderCanvas& CrashReportPanel::westBorder() const noexcept {
  return westBorder_;
}
const BorderCanvas& CrashReportPanel::southBorder() const noexcept {
  return southBorder_;
}
std::string CrashReportPanel::stringifyException(const std::exception_ptr& exception) {
  if(!exception) {
    return "No exception supplied.\n";
  }
  try {
    std::rethrow_exception(exception);
  } catch(const std::exception& ex) {
    std::ostringstream stream;
    stream << ex.what() << '\n';
    return stream.str();
  } catch(...) {
    return "Unknown native exception.\n";
  }
}
std::string CrashReportPanel::systemInfoText() {
  std::string text;
  text += "Minecraft: Minecraft Beta 1.7.3\n";
  text += "OS: Windows\n";
  return text;
}
std::string CrashReportPanel::javaStyleHashHex(const std::string& text) {
  std::uint32_t hash = 0;
  for(unsigned char c : text) {
    hash = hash * 31u + static_cast<std::uint32_t>(c);
  }
  std::ostringstream stream;
  stream << std::hex << hash;
  return stream.str();
}
} // namespace net::minecraft::client
