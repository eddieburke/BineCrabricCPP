#include "seedfinder/gui/SeedfinderScreen.hpp"

#include "msauth/FilePicker.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/world/CreateWorldScreen.hpp"
#include "net/minecraft/client/gui/widget/EntryListWidget.hpp"
#include "net/minecraft/client/gui/widget/TextSizedActionButtonWidget.hpp"
#include "seedfinder/gui/SeedSpecEditor.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "seedfinder/config/ConfigSchema.hpp"
#include "seedfinder/config/JsonConfig.hpp"
#include "seedfinder/engine/BiomeCatalog.hpp"
#include "seedfinder/engine/Scorer.hpp"
#include "seedfinder/engine/SearchEngine.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace net::minecraft::client::gui::screen::world {
namespace {

constexpr int kSidebarLeft = 8;
constexpr int kSidebarWidth = 160;
constexpr int kMapLeft = 176;
constexpr int kMapMargin = 12;

const std::vector<std::string>& spawnPickerBiomes()
{
    static const std::vector<std::string> names = []() {
        std::vector<std::string> picker;
        picker.emplace_back("Any");
        for (const std::string& name : seedfinder::engine::spawnPickerBiomeNames()) {
            picker.push_back(name);
        }
        return picker;
    }();
    return names;
}

constexpr int kFooterReserve = 72;
constexpr int kMinResultHeight = 48;

[[nodiscard]] int contentBottom(int screenHeight)
{
    return screenHeight - kFooterReserve;
}

[[nodiscard]] std::string fieldTextOr(const widget::TextFieldWidget* field, const char* fallback)
{
    return field != nullptr ? field->getText() : fallback;
}

std::string prettifyName(const std::string& name)
{
    std::string out = name;
    std::replace(out.begin(), out.end(), '_', ' ');
    return out;
}

std::uint64_t parseU64Field(const widget::TextFieldWidget* field, std::uint64_t fallback)
{
    if (field == nullptr) {
        return fallback;
    }
    const std::string text = field->getText();
    if (text.empty()) {
        return fallback;
    }
    try {
        return static_cast<std::uint64_t>(std::stoull(text));
    } catch (const std::exception&) {
        return fallback;
    }
}

int parseIntField(const widget::TextFieldWidget* field, int fallback)
{
    if (field == nullptr) {
        return fallback;
    }
    const std::string text = field->getText();
    if (text.empty()) {
        return fallback;
    }
    try {
        return std::stoi(text);
    } catch (const std::exception&) {
        return fallback;
    }
}

float parseFloatField(const widget::TextFieldWidget* field, float fallback)
{
    if (field == nullptr) {
        return fallback;
    }
    const std::string text = field->getText();
    if (text.empty()) {
        return fallback;
    }
    try {
        return std::stof(text);
    } catch (const std::exception&) {
        return fallback;
    }
}

std::string depthLabel(std::uint8_t depth)
{
    switch (depth) {
    case 0:
        return "Detail: Biome Only";
    case 1:
        return "Detail: Terrain";
    case 2:
        return "Detail: Full Decorate";
    default:
        return "Detail: Terrain";
    }
}

} // namespace

class SeedfinderScreen::ResultListWidget : public widget::EntryListWidget {
public:
    ResultListWidget(SeedfinderScreen& owner, Minecraft& minecraft, int width, int height, int top, int bottom)
        : widget::EntryListWidget(minecraft, width, height, top, bottom, 20),
          owner_(owner)
    {
        left_ = kSidebarLeft;
        right_ = kSidebarLeft + kSidebarWidth;
        setListBounds(kSidebarLeft, kSidebarLeft + kSidebarWidth, kSidebarLeft + 4);
    }

protected:
    [[nodiscard]] int getEntryCount() const override
    {
        return static_cast<int>(owner_.results_.size());
    }

    void entryClicked(int index, bool doubleClick) override
    {
        owner_.selectedResultIndex_ = index;
        if (owner_.useSeedButton_ != nullptr) {
            owner_.useSeedButton_->active =
                index >= 0 && index < static_cast<int>(owner_.results_.size());
        }
        owner_.refreshBiomeMap();
        if (doubleClick) {
            owner_.applySelectedSeed();
        }
    }

    [[nodiscard]] bool isSelectedEntry(int index) const override
    {
        return index == owner_.selectedResultIndex_;
    }

    [[nodiscard]] int getEntriesHeight() const override
    {
        return static_cast<int>(owner_.results_.size()) * 20;
    }

    void renderBackground() override
    {
        owner_.renderBackground();
    }

