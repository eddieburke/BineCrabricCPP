#pragma once

#include <algorithm>
#include <vector>

namespace net::minecraft::registry {

template <typename Tag>
class AutoRegistry {
public:
    using Initializer = void (*)();

    static void add(Initializer fn, int priority)
    {
        entries().push_back({fn, priority});
    }

    static void runAll()
    {
        auto& list = entries();
        std::sort(list.begin(), list.end(), [](const Entry& a, const Entry& b) {
            return a.priority < b.priority;
        });
        for (const Entry& entry : list) {
            entry.fn();
        }
    }

private:
    struct Entry {
        Initializer fn;
        int priority;
    };

    static std::vector<Entry>& entries()
    {
        static std::vector<Entry> list;
        return list;
    }
};

template <typename Tag>
class AutoRegistrar {
public:
    AutoRegistrar(typename AutoRegistry<Tag>::Initializer fn, int priority)
    {
        AutoRegistry<Tag>::add(fn, priority);
    }
};

} // namespace net::minecraft::registry
