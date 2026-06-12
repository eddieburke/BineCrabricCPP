#include "net/minecraft/client/option/GameOptions.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/world/World.hpp"

#include "net/minecraft/client/input/KeyCodes.hpp"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

namespace net::minecraft::client::option {
namespace {

void reloadWorldRenderer(Minecraft* minecraft)
{
    if (minecraft != nullptr && minecraft->worldRenderer != nullptr) {
        minecraft->worldRenderer->reload();
    }
}

void reloadTextures(Minecraft* minecraft)
{
    if (minecraft != nullptr) {
        minecraft->textureManager.reload();
    }
}

} // namespace

void GameOptions::load()
{
    if (optionsFile.empty() || !std::filesystem::exists(optionsFile)) {
        applyDerivedSettings();
        return;
    }
    std::ifstream in(optionsFile);
    if (!in) {
        applyDerivedSettings();
        return;
    }
    std::string line;
    while (std::getline(in, line)) {
        try {
            const auto colon = line.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            const std::string key = line.substr(0, colon);
            const std::string val = line.substr(colon + 1);
            if (key == "anaglyph3d") {
                if (val == "true") {
                    stereoMode = 1;
                }
                continue;
            }
            if (key == "skin") {
                skin = val;
                continue;
            }
            if (key == "lastServer") {
                lastServer = val;
                continue;
            }
            if (const std::optional<const OptionSpec*> spec = OptionRegistry::byKey(key)) {
                if ((*spec)->load != nullptr) {
                    (*spec)->load(*this, val);
                }
                continue;
            }
            for (KeyBinding* kb : allKeys) {
                if (key == "key_" + kb->translationKey) {
                    kb->code = std::stoi(val);
                    break;
                }
            }
        } catch (...) {}
    }
    applyDerivedSettings();
}

void GameOptions::save()
{
    if (optionsFile.empty()) {
        return;
    }
    std::ofstream out(optionsFile);
    if (!out) {
        return;
    }

    std::vector<const OptionSpec*> specs;
    specs.reserve(OptionRegistry::all().size());
    for (const OptionSpec& spec : OptionRegistry::all()) {
        specs.push_back(&spec);
    }
    std::sort(specs.begin(), specs.end(),
        [](const OptionSpec* a, const OptionSpec* b) { return a->saveOrder < b->saveOrder; });

    for (const OptionSpec* spec : specs) {
        if (spec->persistKey.empty() || spec->save == nullptr) {
            continue;
        }
        out << spec->persistKey << ':';
        spec->save(*this, out);
        out << '\n';
    }
    if (stereoMode == 1) {
        out << "anaglyph3d:true\n";
    }
    out << "skin:" << skin << "\n"
        << "lastServer:" << lastServer << "\n";
    for (KeyBinding* kb : allKeys) {
        out << "key_" << kb->translationKey << ":" << kb->code << "\n";
    }
}

void GameOptions::applyToWorld(World* world) const
{
    if (world == nullptr) {
        return;
    }
    const ResolvedRenderOptions resolved = resolve(*this);
    world->applyWorldSettings(ofWeather, ofAutoSaveTicks, ofTime);
    // Keep the whole render-distance disc resident, plus a small preload margin.
    // chunkGridRadius is the torus span in blocks; halve to chunks for the radius.
    const int gridChunkRadius = resolved.chunkGridRadius / 16 / 2;
    world->setChunkPreloadRadius(gridChunkRadius + resolved.chunkPreloadRadius);
}

void GameOptions::applyDerivedSettings()
{
    const ResolvedRenderOptions resolved = resolve(*this);
    texture::TextureManager::MIPMAP = ofMipmapLevel > 0;
    texture::TextureManager::MIPMAP_LINEAR = resolved.mipmapLinearFilter;
    ao = ofAoLevel > 0.0f;
}

void GameOptions::applySideEffects(ApplyFlags flags)
{
    if ((flags & ApplyFlags::ApplyDerived) != ApplyFlags::None) {
        applyDerivedSettings();
    }
    if ((flags & ApplyFlags::ReloadTextures) != ApplyFlags::None) {
        reloadTextures(minecraft);
    }
    if ((flags & ApplyFlags::ReloadWorld) != ApplyFlags::None) {
        reloadWorldRenderer(minecraft);
    }
    if ((flags & ApplyFlags::ApplyToWorld) != ApplyFlags::None) {
        if (minecraft != nullptr) {
            applyToWorld(minecraft->world);
        }
    }
    if ((flags & ApplyFlags::UpdateSound) != ApplyFlags::None) {
        if (minecraft != nullptr) {
            minecraft->audio.refreshMusicVolume();
        }
    }
}

bool GameOptions::getBoolean(std::string_view persistKey) const
{
    const std::optional<const OptionSpec*> spec = OptionRegistry::byKey(persistKey);
    if (!spec || (*spec)->getBool == nullptr) {
        return false;
    }
    return (*spec)->getBool(*this);
}

float GameOptions::getFloat(std::string_view persistKey) const
{
    const std::optional<const OptionSpec*> spec = OptionRegistry::byKey(persistKey);
    if (!spec || (*spec)->getFloat == nullptr) {
        return 0.0f;
    }
    return (*spec)->getFloat(*this);
}

std::string GameOptions::getString(std::string_view persistKey) const
{
    const std::optional<const OptionSpec*> spec = OptionRegistry::byKey(persistKey);
    if (!spec) {
        return {};
    }
    if ((*spec)->kind == OptionSpec::Kind::Toggle) {
        return resource::language::I18n::getTranslation(
            getBoolean(persistKey) ? "options.on" : "options.off");
    }
    return std::string((*spec)->persistKey);
}

std::string GameOptions::getKeybindName(int index) const
{
    if (index < 0 || index >= kKeybindCount) {
        return "?";
    }
    return resource::language::I18n::getTranslation(allKeys[index]->translationKey);
}

std::string GameOptions::getKeybindKey(int index) const
{
    if (index < 0 || index >= kKeybindCount) {
        return "?";
    }
#ifdef _WIN32
    return client::input::keyDisplayName(allKeys[index]->code);
#else
    (void)index;
    return "?";
#endif
}

void GameOptions::setKeybindKey(int index, int keyCode)
{
    if (index < 0 || index >= kKeybindCount) {
        return;
    }
    allKeys[index]->code = keyCode;
    save();
}

void GameOptions::cycle(std::string_view persistKey, int delta)
{
    const std::optional<const OptionSpec*> spec = OptionRegistry::byKey(persistKey);
    if (!spec || (*spec)->cycle == nullptr) {
        return;
    }
    (*spec)->cycle(*this, delta);
    applySideEffects((*spec)->onIntChange);
    save();
}

void GameOptions::setFloat(std::string_view persistKey, float value)
{
    const std::optional<const OptionSpec*> spec = OptionRegistry::byKey(persistKey);
    if (!spec) {
        return;
    }
    value = std::clamp(value, 0.0f, 1.0f);
    if ((*spec)->setFloat != nullptr) {
        (*spec)->setFloat(*this, value);
    }
    applySideEffects((*spec)->onFloatChange);
    save();
}

} // namespace net::minecraft::client::option