    void renderEntry(int index, int x, int y, int height, render::Tessellator& tessellator) override
    {
        (void)height;
        (void)tessellator;
        if (index < 0 || index >= static_cast<int>(owner_.results_.size()) || owner_.textRenderer() == nullptr) {
            return;
        }
        const SeedResultRow& row = owner_.results_[static_cast<std::size_t>(index)];
        const std::string biome = seedfinder::engine::biomeNameFromId(row.biomeId);
        std::ostringstream line;
        line << row.seed << "  " << biome;
        owner_.drawTextWithShadow(*owner_.textRenderer(), line.str(), x + 2, y + 3, 0xFFFFFF);
        std::ostringstream scoreLine;
        scoreLine << "score " << row.score;
        owner_.drawTextWithShadow(*owner_.textRenderer(), scoreLine.str(), x + 2, y + 11, 0xA0A0A0);
    }

private:
    SeedfinderScreen& owner_;
};

SeedfinderScreen::SeedfinderScreen(
    screen::ScreenFactory parentFactory,
    std::string returnWorldName,
    std::string returnSeedText)
    : parentFactory_(std::move(parentFactory)),
      returnWorldName_(std::move(returnWorldName)),
      returnSeedText_(std::move(returnSeedText))
{
}

SeedfinderScreen::~SeedfinderScreen()
{
    stopSearch();
    if (importThread_.joinable()) {
        importThread_.join();
    }
    if (biomeMap_ != nullptr && minecraft() != nullptr) {
        biomeMap_->clear(*minecraft());
    }
}

void SeedfinderScreen::forEachField(const std::function<void(widget::TextFieldWidget&)>& fn)
{
    for (const FormField& def : formFields_) {
        if (def.slot != nullptr && *def.slot != nullptr) {
            fn(**def.slot);
        }
    }
}

void SeedfinderScreen::forEachField(const std::function<void(const widget::TextFieldWidget&)>& fn) const
{
    for (const FormField& def : formFields_) {
        if (def.slot != nullptr && *def.slot != nullptr) {
            fn(**def.slot);
        }
    }
}

bool SeedfinderScreen::charAllowedFor(const FormField& def, char character) const
{
    if (character >= '0' && character <= '9') {
        return true;
    }
    const std::string& text = (*def.slot)->getText();
    if (character == '-' && def.allowSign && text.empty()) {
        return true;
    }
    if (character == '.' && def.allowDot && text.find('.') == std::string::npos) {
        return true;
    }
    return false;
}

