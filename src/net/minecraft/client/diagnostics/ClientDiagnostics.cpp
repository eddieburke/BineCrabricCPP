#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#include <thread>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <dbghelp.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#endif
namespace net::minecraft::client::diagnostics {
namespace {
const char* gStartupPhase = "before main";
#ifdef _WIN32
std::atomic<std::uint64_t> gHeartbeat{0};
std::atomic<bool> gHangDumpWritten{false};
std::atomic<bool> gWatchdogDisarmed{false};
std::string executableDirectory() {
  char path[MAX_PATH] = {};
  const DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
  if(length == 0 || length >= MAX_PATH) {
    return ".";
  }
  std::string directory(path, length);
  const std::size_t slash = directory.find_last_of("\\/");
  if(slash != std::string::npos) {
    directory.resize(slash);
  }
  return directory;
}
void ensureConsole() {
  if(GetConsoleWindow() != nullptr) {
    return;
  }
  if(!AllocConsole()) {
    return;
  }
  FILE* stream = nullptr;
  freopen_s(&stream, "CONOUT$", "w", stdout);
  freopen_s(&stream, "CONOUT$", "w", stderr);
  freopen_s(&stream, "CONIN$", "r", stdin);
  std::ios::sync_with_stdio();
}
void writeCrashReportFile(const std::string& details, const char* fileName = "crash-report.txt") {
  const std::string path = executableDirectory() + "\\" + fileName;
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if(!file.is_open()) {
    std::cerr << "Failed to write crash report to " << path << '\n';
    return;
  }
  file << details;
  std::cerr << "Crash report written to " << path << '\n';
}
void showCrashMessageBox(const std::string& title, const std::string& details) {
  std::string message = details;
  constexpr std::size_t kMaxMessageBoxChars = 2000;
  if(message.size() > kMaxMessageBoxChars) {
    message.resize(kMaxMessageBoxChars);
    message += "\n\n(truncated - see crash-report.txt)";
  }
  MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}
std::string describeModuleAtAddress(const void* address) {
  if(address == nullptr) {
    return "unknown module";
  }
  HMODULE module = nullptr;
  if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCSTR>(address),
                         &module) ||
     module == nullptr) {
    return "unknown module";
  }
  char path[MAX_PATH] = {};
  const DWORD length = GetModuleFileNameA(module, path, MAX_PATH);
  std::string description = length > 0 ? std::string(path, length) : "unknown module";
  const auto moduleBase = reinterpret_cast<std::uintptr_t>(module);
  const auto fault = reinterpret_cast<std::uintptr_t>(address);
  description += " +0x";
  std::ostringstream offset;
  offset << std::hex << (fault - moduleBase);
  description += offset.str();
  return description;
}
std::string captureStackTrace() {
  void* frames[32] = {};
  const USHORT count = CaptureStackBackTrace(0, static_cast<DWORD>(std::size(frames)), frames, nullptr);
  if(count == 0) {
    return "  (no stack frames captured)\n";
  }
  std::ostringstream stream;
  stream << "Stack trace:\n";
  for(USHORT i = 0; i < count; ++i) {
    stream << "  #" << i << ' ' << frames[i] << ' ' << describeModuleAtAddress(frames[i]) << '\n';
  }
  return stream.str();
}
void appendRegisterDump(std::ostringstream& stream, const CONTEXT* context) {
  if(context == nullptr) {
    return;
  }
  const auto reg = [&stream](const char* name, DWORD64 value) {
    stream << "  " << name << " = 0x" << std::hex << value << std::dec << " ("
           << describeModuleAtAddress(reinterpret_cast<const void*>(value)) << ")\n";
  };
  stream << "Registers:\n";
  reg("rip", context->Rip);
  reg("rsp", context->Rsp);
  reg("rbp", context->Rbp);
  reg("rax", context->Rax);
  reg("rbx", context->Rbx);
  reg("rcx", context->Rcx);
  reg("rdx", context->Rdx);
  reg("rsi", context->Rsi);
  reg("rdi", context->Rdi);
  reg("r8 ", context->R8);
  reg("r9 ", context->R9);
  reg("r10", context->R10);
  reg("r11", context->R11);
  reg("r12", context->R12);
  reg("r13", context->R13);
  reg("r14", context->R14);
  reg("r15", context->R15);
}
void writeMinidump(EXCEPTION_POINTERS* info, const char* fileName = "crash.dmp") {
  const std::string path = executableDirectory() + "\\" + fileName;
  const HANDLE file =
      CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if(file == INVALID_HANDLE_VALUE) {
    std::cerr << "Failed to open " << path << " for minidump\n";
    return;
  }
  MINIDUMP_EXCEPTION_INFORMATION exceptionInfo{};
  exceptionInfo.ThreadId = GetCurrentThreadId();
  exceptionInfo.ExceptionPointers = info;
  exceptionInfo.ClientPointers = FALSE;
  const auto dumpType = static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory |
                                                   MiniDumpWithThreadInfo);
  const BOOL ok = MiniDumpWriteDump(GetCurrentProcess(),
                                    GetCurrentProcessId(),
                                    file,
                                    dumpType,
                                    info != nullptr ? &exceptionInfo : nullptr,
                                    nullptr,
                                    nullptr);
  CloseHandle(file);
  if(ok) {
    std::cerr << "Minidump written to " << path << '\n';
  } else {
    std::cerr << "MiniDumpWriteDump failed: " << GetLastError() << '\n';
  }
}
std::string formatUnhandledException(EXCEPTION_POINTERS* info) {
  std::ostringstream stream;
  stream << "Unhandled native exception.\n";
  stream << "Startup phase: " << (gStartupPhase != nullptr ? gStartupPhase : "(null)") << '\n';
  stream << "Faulting thread: " << GetCurrentThreadId() << '\n';
  if(info != nullptr && info->ExceptionRecord != nullptr) {
    const EXCEPTION_RECORD* record = info->ExceptionRecord;
    stream << "Exception code: 0x" << std::hex << record->ExceptionCode << std::dec << '\n';
    stream << "Fault address: " << record->ExceptionAddress << '\n';
    stream << "Fault module: " << describeModuleAtAddress(record->ExceptionAddress) << '\n';
    if(record->NumberParameters >= 2 && record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
      stream << "Access type: " << (record->ExceptionInformation[0] == 0 ? "read" : "write") << '\n';
      stream << "Target address: 0x" << std::hex << record->ExceptionInformation[1] << std::dec << '\n';
    }
  }
  if(info != nullptr) {
    appendRegisterDump(stream, info->ContextRecord);
  }
  stream << captureStackTrace();
  return stream.str();
}
LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* info) {
  writeMinidump(info);
  reportFatalError("Minecraft Native - fatal crash", formatUnhandledException(info));
  pauseBeforeExit();
  return EXCEPTION_EXECUTE_HANDLER;
}
[[noreturn]] void terminateHandler() {
  const char* what = "unknown";
  try {
    const std::exception_ptr current = std::current_exception();
    if(current) {
      std::rethrow_exception(current);
    }
  } catch(const std::exception& ex) {
    what = ex.what();
  } catch(...) {
  }
  reportFatalError("Minecraft Native - std::terminate", what);
  pauseBeforeExit();
  std::abort();
}
#endif // _WIN32
} // namespace
void setStartupPhase(const char* phase) {
  gStartupPhase = phase != nullptr ? phase : "(null)";
}
#ifdef _WIN32
void installCrashDiagnostics() {
  ensureConsole();
  SetUnhandledExceptionFilter(unhandledExceptionFilter);
  std::set_terminate(terminateHandler);
}
void reportFatalError(const std::string& title, const std::string& details) {
  std::cerr << title << '\n'
            << details << std::endl;
  writeCrashReportFile(details);
  showCrashMessageBox(title, details);
}
void pauseBeforeExit() {
  if(GetConsoleWindow() != nullptr) {
    std::cerr << "\nPress Enter to exit...\n";
    std::cin.clear();
    std::cin.get();
    return;
  }
  MessageBoxA(
      nullptr, "The application has stopped. Press OK to exit.", "Minecraft Native", MB_OK | MB_SETFOREGROUND);
}
void pingMainLoopHeartbeat() {
  gHeartbeat.fetch_add(1, std::memory_order_relaxed);
  gHangDumpWritten.store(false, std::memory_order_relaxed);
}
void disarmHangWatchdog() {
  gWatchdogDisarmed.store(true, std::memory_order_relaxed);
}
void installHangWatchdog() {
  std::thread([]() {
    std::uint64_t lastSeen = gHeartbeat.load(std::memory_order_relaxed);
    int stalledChecks = 0;
    constexpr int kCheckIntervalSeconds = 1;
    constexpr int kStallThresholdChecks = 6;
    while(true) {
      std::this_thread::sleep_for(std::chrono::seconds(kCheckIntervalSeconds));
      if(gWatchdogDisarmed.load(std::memory_order_relaxed)) {
        return;
      }
      const std::uint64_t current = gHeartbeat.load(std::memory_order_relaxed);
      if(current != lastSeen) {
        lastSeen = current;
        stalledChecks = 0;
        continue;
      }
      ++stalledChecks;
      if(stalledChecks < kStallThresholdChecks) {
        continue;
      }
      bool expected = false;
      if(gHangDumpWritten.compare_exchange_strong(expected, true)) {
        std::cerr << "Main loop appears hung (no heartbeat for " << (kCheckIntervalSeconds * stalledChecks)
                  << "s). Writing hang.dmp\n";
        writeMinidump(nullptr, "hang.dmp");
        writeCrashReportFile(std::string("Main loop hang detected.\nStartup phase: ") +
                                 (gStartupPhase != nullptr ? gStartupPhase : "(null)") +
                                 "\nSee hang.dmp for thread states.\n",
                             "hang-report.txt");
      }
    }
  }).detach();
}
#endif // _WIN32
} // namespace net::minecraft::client::diagnostics
