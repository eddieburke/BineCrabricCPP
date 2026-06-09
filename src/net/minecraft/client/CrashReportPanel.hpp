#pragma once

#include "net/minecraft/client/BorderCanvas.hpp"
#include "net/minecraft/client/LogoCanvas.hpp"
#include "net/minecraft/util/crash/CrashReport.hpp"

#include <string>

namespace net::minecraft::client {

class CrashReportPanel {
public:
    explicit CrashReportPanel(const net::minecraft::util::crash::CrashReport& report);

    const std::string& reportText() const noexcept;
    int backgroundColor() const noexcept;

    const LogoCanvas& logoCanvas() const noexcept;
    const BorderCanvas& eastBorder() const noexcept;
    const BorderCanvas& westBorder() const noexcept;
    const BorderCanvas& southBorder() const noexcept;

private:
    static std::string stringifyException(const std::exception_ptr& exception);
    static std::string systemInfoText();
    static std::string javaStyleHashHex(const std::string& text);

    int backgroundColor_ = 3028036;
    LogoCanvas logoCanvas_;
    BorderCanvas eastBorder_{80};
    BorderCanvas westBorder_{80};
    BorderCanvas southBorder_{100};
    std::string reportText_;
};

} // namespace net::minecraft::client
