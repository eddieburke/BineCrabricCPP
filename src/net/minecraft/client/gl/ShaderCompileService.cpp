#include "net/minecraft/client/gl/ShaderCompileService.hpp"
#include <algorithm>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
namespace net::minecraft::client::gl {
namespace diagnostics = net::minecraft::client::diagnostics;
namespace {
struct WorkerContext {
 HWND hwnd = nullptr;
 HDC dc = nullptr;
 HGLRC ctx = nullptr;
};

// Builds a hidden window and a WGL context sharing `share` on the current
// thread. The pixel format is taken from the primary context so the driver
// treats both as the same format (required for wglShareLists). Returns an
// empty WorkerContext on any failure.
WorkerContext createWorkerContext(HGLRC share, int pixelFormat, const PIXELFORMATDESCRIPTOR& primaryPfd) {
 HWND hwnd = CreateWindowExW(0, L"STATIC", L"shader-compile-worker", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr,
                             GetModuleHandleW(nullptr), nullptr);
 if(hwnd == nullptr) return {};
 HDC dc = GetDC(hwnd);
 if(dc == nullptr) {
  DestroyWindow(hwnd);
  return {};
 }
 PIXELFORMATDESCRIPTOR pfd = primaryPfd;
 if(pixelFormat != 0) {
  if(!SetPixelFormat(dc, pixelFormat, &pfd)) {
   ReleaseDC(hwnd, dc);
   DestroyWindow(hwnd);
   return {};
  }
 } else {
  if(pfd.dwFlags == 0) {
   PIXELFORMATDESCRIPTOR fallback{};
   fallback.nSize = sizeof(PIXELFORMATDESCRIPTOR);
   fallback.nVersion = 1;
   fallback.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
   fallback.iPixelType = PFD_TYPE_RGBA;
   pfd = fallback;
  }
  const int chosen = ChoosePixelFormat(dc, &pfd);
  if(chosen == 0 || !SetPixelFormat(dc, chosen, &pfd)) {
   ReleaseDC(hwnd, dc);
   DestroyWindow(hwnd);
   return {};
  }
 }
 HGLRC ctx = wglCreateContext(dc);
 if(ctx == nullptr) {
  ReleaseDC(hwnd, dc);
  DestroyWindow(hwnd);
  return {};
 }
 if(share != nullptr && !wglShareLists(share, ctx)) {
  wglDeleteContext(ctx);
  ReleaseDC(hwnd, dc);
  DestroyWindow(hwnd);
  return {};
 }
 return {hwnd, dc, ctx};
}

void destroyWorkerContext(WorkerContext& context) {
 if(context.ctx != nullptr) {
  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext(context.ctx);
 }
 if(context.hwnd != nullptr && context.dc != nullptr) ReleaseDC(context.hwnd, context.dc);
 if(context.hwnd != nullptr) DestroyWindow(context.hwnd);
 context = {};
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

struct ShaderCompileService::ContextSpec {
 HGLRC share = nullptr;
 int pixelFormat = 0;
 PIXELFORMATDESCRIPTOR pfd{};
};

ShaderCompileService& ShaderCompileService::instance() {
 static ShaderCompileService service;
 return service;
}

void ShaderCompileService::setCacheDirectory(std::filesystem::path dir) {
 std::lock_guard lock(mutex_);
 disk_.setRoot(std::move(dir));
}

void ShaderCompileService::start() {
 std::lock_guard lock(mutex_);
 if(started_) return;
 stop_ = false;
 render::captureGlShaderSnapshot();
 const unsigned count = std::min(2u, net::minecraft::util::concurrent::ThreadCoordinator::instance().budget().glCompile);
 if(count == 0) return;
 // Snapshot the primary context here (it is current on the main thread) so the
 // worker threads can reproduce its pixel format and share into it. Window and
 // context creation itself happens on the worker threads to keep this call
 // non-blocking; GLFW window creation is main-thread-only and stalls the frame.
 contextSpec_ = std::make_unique<ContextSpec>();
 if(HDC dc = wglGetCurrentDC(); dc != nullptr) {
  contextSpec_->share = wglGetCurrentContext();
  contextSpec_->pixelFormat = GetPixelFormat(dc);
  if(contextSpec_->pixelFormat != 0) {
   DescribePixelFormat(dc, contextSpec_->pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &contextSpec_->pfd);
  }
 }
 workers_.reserve(count);
 started_ = true;
 liveWorkers_ = count;
 for(unsigned i = 0; i < count; ++i) {
  workers_.emplace_back([this] { workerMain(); });
 }
}

void ShaderCompileService::stop() {
 {
  std::lock_guard lock(mutex_);
  stop_ = true;
  started_ = false;
 }
 queueCv_.notify_all();
 for(std::thread& worker : workers_) {
  if(worker.joinable()) worker.join();
 }
 workers_.clear();
 {
  std::lock_guard lock(mutex_);
  jobs_.clear();
  while(!queue_.empty()) queue_.pop();
  completed_.clear();
  stop_ = false;
  liveWorkers_ = 0;
 }
}

bool ShaderCompileService::started() const noexcept {
 const std::lock_guard lock(mutex_);
 return started_;
}

std::shared_ptr<ShaderCompileService::Job> ShaderCompileService::findOrCreateJob(ShaderCompileRequest request) {
 request.contentHash = hashRequest(request);
 auto found = jobs_.find(request.contentHash);
 if(found != jobs_.end()) {
  ++found->second->waiters;
  return found->second;
 }
 auto job = std::make_shared<Job>();
 job->request = std::move(request);
 job->waiters = 1;
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

std::vector<std::shared_ptr<ShaderCompileService::Job>> ShaderCompileService::peekCompleted() {
 std::lock_guard lock(mutex_);
 return completed_;
}

void ShaderCompileService::releaseJob(std::uint64_t contentHash) {
 std::lock_guard lock(mutex_);
 // In-flight: the worker will still run, but drops the job from completed_ when
 // no waiter is left (see workerMain).
 if(auto found = jobs_.find(contentHash); found != jobs_.end()) {
  if(found->second->waiters > 0) {
   --found->second->waiters;
  }
  return;
 }
 for(auto it = completed_.begin(); it != completed_.end(); ++it) {
  if((*it)->request.contentHash != contentHash) {
   continue;
  }
  if(--(*it)->waiters == 0) {
   completed_.erase(it);
  }
  return;
 }
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
   diagnostics::recordWorkSpan("shader.disk.hit", 0);
   return result;
  }
  if(!started_) {
   return runJobOnCurrentContext(request);
  }
 }
 std::shared_ptr<Job> job;
 {
  std::lock_guard lock(mutex_);
  job = findOrCreateJob(std::move(request));
 }
 std::unique_lock lock(mutex_);
 job->cv.wait(lock, [&] { return job->done || stop_ || !started_; });
 if(!job->done) {
  if(!started_) {
   // Every worker failed to come online; compile on the caller's context.
   lock.unlock();
   releaseJob(job->request.contentHash);
   return runJobOnCurrentContext(job->request);
  }
  ShaderCompileResult failed;
  failed.contentHash = job->request.contentHash;
  failed.error = "compile service stopped";
  return failed;
 }
 lock.unlock();
 releaseJob(job->request.contentHash);
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
  diagnostics::recordWorkSpan("shader.disk.hit", 0);
  return result;
 }
 diagnostics::recordWorkSpan("shader.disk.miss", 0);
 GLCore::ensureLoaded();
 ShaderProgram program;
 ProgramBinaryBlob blob;
 diagnostics::WorkSpan compileSpan("shader.compile");
 const bool compiled =
     request.compute ? program.compileComputeToBinary(blob, request.vertex, request.preamble)
                     : program.compileToBinary(blob, request.vertex, request.fragment, request.preamble,
                                               request.geometry, request.tessControl, request.tessEvaluation);
 if(!compiled) {
  result.ok = false;
  // The source compiled but extracting a GL program binary failed (driver lacks
  // glGetProgramBinary / returned an empty binary). The program handle survives a
  // failed extraction, so valid() distinguishes this from a genuine shader error
  // and lets the caller fall back to a plain source compile.
  result.binaryUnsupported = program.valid();
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

void ShaderCompileService::workerMain() {
 ContextSpec* spec = contextSpec_.get();
 WorkerContext context = createWorkerContext(spec->share, spec->pixelFormat, spec->pfd);
 if(context.ctx == nullptr) {
  {
   std::lock_guard lock(mutex_);
   if(liveWorkers_ > 0) --liveWorkers_;
   if(liveWorkers_ == 0) {
    // No worker can serve jobs; wake blocked compileBlocking callers so they
    // fall back to compiling on their own context.
    started_ = false;
    for(auto& entry : jobs_) entry.second->cv.notify_all();
   }
  }
  return;
 }
 GLCore::ensureLoaded();
 for(;;) {
  std::shared_ptr<Job> job;
  {
   std::unique_lock lock(mutex_);
   queueCv_.wait(lock, [&] { return stop_ || !queue_.empty(); });
   if(stop_ && queue_.empty()) break;
   job = queue_.front();
   queue_.pop();
   if(job->waiters == 0) {
    jobs_.erase(job->request.contentHash);
    continue;
   }
  }
   ShaderCompileResult result = runJobOnCurrentContext(job->request);
   {
    std::lock_guard lock(mutex_);
    job->result = std::move(result);
    job->done = true;
    jobs_.erase(job->request.contentHash);
    if(job->waiters > 0) {
     completed_.push_back(job);
    }
   }
   job->cv.notify_all();
 }
 destroyWorkerContext(context);
 {
  std::lock_guard lock(mutex_);
  if(liveWorkers_ > 0) --liveWorkers_;
  if(liveWorkers_ == 0) started_ = false;
 }
}
} // namespace net::minecraft::client::gl
