#pragma once
#include <algorithm>
#include <vector>

namespace net::minecraft::client::render {

class ConditionalState {
 public:
  enum class Flavor {
   Glsl,
   Properties
  };
  explicit ConditionalState(Flavor flavor) : flavor_(flavor) {
   if(flavor_ == Flavor::Properties) frames_.push_back({true, true, true});
  }
  void push(bool condition) {
   const bool parent = active();
   frames_.push_back({parent, parent && condition, parent && condition});
  }
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
  void endif() {
   if(frames_.empty()) return;
   if(flavor_ == Flavor::Properties && frames_.size() == 1) return;
   frames_.pop_back();
  }
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
