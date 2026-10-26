#pragma once
#ifndef MINECRAFT_ENABLE_PROFILING
#ifdef NDEBUG
#define MINECRAFT_ENABLE_PROFILING 0
#else
#define MINECRAFT_ENABLE_PROFILING 1
#endif
#endif
namespace net::minecraft::util::diagnostics {
inline constexpr bool profilingEnabled = MINECRAFT_ENABLE_PROFILING != 0;
}
