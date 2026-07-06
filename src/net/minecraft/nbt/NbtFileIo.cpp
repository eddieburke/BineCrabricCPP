#include "net/minecraft/nbt/NbtFileIo.hpp"
#include <fstream>
#include <stdexcept>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
namespace net::minecraft {
namespace fs = std::filesystem;
void fsyncFile(const fs::path& path) {
#ifdef _WIN32
  const HANDLE handle = CreateFileW(path.wstring().c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if(handle != INVALID_HANDLE_VALUE) {
    FlushFileBuffers(handle);
    CloseHandle(handle);
  }
#else
  const int fd = ::open(path.c_str(), O_RDONLY);
  if(fd >= 0) {
    ::fsync(fd);
    ::close(fd);
  }
#endif
}
void writeFileAtomic(const fs::path& path, const std::function<void(std::ostream&)>& writer,
                     const AtomicWriteOptions& options) {
  fs::path temp = path;
  temp += "_new";
  fs::path backup = path;
  backup += "_old";
  {
    std::ofstream output(temp, std::ios::binary | std::ios::trunc);
    if(!output) {
      throw std::runtime_error("Failed to open temporary file: " + temp.string());
    }
    try {
      writer(output);
      output.flush();
    } catch(...) {
      output.close();
      std::error_code cleanup;
      fs::remove(temp, cleanup);
      throw;
    }
    if(!output) {
      output.close();
      std::error_code cleanup;
      fs::remove(temp, cleanup);
      throw std::runtime_error("Failed to write temporary file: " + temp.string());
    }
  }
  if(options.fsync) {
    fsyncFile(temp);
  }
  // Swap order keeps at least one valid file on disk at every interruption point: the backup
  // holds the old copy from the moment the live file is moved aside until the temp is renamed in.
  std::error_code ec;
  fs::remove(backup, ec);
  ec.clear();
  if(fs::exists(path)) {
    fs::rename(path, backup, ec);
    ec.clear();
  }
  fs::remove(path, ec);
  ec.clear();
  fs::rename(temp, path, ec);
  ec.clear();
  if(!options.keepBackup) {
    fs::remove(backup, ec);
  }
}
} // namespace net::minecraft
