#pragma once
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/gl/ShaderBinaryCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
struct GLFWwindow;
namespace net::minecraft::client::gl {
struct ShaderCompileRequest {
 std::uint64_t contentHash = 0;
 bool compute = false;
 std::string preamble;
 std::string vertex;   // or compute source when compute=true
 std::string fragment;
 std::string geometry;
 std::string tessControl;
 std::string tessEvaluation;
};

struct ShaderCompileResult {
 std::uint64_t contentHash = 0;
 bool ok = false;
 bool fromDisk = false;
 std::string error;
 ProgramBinaryBlob binary;
};

class ShaderCompileService {
 public:
 static ShaderCompileService& instance();

 void setCacheDirectory(std::filesystem::path dir);
 [[nodiscard]] const std::filesystem::path& cacheDirectory() const noexcept {
  return disk_.root();
 }

 // Creates worker contexts. Safe to call multiple times; no-op if already running.
 void start(GLFWwindow* shareWith);
 void stop();

 // Enqueue without waiting. Returns content hash used as job id.
 std::uint64_t submit(ShaderCompileRequest request);
 // Enqueue (if needed) and block until the binary/error is ready.
 ShaderCompileResult compileBlocking(ShaderCompileRequest request);
 void invalidateDiskEntry(std::uint64_t contentHash);
 void storeDiskEntry(const ProgramBinaryBlob& blob);

private:
 struct Job {
  ShaderCompileRequest request;
  bool done = false;
  ShaderCompileResult result;
  std::condition_variable cv;
 };

 void workerMain(std::size_t index, GLFWwindow* window);
 ShaderCompileResult runJobOnCurrentContext(const ShaderCompileRequest& request);
 std::shared_ptr<Job> findOrCreateJob(ShaderCompileRequest request);

 ShaderBinaryCache disk_{{}};
 std::mutex mutex_;
 std::condition_variable queueCv_;
 std::queue<std::shared_ptr<Job>> queue_;
 std::unordered_map<std::uint64_t, std::shared_ptr<Job>> jobs_;
 std::vector<std::thread> workers_;
 std::vector<GLFWwindow*> workerWindows_;
 bool stop_ = false;
 bool started_ = false;
};
} // namespace net::minecraft::client::gl