bool SeedfinderScreen::fieldValid(const FormField& def) const
{
    if (def.slot == nullptr || *def.slot == nullptr) {
        return true;
    }
    const std::string& text = (*def.slot)->getText();
    if (text.empty()) {
        return true; // empty falls back to a sane default
    }
    try {
        if (def.allowDot) {
            (void)std::stof(text);
        } else {
            (void)std::stoull(text);
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

seedfinder::config::SearchConfig SeedfinderScreen::buildSearchConfig() const
{
    using seedfinder::config::SearchConfig;
    SearchConfig cfg;
    cfg.seed_start = parseU64Field(rangeStartField_.get(), 0);
    cfg.seed_end = parseU64Field(rangeEndField_.get(), 100000);
    if (cfg.seed_end < cfg.seed_start) {
        std::swap(cfg.seed_start, cfg.seed_end);
    }
    cfg.dimension = "overworld";
    cfg.scan_radius_chunks = std::max(1, parseIntField(scanRadiusField_.get(), 4));
    cfg.probe_depth_max = probeDepth_;
    cfg.compute_spawn = true;
    cfg.threads = std::max(1, parseIntField(threadsField_.get(), 1));
    cfg.top_k = std::max(1, parseIntField(topKField_.get(), 10));

    const std::vector<std::string>& spawnBiomes = spawnPickerBiomes();
    if (spawnBiomeIndex_ > 0 && spawnBiomeIndex_ < static_cast<int>(spawnBiomes.size())) {
        cfg.spawn_biome = spawnBiomes[static_cast<std::size_t>(spawnBiomeIndex_)];
    }

    const int uniqueMin = parseIntField(uniqueBiomeMinField_.get(), -1);
    if (uniqueMin >= 0) {
        cfg.unique_biome_min = uniqueMin;
    }

    const float minCoverage = parseFloatField(minBiomeCoverageField_.get(), -1.f);
    if (minCoverage >= 0.f) {
        cfg.required_biome_min_coverage_percent = minCoverage;
    }

    const int minBlobRadius = parseIntField(minBiomeRadiusField_.get(), -1);
    if (minBlobRadius >= 1) {
        cfg.required_biome_min_contiguous_radius_chunks = minBlobRadius;
    }

    const float minCompactness = parseFloatField(minBiomeCompactnessField_.get(), -1.f);
    if (minCompactness >= 0.f) {
        cfg.required_biome_min_compactness_percent = minCompactness;
    }

    if ((minCoverage >= 0.f || minBlobRadius >= 1 || minCompactness >= 0.f)
        && !cfg.spawn_biome.empty() && cfg.required_biomes.empty()) {
        cfg.required_biomes.push_back(cfg.spawn_biome);
    }

    if (!specNodes_.empty()) {
        cfg.advanced_rule_data_nbt = seedfinder::config::serializeAdvancedRuleDataNbt(specNodes_);
        cfg.raw_values["advanced_rule_data_nbt"] = cfg.advanced_rule_data_nbt;
        cfg.advanced_rule_nodes.clear();
        cfg.advanced_rule_errors.clear();
        (void)seedfinder::config::parseAdvancedRuleDataNbt(
            cfg.advanced_rule_data_nbt, cfg.advanced_rule_nodes, cfg.advanced_rule_errors);
    }
    return cfg;
}

float SeedfinderScreen::minScoreThreshold() const
{
    return parseFloatField(minScoreField_.get(), 0.f);
}

SeedfinderScreen::FieldTexts SeedfinderScreen::snapshotFieldTexts() const
{
    FieldTexts saved;
    saved.rangeStart = fieldTextOr(rangeStartField_.get(), "0");
    saved.rangeEnd = fieldTextOr(rangeEndField_.get(), "100000");
    saved.scanRadius = fieldTextOr(scanRadiusField_.get(), "4");
    saved.minScore = fieldTextOr(minScoreField_.get(), "0");
    saved.threads = fieldTextOr(threadsField_.get(), "1");
    saved.topK = fieldTextOr(topKField_.get(), "10");
    saved.uniqueBiomeMin = fieldTextOr(uniqueBiomeMinField_.get(), "");
    saved.minBiomeCoverage = fieldTextOr(minBiomeCoverageField_.get(), "");
    saved.minBiomeRadius = fieldTextOr(minBiomeRadiusField_.get(), "");
    saved.minBiomeCompactness = fieldTextOr(minBiomeCompactnessField_.get(), "");
    return saved;
}

void SeedfinderScreen::layoutForm(const FieldTexts* restore)
{
    buttons_.clear();
    formFields_.clear();
    searchButton_ = nullptr;
    stopButton_ = nullptr;
    useSeedButton_ = nullptr;
    depthButton_ = nullptr;
    spawnBiomeButton_ = nullptr;
    specButton_ = nullptr;
    importJsonButton_ = nullptr;

    if (textRenderer() == nullptr) {
        formBottom_ = 150;
        resultTop_ = 164;
        return;
    }

    // Compact, label-on-the-left grid so the whole form stays short enough to never
    // collide with the footer, even at large (auto) GUI scales / short windows.
    panelW_ = std::min(560, std::max(200, width() - 20));
    panelX_ = (width() - panelW_) / 2;
    const int colGap = 16;
    const int colW = (panelW_ - colGap) / 2;
    const int labelW = 78;
    const int fieldW = std::max(44, colW - labelW);
    const int c0 = panelX_;
    const int c1 = panelX_ + colW + colGap;
    const int f0 = c0 + labelW; // field x, column 0
    const int f1 = c1 + labelW; // field x, column 1
    const int formTop = 28;
    const int stride = 24;

    auto pick = [&](const char* fallback, const std::string& saved) -> std::string {
        return restore != nullptr ? saved : fallback;
    };

    auto make = [&](std::unique_ptr<widget::TextFieldWidget>& slot, const char* label, bool allowDot,
                    int x, int y, const std::string& initial) {
        slot = std::make_unique<widget::TextFieldWidget>(this, textRenderer(), x, y, fieldW, 20, initial);
        formFields_.push_back(FormField{&slot, label, false, allowDot, x, y, fieldW});
    };

    const FieldTexts defaults {};
    const FieldTexts& saved = restore != nullptr ? *restore : defaults;
    make(rangeStartField_, "Start", false, f0, formTop + 0 * stride, pick("0", saved.rangeStart));
    make(rangeEndField_, "End", false, f1, formTop + 0 * stride, pick("100000", saved.rangeEnd));
    make(scanRadiusField_, "Radius", false, f0, formTop + 1 * stride, pick("4", saved.scanRadius));
    make(minScoreField_, "Min Score", true, f1, formTop + 1 * stride, pick("0", saved.minScore));
    const unsigned hwThreads = std::thread::hardware_concurrency();
    const std::string defaultThreads = std::to_string(hwThreads == 0 ? 1u : hwThreads);
    make(threadsField_, "Threads", false, f0, formTop + 2 * stride, pick(defaultThreads.c_str(), saved.threads));
    make(topKField_, "Keep Top", false, f1, formTop + 2 * stride, pick("10", saved.topK));
    make(uniqueBiomeMinField_, "Min Biomes", false, f0, formTop + 3 * stride, pick("", saved.uniqueBiomeMin));
    make(minBiomeCoverageField_, "Min Cover %", true, f1, formTop + 3 * stride, pick("", saved.minBiomeCoverage));
    make(minBiomeRadiusField_, "Min Blob R", false, f0, formTop + 4 * stride, pick("", saved.minBiomeRadius));
    make(minBiomeCompactnessField_, "Min Blob %", true, f1, formTop + 4 * stride, pick("", saved.minBiomeCompactness));

    font::TextRenderer& tr = *textRenderer();
    auto textButtonWidth = [&](const std::string& label, int maxWidth) {
        return std::min(maxWidth, widget::TextSizedActionButtonWidget::measureWidth(tr, label));
    };
    auto addTextButton = [&](int x, int y, int maxWidth, std::string label, std::function<void()> onClick) {
        const int buttonWidth = textButtonWidth(label, maxWidth);
        return &addButton<widget::TextSizedActionButtonWidget>(
            x, y, buttonWidth, layout::kDefaultButtonHeight, std::move(label), std::move(onClick));
    };

    const std::string spawnLabel =
        spawnBiomeIndex_ >= 0 && spawnBiomeIndex_ < static_cast<int>(spawnPickerBiomes().size())
            ? "Spawn: " + prettifyName(spawnPickerBiomes()[static_cast<std::size_t>(spawnBiomeIndex_)])
            : "Spawn: Any";
    const std::string depthText = depthLabel(probeDepth_);
    spawnBiomeButton_ = addTextButton(c0, formTop + 5 * stride, colW, spawnLabel,
        [this] { cycleSpawnBiome(1); });
    depthButton_ = addTextButton(c1, formTop + 5 * stride, colW, depthText,
        [this] {
            probeDepth_ = static_cast<std::uint8_t>((probeDepth_ + 1) % 3);
            if (depthButton_ != nullptr) {
                depthButton_->text = depthLabel(probeDepth_);
                depthButton_->width = widget::TextSizedActionButtonWidget::measureWidth(
                    *textRenderer(), depthButton_->text);
            }
        });
    constexpr const char* kImportLabel = "Import JSON...";
    constexpr const char* kSpecLabel = "Edit rules...";
    importJsonButton_ = addTextButton(c0, formTop + 6 * stride, colW, kImportLabel,
        [this] { tryImportJsonConfig(); });
    specButton_ = addTextButton(c1, formTop + 6 * stride, colW, kSpecLabel,
        [this] { openSpecEditor(); });

    formBottom_ = formTop + 7 * stride + 4;
    resultTop_ = formBottom_ + 10;
    const int listBottom = contentBottom(height());
    if (resultTop_ + kMinResultHeight > listBottom) {
        resultTop_ = std::max(formTop + stride * 2, listBottom - kMinResultHeight);
        formBottom_ = std::max(formTop + stride, resultTop_ - 6);
    }

    constexpr int kFooterGap = 4;
    const int footerY = layout::listFooterRow1Y(height());
    const std::string doneLabel = resource::language::I18n::getTranslation("gui.done");
    const int searchW = widget::TextSizedActionButtonWidget::measureWidth(tr, "Search");
    const int stopW = widget::TextSizedActionButtonWidget::measureWidth(tr, "Stop");
    const int useSeedW = widget::TextSizedActionButtonWidget::measureWidth(tr, "Use Seed");
    const int doneW = widget::TextSizedActionButtonWidget::measureWidth(tr, doneLabel);
    const int footerTotalW = searchW + kFooterGap + stopW + kFooterGap + useSeedW + kFooterGap + doneW;
    int footerX = std::max(4, (width() - footerTotalW) / 2);

    searchButton_ = addTextButton(footerX, footerY, width(), "Search", [this] { startSearch(); });
    footerX += searchW + kFooterGap;
    stopButton_ = addTextButton(footerX, footerY, width(), "Stop",
        [this] {
            stopSearch();
            mergePendingResults();
            if (searchButton_ != nullptr) {
                searchButton_->active = true;
            }
            if (stopButton_ != nullptr) {
                stopButton_->active = false;
            }
            forEachField([](widget::TextFieldWidget& field) { field.enabled = true; });
            if (depthButton_ != nullptr) {
                depthButton_->active = true;
            }
            if (specButton_ != nullptr) {
                specButton_->active = true;
            }
        });
    footerX += stopW + kFooterGap;
    useSeedButton_ = addTextButton(footerX, footerY, width(), "Use Seed", [this] { applySelectedSeed(); });
    footerX += useSeedW + kFooterGap;
    addTextButton(footerX, footerY, width(), doneLabel, [this] { returnToCreateWorld(returnSeedText_); });
}

void SeedfinderScreen::layoutResultList()
{
    if (minecraft() == nullptr) {
        return;
    }
    const int listBottom = contentBottom(height());
    if (resultList_ == nullptr) {
        resultList_ = std::make_unique<ResultListWidget>(
            *this, *minecraft(), width(), height(), resultTop_, listBottom);
        return;
    }
    resultList_->setViewport(width(), height(), resultTop_, listBottom);
    resultList_->setListBounds(kSidebarLeft, kSidebarLeft + kSidebarWidth, kSidebarLeft + 4);
}

void SeedfinderScreen::init()
{
    const bool resizing = !firstInit_;
    const FieldTexts saved = resizing ? snapshotFieldTexts() : FieldTexts {};
    firstInit_ = false;

    if (!resizing) {
        results_.clear();
        pendingResults_.clear();
        selectedResultIndex_ = -1;
        searchFinished_ = false;
        seedsChecked_ = 0;
        specEditorOpen_ = false;
        mapPreviewSeed_ = 0;
        mapPreviewRadius_ = 0;
    }

    layoutForm(resizing ? &saved : nullptr);
    layoutResultList();

    if (biomeMap_ == nullptr) {
        biomeMap_ = std::make_unique<widget::BiomeMapWidget>();
    }

    if (specEditor_ == nullptr) {
        specEditor_ = std::make_unique<widget::SeedSpecEditor>();
    }

    updateSpawnBiomeButton();
    updateSpecButton();

    if (!resizing) {
        if (stopButton_ != nullptr) {
            stopButton_->active = false;
        }
        if (useSeedButton_ != nullptr) {
            useSeedButton_->active = false;
        }
    } else {
        if (stopButton_ != nullptr) {
            stopButton_->active = searchRunning_.load();
        }
        if (searchButton_ != nullptr) {
            searchButton_->active = !searchRunning_.load();
        }
        if (useSeedButton_ != nullptr) {
            useSeedButton_->active =
                selectedResultIndex_ >= 0 && selectedResultIndex_ < static_cast<int>(results_.size());
        }
        const bool fieldsEnabled = !searchRunning_.load();
        forEachField([fieldsEnabled](widget::TextFieldWidget& field) { field.enabled = fieldsEnabled; });
        if (depthButton_ != nullptr) {
            depthButton_->active = fieldsEnabled;
        }
        if (specButton_ != nullptr) {
            specButton_->active = fieldsEnabled && !specEditorOpen_;
        }
        if (importJsonButton_ != nullptr) {
            importJsonButton_->active = fieldsEnabled && !importRunning_.load();
        }
    }
}

void SeedfinderScreen::cycleSpawnBiome(int dir)
{
    const std::vector<std::string>& spawnBiomes = spawnPickerBiomes();
    const int n = static_cast<int>(spawnBiomes.size());
    spawnBiomeIndex_ = (spawnBiomeIndex_ + dir + n) % n;
    updateSpawnBiomeButton();
}

void SeedfinderScreen::updateSpawnBiomeButton()
{
    if (spawnBiomeButton_ == nullptr || textRenderer() == nullptr) {
        return;
    }
    const std::vector<std::string>& spawnBiomes = spawnPickerBiomes();
    const std::string& name = spawnBiomes[static_cast<std::size_t>(spawnBiomeIndex_)];
    spawnBiomeButton_->text = "Spawn: " + (spawnBiomeIndex_ == 0 ? std::string("Any") : prettifyName(name));
    spawnBiomeButton_->width =
        widget::TextSizedActionButtonWidget::measureWidth(*textRenderer(), spawnBiomeButton_->text);
}

void SeedfinderScreen::updateSpecButton()
{
    if (specButton_ == nullptr || textRenderer() == nullptr) {
        return;
    }
    if (specNodes_.empty()) {
        specButton_->text = "Edit rules...";
    } else {
        specButton_->text = std::to_string(specNodes_.size()) + " rule(s) - edit";
    }
    specButton_->width = widget::TextSizedActionButtonWidget::measureWidth(*textRenderer(), specButton_->text);
}

void SeedfinderScreen::openSpecEditor()
{
    if (searchRunning_.load()) {
        return;
    }
    if (specEditor_ == nullptr) {
        specEditor_ = std::make_unique<widget::SeedSpecEditor>();
    }
    specEditor_->setNodes(specNodes_);
    specEditorOpen_ = true;
}

void SeedfinderScreen::closeSpecEditor(bool apply)
{
    if (apply && specEditor_ != nullptr) {
        specNodes_ = specEditor_->nodes();
        updateSpecButton();
    }
    specEditorOpen_ = false;
}

void SeedfinderScreen::applySearchConfig(const seedfinder::config::SearchConfig& cfg)
{
    auto setField = [](std::unique_ptr<widget::TextFieldWidget>& field, const std::string& text) {
        if (field != nullptr) {
            field->setText(text);
        }
    };

    setField(rangeStartField_, std::to_string(cfg.seed_start));
    setField(rangeEndField_, std::to_string(cfg.seed_end));
    setField(scanRadiusField_, std::to_string(std::max(1, cfg.scan_radius_chunks)));
    setField(threadsField_, std::to_string(std::max(1, cfg.threads)));
    setField(topKField_, std::to_string(std::max(1, cfg.top_k)));

    if (cfg.unique_biome_min >= 0) {
        setField(uniqueBiomeMinField_, std::to_string(cfg.unique_biome_min));
    } else {
        setField(uniqueBiomeMinField_, "");
    }
    if (cfg.required_biome_min_coverage_percent >= 0.f) {
        setField(minBiomeCoverageField_, std::to_string(cfg.required_biome_min_coverage_percent));
    } else {
        setField(minBiomeCoverageField_, "");
    }
    if (cfg.required_biome_min_contiguous_radius_chunks >= 1) {
        setField(minBiomeRadiusField_, std::to_string(cfg.required_biome_min_contiguous_radius_chunks));
    } else {
        setField(minBiomeRadiusField_, "");
    }
    if (cfg.required_biome_min_compactness_percent >= 0.f) {
        setField(minBiomeCompactnessField_, std::to_string(cfg.required_biome_min_compactness_percent));
    } else {
        setField(minBiomeCompactnessField_, "");
    }

    probeDepth_ = static_cast<std::uint8_t>(std::min<int>(cfg.probe_depth_max, 2));
    if (depthButton_ != nullptr && textRenderer() != nullptr) {
        depthButton_->text = depthLabel(probeDepth_);
        depthButton_->width =
            widget::TextSizedActionButtonWidget::measureWidth(*textRenderer(), depthButton_->text);
    }

    const std::vector<std::string>& spawnBiomes = spawnPickerBiomes();
    spawnBiomeIndex_ = 0;
    if (!cfg.spawn_biome.empty()) {
        for (int i = 1; i < static_cast<int>(spawnBiomes.size()); ++i) {
            if (spawnBiomes[static_cast<std::size_t>(i)] == cfg.spawn_biome) {
                spawnBiomeIndex_ = i;
                break;
            }
        }
    }
    updateSpawnBiomeButton();

    specNodes_ = cfg.advanced_rule_nodes;
    updateSpecButton();
}

void SeedfinderScreen::tryImportJsonConfig()
{
    if (searchRunning_.load() || importRunning_.load() || specEditorOpen_) {
        return;
    }

    const std::optional<std::filesystem::path> path = msauth::pickJsonFile();
    if (!path.has_value()) {
        return;
    }

    if (importThread_.joinable()) {
        importThread_.join();
    }

    importRunning_ = true;
    importFinished_ = false;
    importStatus_ = "Loading " + path->filename().string() + "...";
    if (importJsonButton_ != nullptr) {
        importJsonButton_->active = false;
    }

    const std::filesystem::path configPath = *path;
    importThread_ = std::thread([this, configPath]() {
        seedfinder::config::LoadResult loaded = seedfinder::config::loadConfigFromFile(configPath.string());
        {
            std::lock_guard<std::mutex> lock(importMutex_);
            pendingImport_ = std::move(loaded);
        }
        importFinished_ = true;
    });
}

void SeedfinderScreen::mergePendingImport()
{
    if (!importFinished_.exchange(false)) {
        return;
    }
    if (importThread_.joinable()) {
        importThread_.join();
    }
    importRunning_ = false;

    seedfinder::config::LoadResult loaded;
    {
        std::lock_guard<std::mutex> lock(importMutex_);
        loaded = std::move(pendingImport_);
    }

    if (!loaded.ok) {
        importStatus_ = "JSON error: " + loaded.error;
    } else {
        applySearchConfig(loaded.config);
        importStatus_ = "Loaded config";
        if (!loaded.config.advanced_rule_errors.empty()) {
            importStatus_ += " (rule warnings)";
        }
    }

    if (importJsonButton_ != nullptr && !searchRunning_.load()) {
        importJsonButton_->active = true;
    }
}

void SeedfinderScreen::tick()
{
    if (!specEditorOpen_) {
        forEachField([](widget::TextFieldWidget& field) { field.tick(); });
    }

    mergePendingResults();
    mergePendingImport();

    if (searchFinished_.exchange(false)) {
        if (searchThread_.joinable()) {
            searchThread_.join();
        }
        searchRunning_ = false;
        if (searchButton_ != nullptr) {
            searchButton_->active = true;
        }
        if (stopButton_ != nullptr) {
            stopButton_->active = false;
        }
        forEachField([](widget::TextFieldWidget& field) { field.enabled = true; });
        if (depthButton_ != nullptr) {
            depthButton_->active = true;
        }
        if (specButton_ != nullptr) {
            specButton_->active = true;
        }
        if (importJsonButton_ != nullptr) {
            importJsonButton_->active = true;
        }
    }
}

void SeedfinderScreen::removed()
{
    stopSearch();
    if (importThread_.joinable()) {
        importThread_.join();
    }
    if (biomeMap_ != nullptr && minecraft() != nullptr) {
        biomeMap_->clear(*minecraft());
    }
}

void SeedfinderScreen::mergePendingResults()
{
    std::vector<SeedResultRow> pending;
    {
        std::lock_guard<std::mutex> lock(resultsMutex_);
        pending.swap(pendingResults_);
    }
    if (pending.empty()) {
        return;
    }
    results_.insert(results_.end(), pending.begin(), pending.end());
    std::sort(results_.begin(), results_.end(), [](const SeedResultRow& a, const SeedResultRow& b) {
        return a.score > b.score;
    });
    const int topK = std::max(1, parseIntField(topKField_.get(), 10));
    if (static_cast<int>(results_.size()) > topK) {
        results_.resize(static_cast<std::size_t>(topK));
    }
    if (selectedResultIndex_ < 0 && !results_.empty()) {
        selectedResultIndex_ = 0;
        if (useSeedButton_ != nullptr) {
            useSeedButton_->active = true;
        }
    }
    if (selectedResultIndex_ >= 0) {
        refreshBiomeMap();
    }
}

void SeedfinderScreen::refreshBiomeMap()
{
    if (biomeMap_ == nullptr || minecraft() == nullptr) {
        return;
    }
    if (selectedResultIndex_ < 0 || selectedResultIndex_ >= static_cast<int>(results_.size())) {
        biomeMap_->clear(*minecraft());
        mapPreviewSeed_ = 0;
        return;
    }

    const SeedResultRow& row = results_[static_cast<std::size_t>(selectedResultIndex_)];
    const int radius = std::max(1, parseIntField(scanRadiusField_.get(), 4));
    if (mapPreviewSeed_ == row.seed && mapPreviewRadius_ == radius && biomeMap_->hasMap()) {
        return;
    }

    biomeMap_->build(*minecraft(), row.seed, row.spawnX, row.spawnZ, radius);
    mapPreviewSeed_ = row.seed;
    mapPreviewRadius_ = radius;
}

void SeedfinderScreen::startSearch()
{
    if (searchRunning_.load()) {
        return;
    }

    stopSearch();
    results_.clear();
    pendingResults_.clear();
    selectedResultIndex_ = -1;
    mapPreviewSeed_ = 0;
    if (biomeMap_ != nullptr && minecraft() != nullptr) {
        biomeMap_->clear(*minecraft());
    }
    seedsChecked_ = 0;
    cancelFlag_ = false;
    searchRunning_ = true;
    searchFinished_ = false;

    if (searchButton_ != nullptr) {
        searchButton_->active = false;
    }
    if (stopButton_ != nullptr) {
        stopButton_->active = true;
    }
    if (useSeedButton_ != nullptr) {
        useSeedButton_->active = false;
    }
    forEachField([](widget::TextFieldWidget& field) { field.enabled = false; });
    if (depthButton_ != nullptr) {
        depthButton_->active = false;
    }
    if (specButton_ != nullptr) {
        specButton_->active = false;
    }
    if (importJsonButton_ != nullptr) {
        importJsonButton_->active = false;
    }

    const seedfinder::config::SearchConfig cfg = buildSearchConfig();
    const float minScore = minScoreThreshold();

    searchThread_ = std::thread([this, cfg, minScore]() {
        seedfinder::engine::SearchEngine engine(cfg);
        engine.setCancelFlag(&cancelFlag_);
        engine.setProgressCounter(&seedsChecked_);
        engine.setHitCallback([this, minScore](const seedfinder::engine::SearchHit& hit) {
            if (hit.score < minScore) {
                return;
            }
            SeedResultRow row;
            row.seed = hit.seed;
            row.score = hit.score;
            row.spawnX = hit.probe.spawn_x;
            row.spawnY = hit.probe.spawn_y;
            row.spawnZ = hit.probe.spawn_z;
            row.biomeId = hit.probe.spawn_biome_id;
            row.allHard = hit.all_hard;
            std::lock_guard<std::mutex> lock(resultsMutex_);
            pendingResults_.push_back(row);
        });
        const seedfinder::engine::SearchSummary summary = engine.run();
        seedsChecked_ = summary.seeds_checked;
        searchFinished_ = true;
    });
}

void SeedfinderScreen::stopSearch()
{
    cancelFlag_ = true;
    if (searchThread_.joinable()) {
        searchThread_.join();
    }
    searchRunning_ = false;
    searchFinished_ = false;
    cancelFlag_ = false;
}

void SeedfinderScreen::returnToCreateWorld(const std::string& seedText)
{
    if (minecraft() == nullptr) {
        return;
    }
    stopSearch();
    const screen::ScreenFactory parent = parentFactory_;
    const std::string worldName = returnWorldName_;
    minecraft()->setScreen(std::make_unique<CreateWorldScreen>(parent, worldName, seedText));
}

void SeedfinderScreen::applySelectedSeed()
{
    if (selectedResultIndex_ < 0 || selectedResultIndex_ >= static_cast<int>(results_.size())) {
        return;
    }
    returnToCreateWorld(std::to_string(results_[static_cast<std::size_t>(selectedResultIndex_)].seed));
}

void SeedfinderScreen::keyPressed(char character, int keyCode)
{
    if (specEditorOpen_) {
        if (escapePressed(keyCode)) {
            closeSpecEditor(false);
        }
        return;
    }
    for (const FormField& def : formFields_) {
        if (def.slot == nullptr || *def.slot == nullptr || !(*def.slot)->focused) {
            continue;
        }
        if (character >= 32 && !charAllowedFor(def, character)) {
            return; // swallow disallowed printable characters (idiot-proof numeric fields)
        }
        (*def.slot)->keyPressed(character, keyCode);
        return;
    }
}

void SeedfinderScreen::mouseClicked(int mouseX, int mouseY, int button)
{
    if (specEditorOpen_) {
        if (specEditor_ != nullptr && textRenderer() != nullptr) {
            const widget::SeedSpecEditor::Result result =
                specEditor_->mouseClicked(*textRenderer(), width(), height(), mouseX, mouseY, button);
            if (result == widget::SeedSpecEditor::Result::Done) {
                closeSpecEditor(true);
            } else if (result == widget::SeedSpecEditor::Result::Cancel) {
                closeSpecEditor(false);
            }
        }
        return;
    }
    Screen::mouseClicked(mouseX, mouseY, button);
    forEachField([&](widget::TextFieldWidget& field) { field.mouseClicked(mouseX, mouseY, button); });
}

void SeedfinderScreen::render(int mouseX, int mouseY, float tickDelta)
{
    if (specEditorOpen_ && specEditor_ != nullptr && textRenderer() != nullptr) {
        specEditor_->render(*this, *textRenderer(), width(), height(), mouseX, mouseY);
        return;
    }

    renderBackground();

    if (resultList_ != nullptr) {
        resultList_->render(mouseX, mouseY, tickDelta);
    }

    if (textRenderer() != nullptr) {
        font::TextRenderer& tr = *textRenderer();
        drawCenteredTextWithShadow(tr, "Seed Finder", width() / 2, 12, 0xFFFFFF);

        // panel behind the search-settings form
        fill(panelX_ - 8, 24, panelX_ + panelW_ + 8, formBottom_ - 4, 0x90101010U);

        const FormField* firstInvalid = nullptr;
        for (const FormField& def : formFields_) {
            const bool ok = fieldValid(def);
            if (!ok && firstInvalid == nullptr) {
                firstInvalid = &def;
            }
            // label sits to the left of the field, right-aligned against it
            const int labelX = def.x - 5 - tr.getWidth(def.label);
            drawTextWithShadow(tr, def.label, labelX, def.y + 6, ok ? 0xC0C0C0 : 0xFF5555);
        }

        drawTextWithShadow(tr, "Seeds", kSidebarLeft, resultTop_ - 12, 0xE0E0E0);
        drawTextWithShadow(tr, "Biome Map", kMapLeft, resultTop_ - 12, 0xE0E0E0);

        if (selectedResultIndex_ >= 0 && selectedResultIndex_ < static_cast<int>(results_.size())) {
            const SeedResultRow& row = results_[static_cast<std::size_t>(selectedResultIndex_)];
            const std::string biome = seedfinder::engine::biomeNameFromId(row.biomeId);
            std::ostringstream mapCaption;
            mapCaption << row.seed << "  spawn (" << row.spawnX << ", " << row.spawnY << ", " << row.spawnZ << ")  "
                       << biome;
            drawTextWithShadow(tr, mapCaption.str(), kMapLeft, resultTop_ + 2, 0xC0C0C0);
        }

        std::ostringstream status;
        if (firstInvalid != nullptr) {
            status << "Check \"" << firstInvalid->label << "\": numbers only";
        } else if (importRunning_.load()) {
            status << importStatus_;
        } else if (searchRunning_.load()) {
            status << "Searching... checked " << seedsChecked_.load() << " seeds";
            if (!results_.empty()) {
                status << ", " << results_.size() << " hit(s)";
            }
        } else if (!importStatus_.empty() && results_.empty()) {
            status << importStatus_;
        } else if (!results_.empty()) {
            status << results_.size() << " result(s), checked " << seedsChecked_.load() << " seeds";
        } else {
            status << "Ready";
        }
        const int statusColor = firstInvalid != nullptr ? 0xFF5555 : 0x808080;
        drawTextWithShadow(tr, status.str(), width() / 2 - 100, layout::listFooterRow2Y(height()), statusColor);
    }

    forEachField([](widget::TextFieldWidget& field) { field.render(); });

    if (biomeMap_ != nullptr && minecraft() != nullptr) {
        const int mapTop = resultTop_ + 12;
        const int mapBottom = contentBottom(height());
        const int mapWidth = std::max(64, width() - kMapLeft - kMapMargin);
        const int mapHeight = std::max(64, mapBottom - mapTop);
        biomeMap_->render(*minecraft(), kMapLeft, mapTop, mapWidth, mapHeight);
    }

    Screen::render(mouseX, mouseY, tickDelta);
}

} // namespace net::minecraft::client::gui::screen::world
