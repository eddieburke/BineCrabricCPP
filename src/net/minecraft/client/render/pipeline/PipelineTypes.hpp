#pragma once

#include <functional>

namespace net::minecraft::client::render {

using ColorMaskRestoreFn = std::function<void()>;

} // namespace net::minecraft::client::render
