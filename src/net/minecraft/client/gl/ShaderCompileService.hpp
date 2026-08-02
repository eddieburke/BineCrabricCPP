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
 // True when the source compiled but no binary could be produced (the driver
 // lacks glGetProgramBinary). Callers should fall back to a source compile.
 bool binaryUnsupported = false;
 std::string error;
 ProgramBinaryBlob binary;
};

class ShaderCompileService {
 public:
 static ShaderCompileService& instance();

 // A queued request plus the worker's finished result. Workers only produce
 // binary blobs; the main thread owns GL program objects and links them here.
 struct Job {
  ShaderCompileRequest request;
  bool done = false;
  ShaderCompileResult result;
  std::condition_variable cv;
  // Number of ProgramCaches still waiting on this job's binary. peekCompleted
  // hands out shared_ptr<Job>s; releaseJob drops it from the completed list
  // once every waiter has taken its copy.
  unsigned waiters = 0;
 };

 void setCacheDirectory(std::filesystem::path dir);
 [[nodiscard]] const std::filesystem::path& cacheDirectory() const noexcept {
  return disk_.root();
 }

  // Creates worker contexts sharing the current GL context (or standalone if
  // none is current). Safe to call multiple times; no-op if already running.
  void start();
  void stop();
  // True once start() has created worker threads. Callers that must not block
  // the main thread check this and fall back to a synchronous compile otherwise
  // (headless/test environments).
  [[nodiscard]] bool started() const noexcept;

  // Enqueue without waiting. Returns content hash used as job id.
  std::uint64_t submit(ShaderCompileRequest request);
  // Enqueue (if needed) and block until the binary/error is ready.
  ShaderCompileResult compileBlocking(ShaderCompileRequest request);
  // Snapshot of every request finished since the last poll. Never blocks and
  // does not remove jobs; callers resolve their pending hashes then releaseJob.
  // Main-thread only.
  std::vector<std::shared_ptr<Job>> peekCompleted();
  // Signal that one waiter (a ProgramCache) has consumed `contentHash`. When the
  // last waiter releases the job it is dropped from the completed list.
  void releaseJob(std::uint64_t contentHash);
  void invalidateDiskEntry(std::uint64_t contentHash);
  void storeDiskEntry(const ProgramBinaryBlob& blob);

private:
 void workerMain(std::size_t index, GLFWwindow* window);
 ShaderCompileResult runJobOnCurrentContext(const ShaderCompileRequest& request);
 std::shared_ptr<Job> findOrCreateJob(ShaderCompileRequest request);

 ShaderBinaryCache disk_{{}};
 mutable std::mutex mutex_;
 std::condition_variable queueCv_;
 std::queue<std::shared_ptr<Job>> queue_;
 std::unordered_map<std::uint64_t, std::shared_ptr<Job>> jobs_;
 std::vector<std::shared_ptr<Job>> completed_;
 std::vector<std::thread> workers_;
 std::vector<GLFWwindow*> workerWindows_;
 bool stop_ = false;
 bool started_ = false;
};
} // namespace net::minecraft::client::gl
