#pragma once
#include <string>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
#ifdef _WIN32
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#endif

namespace net::minecraft::client::render {
inline void shaderFatal(const std::string& title, const std::string& details) {
 ClientLog::LOGGER.log(::net::minecraft::util::logging::LogLevel::Severe,
                       "[shader] " + title + ": " + details);
#ifdef _WIN32
 diagnostics::reportFatalError(title, details);
#endif
}
} // namespace net::minecraft::client::render
