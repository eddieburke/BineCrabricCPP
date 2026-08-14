#pragma once
#include <cstdint>
#include <string_view>

// Emits Intel VTune ITT markers when attached with VTune's "User-mode
// sampling and tracing" collection. Build with -DVTUNE_ENABLED=ON (see
// CMakeLists.txt) to link libittnotify; otherwise every macro is a no-op.
//
// VT_TRACE_EVENT(name)     RAII task span for the current scope
// VT_TRACE_COUNTER(name,v) instantaneous counter sample
// VT_TRACE_FRAME()         marks a frame boundary on the main VTune domain

#ifdef VTUNE_ENABLED
#include <ittnotify.h>

namespace net::minecraft::client::debug::vtune {

inline __itt_domain* domain() {
 static __itt_domain* d = __itt_domain_create("BineCrabric");
 return d;
}

class ScopedTask {
 public:
 explicit ScopedTask(const char* name) noexcept {
  __itt_task_begin(domain(), __itt_null, __itt_null, __itt_string_handle_create(name));
 }
 // string_view callers (e.g. FramePipeline::phaseName) always hand back
 // literal-backed views, so .data() is null-terminated in practice.
 explicit ScopedTask(std::string_view name) noexcept : ScopedTask(name.data()) {
 }
 ~ScopedTask() {
  __itt_task_end(domain());
 }
 ScopedTask(const ScopedTask&) = delete;
 ScopedTask& operator=(const ScopedTask&) = delete;
};

inline void counter(__itt_counter& slot, const char* name, double value) {
 if (slot == nullptr) {
  slot = __itt_counter_create(name, "BineCrabric");
 }
 __itt_counter_set_value(slot, &value);
}

} // namespace net::minecraft::client::debug::vtune

#define VT_TRACE_EVENT(name) \
 ::net::minecraft::client::debug::vtune::ScopedTask __vt_task_##__LINE__(name)
// Each call site owns its own counter handle (initialized once, on first
// hit) so concurrent call sites never fight over a shared cache slot.
#define VT_TRACE_COUNTER(name, value)                                        \
 do {                                                                        \
  static __itt_counter __vt_counter_##__LINE__ = nullptr;                   \
  ::net::minecraft::client::debug::vtune::counter(                          \
      __vt_counter_##__LINE__, (name), static_cast<double>(value));         \
 } while (0)
// Ends the previous frame instance (no-op on the very first call) and opens
// the next one, so each call marks exactly one frame boundary in VTune.
#define VT_TRACE_FRAME()                                                     \
 do {                                                                        \
  __itt_frame_end_v3(::net::minecraft::client::debug::vtune::domain(), nullptr); \
  __itt_frame_begin_v3(::net::minecraft::client::debug::vtune::domain(), nullptr); \
 } while (0)

#else // !VTUNE_ENABLED

#define VT_TRACE_EVENT(name)
#define VT_TRACE_COUNTER(name, value)
#define VT_TRACE_FRAME()

#endif // VTUNE_ENABLED
