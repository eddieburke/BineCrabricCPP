#include "seedfinder/gui/SeedSpecEditor.hpp"

#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/DrawContext.hpp"
#include "seedfinder/engine/BiomeCatalog.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <utility>

namespace net::minecraft::client::gui::widget {

using seedfinder::config::RuleNode;
using seedfinder::config::RuleNodeKind;

namespace {

// ---- layout constants -------------------------------------------------------
constexpr int kMargin = 24;
constexpr int kTitleY = 13;
constexpr int kContentTop = 32;
constexpr int kFooterH = 32;
constexpr int kCardPad = 8;
constexpr int kHeaderH = 22;
constexpr int kRowH = 20;
constexpr int kRowGap = 3;
constexpr int kCardGap = 8;
constexpr int kArrowW = 14;
constexpr int kIconW = 16;
constexpr int kChipH = 16;

// ---- palette (0xAARRGGBB) ---------------------------------------------------
constexpr std::uint32_t kCardBg = 0xE0242424;
constexpr std::uint32_t kHeaderBg = 0xFF3A3A3A;
constexpr std::uint32_t kBtnBg = 0xFF555555;
constexpr std::uint32_t kValueBg = 0xFF2B2B2B;
constexpr std::uint32_t kChipOn = 0xFF2E7D32;
constexpr std::uint32_t kChipOff = 0xFF404040;
constexpr std::uint32_t kDeleteBg = 0xFF7A2222;
constexpr std::uint32_t kAddBg = 0xFF2E5C8A;
constexpr std::uint32_t kDoneBg = 0xFF2E7D32;
constexpr int kLabelFg = 0xB0B0B0;
constexpr int kValueFg = 0xFFFFFF;
constexpr int kMutedFg = 0x808080;

struct BlockEntry {
    int id;
    const char* name;
};
const std::array<BlockEntry, 23> kBlocks = {{{1, "Stone"}, {2, "Grass"}, {3, "Dirt"}, {4, "Cobblestone"},
    {12, "Sand"}, {13, "Gravel"}, {14, "Gold Ore"}, {15, "Iron Ore"}, {16, "Coal Ore"}, {17, "Wood"},
    {18, "Leaves"}, {24, "Sandstone"}, {49, "Obsidian"}, {56, "Diamond Ore"}, {73, "Redstone Ore"},
    {21, "Lapis Ore"}, {79, "Ice"}, {78, "Snow"}, {81, "Cactus"}, {82, "Clay"}, {87, "Netherrack"},
    {88, "Soul Sand"}, {89, "Glowstone"}}};

std::string prettyBiome(const std::string& name)
{
    std::string out = name;
    std::replace(out.begin(), out.end(), '_', ' ');
    return out;
}

std::string blockName(int id)
{
    for (const BlockEntry& e : kBlocks) {
        if (e.id == id) {
            return e.name;
        }
    }
    return "block " + std::to_string(id);
}

int blockIndex(int id)
{
    for (std::size_t i = 0; i < kBlocks.size(); ++i) {
        if (kBlocks[i].id == id) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

int cycleBlock(int id, int dir)
{
    const int n = static_cast<int>(kBlocks.size());
    int idx = (blockIndex(id) + dir % n + n) % n;
    return kBlocks[static_cast<std::size_t>(idx)].id;
}

const char* kindLabel(RuleNodeKind kind)
{
    switch (kind) {
    case RuleNodeKind::BiomeConstraint:
        return "Biome";
    case RuleNodeKind::BlockConstraint:
        return "Block";
    case RuleNodeKind::BlockOnBlock:
        return "Block-on-Block";
    case RuleNodeKind::MetricObjective:
        return "Objective";
    }
    return "Biome";
}

RuleNode makeDefaultNode(RuleNodeKind kind)
{
    RuleNode node;
    node.kind = kind;
    switch (kind) {
    case RuleNodeKind::BiomeConstraint:
        node.metric = "biome";
        node.op = "any_of";
        node.min_value = -1.0;
        node.max_value = -1.0;
        node.value = -1.0;
        break;
    case RuleNodeKind::BlockConstraint:
        node.metric = "spawn_surface_block_id";
        node.op = "eq";
        node.block_id = 12;
        break;
    case RuleNodeKind::BlockOnBlock:
        node.metric = "block_on_block";
        node.op = "on";
        node.block_id = 12;
        node.block_below_id = 3;
        break;
    case RuleNodeKind::MetricObjective:
        node.metric = "unique_biome_count";
        node.direction = "maximize";
        node.weight = 1.f;
        break;
    }
    return node;
}

const std::array<const char*, 6> kObjectiveMetrics = {
    "unique_biome_count",
    "dominant_biome_percent",
    "biome_chunk_count",
    "biome_coverage_percent",
    "max_contiguous_biome_radius",
    "biome_blob_compactness_percent",
};

int metricIndex(const std::string& metric)
{
    for (std::size_t i = 0; i < kObjectiveMetrics.size(); ++i) {
        if (metric == kObjectiveMetrics[i]) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

std::string cycleMetric(const std::string& metric, int dir)
{
    const int n = static_cast<int>(kObjectiveMetrics.size());
    int idx = (metricIndex(metric) + dir % n + n) % n;
    return kObjectiveMetrics[static_cast<std::size_t>(idx)];
}

const char* metricLabel(const std::string& metric)
{
    if (metric == "unique_biome_count") {
        return "biome variety in scan area";
    }
    if (metric == "dominant_biome_percent") {
        return "dominant biome coverage %";
    }
    if (metric == "biome_chunk_count") {
        return "selected biome chunks in area";
    }
    if (metric == "biome_coverage_percent") {
        return "selected biome coverage %";
    }
    if (metric == "max_contiguous_biome_radius") {
        return "largest contiguous biome blob";
    }
    if (metric == "biome_blob_compactness_percent") {
        return "biome blob compactness %";
    }
    return "biome variety in scan area";
}

bool objectiveNeedsBiomePick(const std::string& metric)
{
    return seedfinder::engine::metricNeedsBiomePick(metric);
}

RuleNodeKind cycleKind(RuleNodeKind kind, int dir)
{
    int k = (static_cast<int>(kind) + dir + 4) % 4;
    return static_cast<RuleNodeKind>(k);
}

std::string stepperText(double v)
{
    if (v < 0.0) {
        return "any";
    }
    return std::to_string(static_cast<long long>(v));
}

} // namespace

void SeedSpecEditor::setNodes(std::vector<RuleNode> nodes)
{
    nodes_ = std::move(nodes);
    scroll_ = 0;
    pendingResult_ = Result::None;
}

void SeedSpecEditor::clampScroll(int screenHeight)
{
    const int viewport = std::max(0, (screenHeight - kFooterH) - kContentTop);
    const int maxScroll = std::max(0, contentHeight_ - viewport);
    scroll_ = std::clamp(scroll_, 0, maxScroll);
}

void SeedSpecEditor::collect(font::TextRenderer& tr, int screenWidth, int screenHeight,
    std::vector<Control>& out)
{
    out.clear();
    const int left = kMargin;
    const int cardW = screenWidth - 2 * kMargin;
    const int innerLeft = left + kCardPad;
    const int innerRight = left + cardW - kCardPad;
    const int valueCol = innerLeft + 96; // left edge of controls after a row label
    const int top0 = kContentTop - scroll_;
    int cy = top0;

    auto addBtn = [&](int x, int y, int w, int h, std::string text, std::uint32_t bg, int fg,
                      bool center, bool fixed, std::function<void()> action) {
        Control c;
        c.x = x;
        c.y = y;
        c.w = w;
        c.h = h;
        c.text = std::move(text);
        c.bg = bg;
        c.fg = fg;
        c.center = center;
        c.action = std::move(action);
        (void)fixed;
        out.push_back(std::move(c));
    };
    auto addLabel = [&](int x, int y, std::string text, int fg) {
        Control c;
        c.x = x;
        c.y = y;
        c.text = std::move(text);
        c.fg = fg;
        out.push_back(std::move(c));
    };

    // A [<] value [>] cycle control. Returns the total width consumed.
    auto addCycle = [&](int x, int y, const std::string& valueText, int valueFg,
                        const std::function<void()>& onLeft, const std::function<void()>& onRight) -> int {
        const int valueW = std::max(58, tr.getWidth(valueText) + 12);
        addBtn(x, y, kArrowW, kRowH - 2, "<", kBtnBg, kValueFg, true, false, onLeft);
        addBtn(x + kArrowW + 1, y, valueW, kRowH - 2, valueText, kValueBg, valueFg, true, false, nullptr);
        addBtn(x + kArrowW + 1 + valueW + 1, y, kArrowW, kRowH - 2, ">", kBtnBg, kValueFg, true, false, onRight);
        return kArrowW + 1 + valueW + 1 + kArrowW;
    };
    // A [-] value [+] stepper.
    auto addStepper = [&](int x, int y, const std::string& valueText,
                          const std::function<void()>& onMinus, const std::function<void()>& onPlus) {
        const int valueW = std::max(50, tr.getWidth(valueText) + 12);
        addBtn(x, y, kArrowW, kRowH - 2, "-", kBtnBg, kValueFg, true, false, onMinus);
        addBtn(x + kArrowW + 1, y, valueW, kRowH - 2, valueText, kValueBg, kValueFg, true, false, nullptr);
        addBtn(x + kArrowW + 1 + valueW + 1, y, kArrowW, kRowH - 2, "+", kBtnBg, kValueFg, true, false, onPlus);
    };

    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        RuleNode& node = nodes_[i];
        const int cardTop = cy;

        // --- measure body rows so we can draw the card panel behind them ---
        int rows = 0;
        int chipRows = 0;
        if (node.kind == RuleNodeKind::BiomeConstraint) {
            rows = 4;
            // chips: lay out to count rows
            int chipX = innerLeft;
            chipRows = 1;
            for (const std::string& name : seedfinder::engine::allCanonicalBiomeNames()) {
                const int w = tr.getWidth(prettyBiome(name)) + 12;
                if (chipX + w > innerRight) {
                    ++chipRows;
                    chipX = innerLeft;
                }
                chipX += w + 4;
            }
        } else if (node.kind == RuleNodeKind::BlockConstraint) {
            rows = (node.metric == "block_histogram") ? 4 : 3;
        } else if (node.kind == RuleNodeKind::BlockOnBlock) {
            rows = 4;
        } else {
            rows = objectiveNeedsBiomePick(node.metric) ? 4 : 3;
        }
        int bodyH = kCardPad + rows * (kRowH + kRowGap);
        if (node.kind == RuleNodeKind::BiomeConstraint) {
            bodyH += kCardPad + chipRows * (kChipH + 4); // "Biomes" label line + chips
        }
        if (node.kind == RuleNodeKind::MetricObjective && objectiveNeedsBiomePick(node.metric)) {
            bodyH += kCardPad + kChipH + 4;
        }
        const int cardH = kHeaderH + bodyH + kCardPad;

        // --- card panel + header strip (drawn before this card's controls) ---
        addBtn(left, cardTop, cardW, cardH, "", kCardBg, 0, false, false, nullptr);
        addBtn(left, cardTop, cardW, kHeaderH, "", kHeaderBg, 0, false, false, nullptr);

        // --- header controls ---
        int hx = innerLeft;
        const int hy = cardTop + 3;
        if (i > 0) {
            addBtn(hx, hy, kIconW, kIconW, "^", kBtnBg, kValueFg, true, false,
                [this, i] { std::swap(nodes_[i], nodes_[i - 1]); });
        }
        hx += kIconW + 2;
        if (i + 1 < nodes_.size()) {
            addBtn(hx, hy, kIconW, kIconW, "v", kBtnBg, kValueFg, true, false,
                [this, i] { std::swap(nodes_[i], nodes_[i + 1]); });
        }
        hx += kIconW + 6;
        addLabel(hx, cardTop + 7, "RULE " + std::to_string(i + 1), kValueFg);

        // delete (right edge) + type cycle (left of delete)
        addBtn(innerRight - kIconW, hy, kIconW, kIconW, "X", kDeleteBg, kValueFg, true, false,
            [this, i] { nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(i)); });
        {
            const std::string kindText = kindLabel(node.kind);
            const int cycleW = kArrowW + 1 + std::max(58, tr.getWidth(kindText) + 12) + 1 + kArrowW;
            const int cx = innerRight - kIconW - 6 - cycleW;
            addCycle(cx, cardTop + 3, kindText, kValueFg,
                [this, i] { nodes_[i] = makeDefaultNode(cycleKind(nodes_[i].kind, -1)); },
                [this, i] { nodes_[i] = makeDefaultNode(cycleKind(nodes_[i].kind, +1)); });
        }

        // --- body ---
        int by = cardTop + kHeaderH + kCardPad;

        if (node.kind == RuleNodeKind::BiomeConstraint) {
            addLabel(innerLeft, by + 5, "Match", kLabelFg);
            addCycle(valueCol, by, node.op == "none_of" ? "none of" : "any of", kValueFg,
                [this, i] { nodes_[i].op = (nodes_[i].op == "none_of") ? "any_of" : "none_of"; },
                [this, i] { nodes_[i].op = (nodes_[i].op == "none_of") ? "any_of" : "none_of"; });
            by += kRowH + kRowGap;

            addLabel(innerLeft, by + 5, "Min cover %", kLabelFg);
            addStepper(valueCol, by, stepperText(node.min_value),
                [this, i] { nodes_[i].min_value = nodes_[i].min_value <= 0.0 ? -1.0 : nodes_[i].min_value - 5.0; },
                [this, i] {
                    nodes_[i].min_value = (nodes_[i].min_value < 0.0 ? 0.0 : nodes_[i].min_value) + 5.0;
                    if (nodes_[i].min_value > 100.0) {
                        nodes_[i].min_value = 100.0;
                    }
                });
            by += kRowH + kRowGap;

            addLabel(innerLeft, by + 5, "Min radius", kLabelFg);
            addStepper(valueCol, by, stepperText(node.value),
                [this, i] { nodes_[i].value = nodes_[i].value <= 0.0 ? -1.0 : nodes_[i].value - 1.0; },
                [this, i] { nodes_[i].value = (nodes_[i].value < 0.0 ? 0.0 : nodes_[i].value) + 1.0; });
            addLabel(valueCol + 120, by + 5, "chunks (contiguous blob)", kMutedFg);
            by += kRowH + kRowGap;

            addLabel(innerLeft, by + 5, "Max cover %", kLabelFg);
            addStepper(valueCol, by, stepperText(node.max_value),
                [this, i] { nodes_[i].max_value = nodes_[i].max_value <= 0.0 ? -1.0 : nodes_[i].max_value - 5.0; },
                [this, i] {
                    nodes_[i].max_value = (nodes_[i].max_value < 0.0 ? 0.0 : nodes_[i].max_value) + 5.0;
                    if (nodes_[i].max_value > 100.0) {
                        nodes_[i].max_value = 100.0;
                    }
                });
            by += kRowH + kRowGap;

            addLabel(innerLeft, by + 2, "Biomes", kLabelFg);
            by += kCardPad + 6;
            int chipX = innerLeft;
            for (const std::string& name : seedfinder::engine::allCanonicalBiomeNames()) {
                const std::string label = prettyBiome(name);
                const int w = tr.getWidth(label) + 12;
                if (chipX + w > innerRight) {
                    chipX = innerLeft;
                    by += kChipH + 4;
                }
                const std::string biome = name;
                const bool on = std::find(node.values.begin(), node.values.end(), biome) != node.values.end();
                addBtn(chipX, by, w, kChipH, label, on ? kChipOn : kChipOff, kValueFg, true, false,
                    [this, i, biome] {
                        auto& vals = nodes_[i].values;
                        const auto it = std::find(vals.begin(), vals.end(), biome);
                        if (it != vals.end()) {
                            vals.erase(it);
                        } else {
                            vals.push_back(biome);
                        }
                    });
                chipX += w + 4;
            }
            by += kChipH + 4;
        } else if (node.kind == RuleNodeKind::BlockConstraint) {
            const bool histogram = node.metric == "block_histogram";
            addLabel(innerLeft, by + 5, "Check", kLabelFg);
            addCycle(valueCol, by, histogram ? "block count in area" : "spawn surface block", kValueFg,
                [this, i] {
                    nodes_[i].metric =
                        (nodes_[i].metric == "block_histogram") ? "spawn_surface_block_id" : "block_histogram";
                },
                [this, i] {
                    nodes_[i].metric =
                        (nodes_[i].metric == "block_histogram") ? "spawn_surface_block_id" : "block_histogram";
                });
            by += kRowH + kRowGap;

            addLabel(innerLeft, by + 5, "Block", kLabelFg);
            addCycle(valueCol, by, blockName(node.block_id), kValueFg,
                [this, i] { nodes_[i].block_id = cycleBlock(nodes_[i].block_id, -1); },
                [this, i] { nodes_[i].block_id = cycleBlock(nodes_[i].block_id, +1); });
            by += kRowH + kRowGap;

            if (histogram) {
                addLabel(innerLeft, by + 5, "Min count", kLabelFg);
                addStepper(valueCol, by, stepperText(node.min_value),
                    [this, i] { nodes_[i].min_value = nodes_[i].min_value <= 0.0 ? -1.0 : nodes_[i].min_value - 1.0; },
                    [this, i] { nodes_[i].min_value = (nodes_[i].min_value < 0.0 ? 0.0 : nodes_[i].min_value) + 1.0; });
                by += kRowH + kRowGap;
                addLabel(innerLeft, by + 5, "Max count", kLabelFg);
                addStepper(valueCol, by, stepperText(node.max_value),
                    [this, i] { nodes_[i].max_value = nodes_[i].max_value <= 0.0 ? -1.0 : nodes_[i].max_value - 1.0; },
                    [this, i] { nodes_[i].max_value = (nodes_[i].max_value < 0.0 ? 0.0 : nodes_[i].max_value) + 1.0; });
                by += kRowH + kRowGap;
            } else {
                addLabel(innerLeft, by + 5, "Must be", kLabelFg);
                addCycle(valueCol, by, node.op == "ne" ? "not this block" : "this block", kValueFg,
                    [this, i] { nodes_[i].op = (nodes_[i].op == "ne") ? "eq" : "ne"; },
                    [this, i] { nodes_[i].op = (nodes_[i].op == "ne") ? "eq" : "ne"; });
                by += kRowH + kRowGap;
            }
        } else if (node.kind == RuleNodeKind::BlockOnBlock) {
            addLabel(innerLeft, by + 5, "Top block", kLabelFg);
            addCycle(valueCol, by, blockName(node.block_id), kValueFg,
                [this, i] { nodes_[i].block_id = cycleBlock(nodes_[i].block_id, -1); },
                [this, i] { nodes_[i].block_id = cycleBlock(nodes_[i].block_id, +1); });
            by += kRowH + kRowGap;
            addLabel(innerLeft, by + 5, "On top of", kLabelFg);
            addCycle(valueCol, by, blockName(node.block_below_id), kValueFg,
                [this, i] { nodes_[i].block_below_id = cycleBlock(nodes_[i].block_below_id, -1); },
                [this, i] { nodes_[i].block_below_id = cycleBlock(nodes_[i].block_below_id, +1); });
            by += kRowH + kRowGap;
            addLabel(innerLeft, by + 5, "Min count", kLabelFg);
            addStepper(valueCol, by, stepperText(node.min_value),
                [this, i] { nodes_[i].min_value = nodes_[i].min_value <= 0.0 ? -1.0 : nodes_[i].min_value - 1.0; },
                [this, i] { nodes_[i].min_value = (nodes_[i].min_value < 0.0 ? 0.0 : nodes_[i].min_value) + 1.0; });
            by += kRowH + kRowGap;
            addLabel(innerLeft, by + 5, "(preview \xE2\x80\x94 not yet scored)", kMutedFg);
            by += kRowH + kRowGap;
        } else { // MetricObjective
            addLabel(innerLeft, by + 5, "Prefer", kLabelFg);
            addCycle(valueCol, by, node.direction == "minimize" ? "less" : "more", kValueFg,
                [this, i] { nodes_[i].direction = (nodes_[i].direction == "minimize") ? "maximize" : "minimize"; },
                [this, i] { nodes_[i].direction = (nodes_[i].direction == "minimize") ? "maximize" : "minimize"; });
            by += kRowH + kRowGap;
            addLabel(innerLeft, by + 5, "Measure", kLabelFg);
            addCycle(valueCol, by, metricLabel(node.metric), kValueFg,
                [this, i] {
                    nodes_[i].metric = cycleMetric(nodes_[i].metric, -1);
                    if (!objectiveNeedsBiomePick(nodes_[i].metric)) {
                        nodes_[i].values.clear();
                    } else if (nodes_[i].values.empty()) {
                        nodes_[i].values.push_back("desert");
                    }
                },
                [this, i] {
                    nodes_[i].metric = cycleMetric(nodes_[i].metric, +1);
                    if (!objectiveNeedsBiomePick(nodes_[i].metric)) {
                        nodes_[i].values.clear();
                    } else if (nodes_[i].values.empty()) {
                        nodes_[i].values.push_back("desert");
                    }
                });
            by += kRowH + kRowGap;
            if (objectiveNeedsBiomePick(node.metric)) {
                addLabel(innerLeft, by + 2, "Target biome", kLabelFg);
                by += kCardPad + 6;
                int chipX = innerLeft;
                for (const std::string& name : seedfinder::engine::allCanonicalBiomeNames()) {
                    const std::string label = prettyBiome(name);
                    const int w = tr.getWidth(label) + 12;
                    if (chipX + w > innerRight) {
                        chipX = innerLeft;
                        by += kChipH + 4;
                    }
                    const std::string biome = name;
                    const bool on = node.values.size() == 1 && node.values[0] == biome;
                    addBtn(chipX, by, w, kChipH, label, on ? kChipOn : kChipOff, kValueFg, true, false,
                        [this, i, biome] { nodes_[i].values = {biome}; });
                    chipX += w + 4;
                }
                by += kChipH + 4;
            }
            addLabel(innerLeft, by + 5, "Weight", kLabelFg);
            addStepper(valueCol, by, std::to_string(static_cast<long long>(node.weight <= 0.f ? 1.f : node.weight)),
                [this, i] { nodes_[i].weight = std::max(1.f, nodes_[i].weight - 1.f); },
                [this, i] { nodes_[i].weight = std::min(20.f, (nodes_[i].weight <= 0.f ? 1.f : nodes_[i].weight) + 1.f); });
            by += kRowH + kRowGap;
        }

        cy = cardTop + cardH + kCardGap;
    }

    // "+ Add Rule" button at the bottom of the scrolled content.
    addBtn(left, cy, cardW, kRowH + 4, "+ Add Rule", kAddBg, kValueFg, true, false,
        [this] { nodes_.push_back(makeDefaultNode(RuleNodeKind::BiomeConstraint)); });
    cy += kRowH + 4;

    contentHeight_ = cy - top0;

    // ---- fixed footer: Cancel | Done ----
    const int footerY = screenHeight - kFooterH + 5;
    const int btnW = 110;
    const int gap = 8;
    const int totalW = btnW * 2 + gap;
    const int fx = (screenWidth - totalW) / 2;
    addBtn(fx, footerY, btnW, kRowH, "Cancel", kBtnBg, kValueFg, true, true,
        [this] { pendingResult_ = Result::Cancel; });
    addBtn(fx + btnW + gap, footerY, btnW, kRowH, "Done", kDoneBg, kValueFg, true, true,
        [this] { pendingResult_ = Result::Done; });

    // scroll arrows (right edge)
    addBtn(screenWidth - kMargin + 2, kContentTop, kIconW, kIconW, "^", kBtnBg, kValueFg, true, true,
        [this] { scroll_ -= 40; });
    addBtn(screenWidth - kMargin + 2, screenHeight - kFooterH - kIconW, kIconW, kIconW, "v", kBtnBg, kValueFg,
        true, true, [this] { scroll_ += 40; });
}

void SeedSpecEditor::render(gui::DrawContext& ctx, font::TextRenderer& tr,
    int screenWidth, int screenHeight, int mouseX, int mouseY)
{
    clampScroll(screenHeight);

    ctx.fillGradient(0, 0, screenWidth, screenHeight, 0xFF12121AU, 0xFF1C1C2AU);

    std::vector<Control> controls;
    collect(tr, screenWidth, screenHeight, controls);

    const int viewTop = kContentTop;
    const int viewBottom = screenHeight - kFooterH;

    for (const Control& c : controls) {
        // cull scrolled controls outside the viewport (footer/scroll arrows live below it)
        const bool inFooter = c.y >= viewBottom - 1;
        if (!inFooter && (c.y + c.h <= viewTop || c.y >= viewBottom)) {
            continue;
        }
        if (c.bg != 0) {
            ctx.fill(c.x, c.y, c.x + c.w, c.y + c.h, c.bg);
            const bool hover = c.action && mouseX >= c.x && mouseX < c.x + c.w
                && mouseY >= c.y && mouseY < c.y + c.h;
            if (hover) {
                ctx.fill(c.x, c.y, c.x + c.w, c.y + c.h, 0x30FFFFFFU);
            }
        }
        if (!c.text.empty()) {
            const int ty = c.y + (c.h > 0 ? (c.h - 8) / 2 : 0);
            if (c.center) {
                ctx.drawCenteredTextWithShadow(tr, c.text, c.x + c.w / 2, ty, c.fg);
            } else {
                ctx.drawTextWithShadow(tr, c.text, c.x, c.y, c.fg);
            }
        }
    }

    // mask any card overflow above the content area, then draw the title.
    ctx.fill(0, 0, screenWidth, viewTop, 0xFF12121AU);
    ctx.drawCenteredTextWithShadow(tr, "Seed Specification \xE2\x80\x94 build rules, no typing", screenWidth / 2,
        kTitleY, 0xFFFFFF);
    if (nodes_.empty()) {
        ctx.drawCenteredTextWithShadow(tr, "No rules yet. Press \"+ Add Rule\" to start.", screenWidth / 2,
            kContentTop + 24, kMutedFg);
    }
    ctx.fill(0, viewBottom, screenWidth, screenHeight, 0xFF12121AU);

    // redraw footer controls on top of the mask
    for (const Control& c : controls) {
        if (c.y < viewBottom - 1) {
            continue;
        }
        if (c.bg != 0) {
            ctx.fill(c.x, c.y, c.x + c.w, c.y + c.h, c.bg);
            const bool hover = c.action && mouseX >= c.x && mouseX < c.x + c.w
                && mouseY >= c.y && mouseY < c.y + c.h;
            if (hover) {
                ctx.fill(c.x, c.y, c.x + c.w, c.y + c.h, 0x30FFFFFFU);
            }
        }
        if (!c.text.empty()) {
            const int ty = c.y + (c.h - 8) / 2;
            ctx.drawCenteredTextWithShadow(tr, c.text, c.x + c.w / 2, ty, c.fg);
        }
    }
}

SeedSpecEditor::Result SeedSpecEditor::mouseClicked(font::TextRenderer& tr,
    int screenWidth, int screenHeight, int mouseX, int mouseY, int button)
{
    if (button != 0) {
        return Result::None;
    }
    pendingResult_ = Result::None;

    std::vector<Control> controls;
    collect(tr, screenWidth, screenHeight, controls);

    const int viewTop = kContentTop;
    const int viewBottom = screenHeight - kFooterH;

    // footer/scroll controls sit on top, so test them first
    for (const Control& c : controls) {
        if (c.y < viewBottom - 1 || !c.action) {
            continue;
        }
        if (mouseX >= c.x && mouseX < c.x + c.w && mouseY >= c.y && mouseY < c.y + c.h) {
            c.action();
            clampScroll(screenHeight);
            return pendingResult_;
        }
    }
    for (const Control& c : controls) {
        if (c.y >= viewBottom - 1 || !c.action) {
            continue;
        }
        if (c.y + c.h <= viewTop || c.y >= viewBottom) {
            continue; // culled
        }
        if (mouseX >= c.x && mouseX < c.x + c.w && mouseY >= c.y && mouseY < c.y + c.h) {
            c.action();
            clampScroll(screenHeight);
            return Result::None;
        }
    }
    return Result::None;
}

} // namespace net::minecraft::client::gui::widget
