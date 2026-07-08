#pragma once
#include <algorithm>
#include <cstdint>
#include <ostream>
#include <string_view>

#include "net/minecraft/client/option/ApplyFlags.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"

namespace net::minecraft::client::option {
struct OptionSpec {
    std::string_view persistKey;
    int saveOrder = 0;
    enum class Kind {
        Slider,
        Toggle,
        Cycle,
        Hidden
    } kind = Kind::Toggle;
    ApplyFlags onIntChange = ApplyFlags::None;
    ApplyFlags onFloatChange = ApplyFlags::None;
    float (*getFloat)(const GameOptions&) = nullptr;
    void (*setFloat)(GameOptions&, float) = nullptr;
    bool (*getBool)(const GameOptions&) = nullptr;
    void (*cycle)(GameOptions&, int delta) = nullptr;
    void (*load)(GameOptions&, std::string_view value) = nullptr;
    void (*save)(const GameOptions&, std::ostream& out) = nullptr;
    bool (*isEnabled)(const GameOptions&) = nullptr;
    bool (*needsScreenResize)(const GameOptions&) = nullptr;
};

namespace option_spec_detail {
template <float GameOptions::* Member>
float getFloatMember(const GameOptions& options) {
    return options.*Member;
}

template <float GameOptions::* Member>
void setFloatMember(GameOptions& options, float value) {
    options.*Member = value;
}

template <float GameOptions::* Member>
void loadFloatMember(GameOptions& options, std::string_view value) {
    try {
        options.*Member = std::stof(std::string(value));
    } catch (...) {
    }
}

template <float GameOptions::* Member>
void saveFloatMember(const GameOptions& options, std::ostream& out) {
    out << options.*Member;
}

template <bool GameOptions::* Member>
bool getBoolMember(const GameOptions& options) {
    return options.*Member;
}

template <bool GameOptions::* Member>
void cycleBoolMember(GameOptions& options, int) {
    options.*Member = !(options.*Member);
}

template <bool GameOptions::* Member>
void loadBoolMember(GameOptions& options, std::string_view value) {
    options.*Member = (value == "true");
}

template <bool GameOptions::* Member>
void saveBoolMember(const GameOptions& options, std::ostream& out) {
    out << (options.*Member ? "true" : "false");
}

template <int GameOptions::* Member, int Mod>
void cycleIntMod(GameOptions& options, int delta) {
    options.*Member = (options.*Member + delta + Mod) % Mod;
}

template <int GameOptions::* Member>
void loadIntMember(GameOptions& options, std::string_view value) {
    try {
        options.*Member = std::stoi(std::string(value));
    } catch (...) {
    }
}

template <int GameOptions::* Member>
void saveIntMember(const GameOptions& options, std::ostream& out) {
    out << options.*Member;
}

inline int cycleDiscrete(int current, int dir, const int* values, int count) {
    int idx = 0;
    for (int i = 0; i < count; ++i) {
        if (values[i] == current) {
            idx = i;
            break;
        }
    }
    idx = (idx + dir + count) % count;
    return values[idx];
}

inline void setAoLevel(GameOptions& options, float value) {
    options.aoLevel = value;
    options.ao = value > 0.0f;
}

inline float getRenderScaleSlider(const GameOptions& options) {
    return (options.renderScale - 1.0f) / 4.0f;
}

inline void setRenderScaleSlider(GameOptions& options, float value) {
    options.renderScale = 1.0f + value * 4.0f;
}

inline float getEntityDistanceSlider(const GameOptions& options) {
    return (options.entityDistanceScale - 0.25f) / 3.75f;
}

inline void setEntityDistanceSlider(GameOptions& options, float value) {
    options.entityDistanceScale = 0.25f + value * 3.75f;
}

inline bool alwaysResize(const GameOptions&) {
    return true;
}

inline OptionSpec makeSlider(const char* key,
                             int saveOrder,
                             ApplyFlags intFx,
                             ApplyFlags floatFx,
                             float (*getF)(const GameOptions&),
                             void (*setF)(GameOptions&, float),
                             void (*loadFn)(GameOptions&, std::string_view),
                             void (*saveFn)(const GameOptions&, std::ostream&),
                             bool (*isEnabled)(const GameOptions&) = nullptr) {
    return {key,
            saveOrder,
            OptionSpec::Kind::Slider,
            intFx,
            floatFx,
            getF,
            setF,
            nullptr,
            nullptr,
            loadFn,
            saveFn,
            isEnabled,
            nullptr};
}

inline OptionSpec makeToggle(const char* key,
                             int saveOrder,
                             ApplyFlags flags,
                             bool (*getB)(const GameOptions&),
                             void (*cyc)(GameOptions&, int),
                             void (*loadFn)(GameOptions&, std::string_view),
                             void (*saveFn)(const GameOptions&, std::ostream&),
                             bool (*isEnabled)(const GameOptions&) = nullptr) {
    return {key,
            saveOrder,
            OptionSpec::Kind::Toggle,
            flags,
            ApplyFlags::None,
            nullptr,
            nullptr,
            getB,
            cyc,
            loadFn,
            saveFn,
            isEnabled,
            nullptr};
}

inline OptionSpec makeCycle(const char* key,
                            int saveOrder,
                            ApplyFlags flags,
                            void (*cyc)(GameOptions&, int),
                            void (*loadFn)(GameOptions&, std::string_view),
                            void (*saveFn)(const GameOptions&, std::ostream&),
                            bool (*getB)(const GameOptions&) = nullptr,
                            bool (*isEnabled)(const GameOptions&) = nullptr,
                            bool (*needsResize)(const GameOptions&) = nullptr) {
    return {key,
            saveOrder,
            OptionSpec::Kind::Cycle,
            flags,
            ApplyFlags::None,
            nullptr,
            nullptr,
            getB,
            cyc,
            loadFn,
            saveFn,
            isEnabled,
            needsResize};
}

inline OptionSpec makeHidden(const char* key,
                             int saveOrder,
                             ApplyFlags flags,
                             bool (*getB)(const GameOptions&),
                             void (*loadFn)(GameOptions&, std::string_view),
                             void (*saveFn)(const GameOptions&, std::ostream&)) {
    return {key,
            saveOrder,
            OptionSpec::Kind::Hidden,
            flags,
            ApplyFlags::None,
            nullptr,
            nullptr,
            getB,
            nullptr,
            loadFn,
            saveFn,
            nullptr,
            nullptr};
}
}  // namespace option_spec_detail
}  // namespace net::minecraft::client::option
