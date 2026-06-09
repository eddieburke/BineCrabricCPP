#pragma once

#include <string>
#include <utility>

namespace net::minecraft::client::option {

class KeyBinding {
public:
    KeyBinding(std::string translationKeyIn, int codeIn)
        : translationKey(std::move(translationKeyIn)),
          code(codeIn)
    {
    }

    std::string translationKey;
    int code = 0;
};

} // namespace net::minecraft::client::option
