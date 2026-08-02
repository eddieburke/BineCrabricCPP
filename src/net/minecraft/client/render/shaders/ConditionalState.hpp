#pragma once
#include <algorithm>
#include <vector>

namespace net::minecraft::client::render {

// One shared conditional-stack machine for #if/#ifdef/#ifndef/#elif/#else/#endif.
// Both the GLSL source engine (normalizePackSource) and the .properties engine
// (preprocessProperties) compute the branch condition themselves (through the
// shared evaluateIfExpression / defined helpers) and feed this machine only the
// truth value; the stack pairing and active/matched tracking live here.
//
// The two engines historically diverged on two edge cases, preserved via Flavor:
//  - A #elif/#else under an already-inactive enclosing #if: the GLSL engine keeps
//    the frame inactive (parentActive && !taken); the .properties engine consults
//    only the frame's own matched flag and can flip the frame's active bit back
//    on (its lineActive() = all-of then masks it, keeping the emitted text equal).
//  - Unmatched #elif/#else/#endif at top level: the GLSL engine ignores them; the
//    .properties engine keeps a bottom sentinel frame that is never popped, so a
//    top-level #else/#elif disables the rest of the file.
class ConditionalState {
 public:
  enum class Flavor {
   Glsl,
   Properties
  };
  explicit ConditionalState(Flavor flavor) : flavor_(flavor) {
   if(flavor_ == Flavor::Properties) frames_.push_back({true, true, true});
  }
  // #if/#ifdef/#ifndef: condition is the evaluated branch truth value.
  void push(bool condition) {
   const bool parent = active();
   frames_.push_back({parent, parent && condition, parent && condition});
  }
  // #elif: condition is the evaluated truth value of the elif expression.
  void elif(bool condition) {
   if(frames_.empty()) return;
   Frame& frame = frames_.back();
   if(flavor_ == Flavor::Properties) {
    if(frame.matched) {
     frame.active = false;
    } else if(condition) {
     frame.active = true;
     frame.matched = true;
    } else {
     frame.active = false;
    }
   } else if(frame.parentActive && !frame.matched) {
    frame.active = condition;
    frame.matched = condition;
   } else {
    frame.active = false;
   }
  }
  // #else
  void else_() {
   if(frames_.empty()) return;
   Frame& frame = frames_.back();
   if(flavor_ == Flavor::Properties) {
    if(frame.matched) {
     frame.active = false;
    } else {
     frame.active = true;
     frame.matched = true;
    }
   } else {
    frame.active = frame.parentActive && !frame.matched;
    frame.matched = true;
   }
  }
  // #endif: pops the innermost frame; the .properties sentinel is never popped.
  void endif() {
   if(frames_.empty()) return;
   if(flavor_ == Flavor::Properties && frames_.size() == 1) return;
   frames_.pop_back();
  }
  // True when the current line should be emitted/processed. The .properties
  // engine's lineActive() is an all-of over the whole stack (its #elif/#else can
  // leave a frame active under an inactive parent); the GLSL engine's active() is
  // the top frame only (its #elif/#else honor parentActive, so the top frame is
  // always transitive).
  bool active() const {
   if(flavor_ == Flavor::Properties) {
    return std::all_of(frames_.begin(), frames_.end(), [](const Frame& frame) { return frame.active; });
   }
   return frames_.empty() || frames_.back().active;
  }

 private:
  struct Frame {
   bool parentActive;
   bool matched;
   bool active;
  };
  Flavor flavor_;
  std::vector<Frame> frames_;
};
} // namespace net::minecraft::client::render
