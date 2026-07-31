#include "net/minecraft/client/gl/ShaderCompileService.hpp"
#include <algorithm>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "net/minecraft/client/gl/GLCore.hpp"
namespace net::minecraft::client::gl {
namespace {
GLFWwindow* createWorkerWindow(GLFWwindow* shareWith) {
 glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
 glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
 glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
 glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
 glfwWindowHint(GLFW_DEPTH_BITS, 24);
 glfwWindowHint(GLFW_STENCIL_BITS, 8);
 glfwWindowHint(GLFW_ALPHA_BITS, 0);
 return glfwCreateWindow(16, 16, "shader-compile-worker", nullptr, shareWith);
}

std::uint64_t hashRequest(const ShaderCompileRequest& request) {
 if(request.contentHash != 0) return request.contentHash;
 if(request.compute) {
  return ShaderProgram::contentHash(true, request.preamble, request.vertex);
 }
 return ShaderProgram::contentHash(false, request.preamble, request.vertex, request.fragment,
                                   request.geometry, request.tessControl, request.tessEvaluation);
}
} // namespace

ShaderCompileService& ShaderCompileService::instance() {
 static ShaderCompileService service;
 return service;
}

void ShaderCompileService::setCacheDirectory(std::filesystem::path dir) {
 std::lock_guard lock(mutex_);
 disk_.setRoot(std::move(dir));
}

void ShaderCompileService::start(GLFWwindow* shareWith) {
 std::lock_guard lock(mutex_);
 if(started_) return;
 stop_ = false;
 unsigned count =
     std::max(1U, std::thread::hardware_concurrency() > 2 ? std::thread::hardware_concurrency() - 2 : 1U);
 count = std::min(count, 4U);
 workerWindows_.reserve(count);
 workers_.reserve(count);
 for(unsigned i = 0; i < count; ++i) {
  GLFWwindow* window = createWorkerWindow(shareWith);
  if(window == nullptr) continue;
  workerWindows_.push_back(window);
 }
 if(workerWindows_.empty()) return;
 started_ = true;
 for(std::size_t i = 0; i < workerWindows_.size(); ++i) {
  workers_.emplace_back([this, i] { workerMain(i, workerWindows_[i]); });
 }
}

void ShaderCompileService::stop() {
 {
  std::lock_guard lock(mutex_);
  if(!started_) return;
  stop_ = true;
 }
 queueCv_.notify_all();
 for(std::thread& worker : workers_) {
  if(worker.joinable()) worker.join();
 }
 workers_.clear();
 for(GLFWwindow* window : workerWindows_) {
  if(window != nullptr) glfwDestroyWindow(window);
 }
 workerWindows_.clear();
 {
  std::lock_guard lock(mutex_);
  jobs_.clear();
  while(!queue_.empty()) queue_.pop();
  started_ = false;
  stop_ = false;
 }
}

std::shared_ptr<ShaderCompileService::Job> ShaderCompileService::findOrCreateJob(ShaderCompileRequest request) {
 request.contentHash = hashRequest(request);
 auto found = jobs_.find(request.contentHash);
 if(found != jobs_.end()) return found->second;
 auto job = std::make_shared<Job>();
 job->request = std::move(request);
 jobs_.emplace(job->request.contentHash, job);
 queue_.push(job);
 queueCv_.notify_one();
 return job;
}

std::uint64_t ShaderCompileService::submit(ShaderCompileRequest request) {
 std::lock_guard lock(mutex_);
 if(!started_) {
  request.contentHash = hashRequest(request);
  return request.contentHash;
 }
 return findOrCreateJob(std::move(request))->request.contentHash;
}

ShaderCompileResult ShaderCompileService::compileBlocking(ShaderCompileRequest request) {
 request.contentHash = hashRequest(request);
 // Fast path: disk hit without needing a worker.
 {
  std::lock_guard lock(mutex_);
  if(auto cached = disk_.tryLoad(request.contentHash)) {
   ShaderCompileResult result;
   result.contentHash = request.contentHash;
   result.ok = true;
   result.fromDisk = true;
   result.binary = std::move(*cached);
   return result;
  }
  if(!started_ || workerWindows_.empty()) {
   return runJobOnCurrentContext(request);
  }
 }
 std::shared_ptr<Job> job;
 {
  std::lock_guard lock(mutex_);
  job = findOrCreateJob(std::move(request));
 }
 std::unique_lock lock(mutex_);
 job->cv.wait(lock, [&] { return job->done || stop_; });
 if(!job->done) {
  ShaderCompileResult failed;
  failed.contentHash = job->request.contentHash;
  failed.error = "compile service stopped";
  return failed;
 }
 return job->result;
}

void ShaderCompileService::invalidateDiskEntry(std::uint64_t contentHash) {
 std::lock_guard lock(mutex_);
 disk_.remove(contentHash);
 jobs_.erase(contentHash);
}

void ShaderCompileService::storeDiskEntry(const ProgramBinaryBlob& blob) {
 std::lock_guard lock(mutex_);
 disk_.store(blob);
}

ShaderCompileResult ShaderCompileService::runJobOnCurrentContext(const ShaderCompileRequest& request) {
 ShaderCompileResult result;
 result.contentHash = request.contentHash;
 if(auto cached = disk_.tryLoad(request.contentHash)) {
  result.ok = true;
  result.fromDisk = true;
  result.binary = std::move(*cached);
  return result;
 }
 GLCore::ensureLoaded();
 ShaderProgram program;
 ProgramBinaryBlob blob;
 const bool compiled =
     request.compute ? program.compileComputeToBinary(blob, request.vertex, request.preamble)
                     : program.compileToBinary(blob, request.vertex, request.fragment, request.preamble,
                                               request.geometry, request.tessControl, request.tessEvaluation);
 if(!compiled) {
  result.ok = false;
  result.error = program.lastError().empty() ? std::string("compile failed") : program.lastError();
  return result;
 }
 blob.contentHash = request.contentHash;
 result.ok = true;
 result.fromDisk = false;
 result.binary = std::move(blob);
 disk_.store(result.binary);
 return result;
}

void ShaderCompileService::workerMain(std::size_t, GLFWwindow* window) {
 glfwMakeContextCurrent(window);
 GLCore::ensureLoaded();
 for(;;) {
  std::shared_ptr<Job> job;
  {
   std::unique_lock lock(mutex_);
   queueCv_.wait(lock, [&] { return stop_ || !queue_.empty(); });
   if(stop_ && queue_.empty()) break;
   job = queue_.front();
   queue_.pop();
  }
  ShaderCompileResult result = runJobOnCurrentContext(job->request);
  {
   std::lock_guard lock(mutex_);
   job->result = std::move(result);
   job->done = true;
   jobs_.erase(job->request.contentHash);
  }
  job->cv.notify_all();
 }
 glfwMakeContextCurrent(nullptr);
}
} // namespace net::minecraft::client::gl
