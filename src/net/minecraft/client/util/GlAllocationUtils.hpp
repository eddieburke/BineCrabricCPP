#pragma once

#include "net/minecraft/client/gl/GL11.hpp"

#include <mutex>
#include <vector>

namespace net::minecraft::client::util {

// Faithful port of net.minecraft.client.util.GlAllocationUtils.
class GlAllocationUtils {
public:
    static int generateDisplayLists(int range)
    {
        std::lock_guard lock(mutex());
        const int base = gl::GL11::glGenLists(range);
        displayLists().push_back(base);
        displayLists().push_back(range);
        return base;
    }

    static void generateTextureName(unsigned int& outName)
    {
        std::lock_guard lock(mutex());
        gl::GL11::glGenTextures(1, &outName);
        textureNames().push_back(outName);
    }

    static void deleteDisplayLists(int index)
    {
        std::lock_guard lock(mutex());
        auto& lists = displayLists();
        for (std::size_t i = 0; i + 1 < lists.size(); i += 2) {
            if (lists[i] == index) {
                gl::GL11::glDeleteLists(lists[i], lists[i + 1]);
                lists.erase(lists.begin() + static_cast<std::ptrdiff_t>(i),
                            lists.begin() + static_cast<std::ptrdiff_t>(i + 2));
                return;
            }
        }
    }

    static void clear()
    {
        std::lock_guard lock(mutex());
        auto& lists = displayLists();
        for (std::size_t i = 0; i + 1 < lists.size(); i += 2) {
            gl::GL11::glDeleteLists(lists[i], lists[i + 1]);
        }
        lists.clear();

        auto& names = textureNames();
        if (!names.empty()) {
            gl::GL11::glDeleteTextures(static_cast<int>(names.size()), names.data());
        }
        names.clear();
    }

private:
    static std::mutex& mutex()
    {
        static std::mutex m;
        return m;
    }

    static std::vector<int>& displayLists()
    {
        static std::vector<int> v;
        return v;
    }

    static std::vector<unsigned int>& textureNames()
    {
        static std::vector<unsigned int> v;
        return v;
    }
};

} // namespace net::minecraft::client::util
