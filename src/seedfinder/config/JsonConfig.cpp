#include "seedfinder/config/JsonConfig.hpp"

#include "seedfinder/engine/SeedString.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

namespace seedfinder::config {
namespace {

class JsonParser {
public:
    explicit JsonParser(const std::string& text)
        : text_(text)
    {
    }

    bool parseObject(std::unordered_map<std::string, std::string>& out)
    {
        skipWs();
        if (!consume('{')) {
            return false;
        }
        skipWs();
        if (peek() == '}') {
            ++pos_;
            return true;
        }
        for (;;) {
            std::string key;
            if (!parseString(key)) {
                return false;
            }
            skipWs();
            if (!consume(':')) {
                return false;
            }
            std::string value;
            if (!parseValue(value)) {
                return false;
            }
            out[key] = value;
            skipWs();
            if (peek() == '}') {
                ++pos_;
                return true;
            }
            if (!consume(',')) {
                return false;
            }
        }
    }

private:
    const std::string& text_;
    std::size_t pos_ = 0;

    char peek() const
    {
        return pos_ < text_.size() ? text_[pos_] : '\0';
    }

    void skipWs()
    {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    bool consume(char c)
    {
        skipWs();
        if (peek() != c) {
            return false;
        }
        ++pos_;
        return true;
    }

    bool parseString(std::string& out)
    {
        skipWs();
        if (peek() != '"') {
            return false;
        }
        ++pos_;
        out.clear();
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"') {
                return true;
            }
            if (c == '\\' && pos_ < text_.size()) {
                out.push_back(text_[pos_++]);
                continue;
            }
            out.push_back(c);
        }
        return false;
    }

    bool parseValue(std::string& out)
    {
        skipWs();
        if (peek() == '"') {
            return parseString(out);
        }
        if (peek() == '[') {
            return parseArray(out);
        }
        if (peek() == '{') {
            return parseNestedObject(out);
        }
        return parsePrimitive(out);
    }

    bool parsePrimitive(std::string& out)
    {
        skipWs();
        const std::size_t start = pos_;
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if (c == ',' || c == '}' || c == ']' || std::isspace(static_cast<unsigned char>(c))) {
                break;
            }
            ++pos_;
        }
        if (start == pos_) {
            return false;
        }
        out = text_.substr(start, pos_ - start);
        return true;
    }

    bool parseArray(std::string& out)
    {
        if (!consume('[')) {
            return false;
        }
        std::ostringstream joined;
        bool first = true;
        skipWs();
        if (peek() == ']') {
            ++pos_;
            out.clear();
            return true;
        }
        for (;;) {
            std::string item;
            if (!parseValue(item)) {
                return false;
            }
            if (!first) {
                joined << ',';
            }
            first = false;
            joined << item;
            skipWs();
            if (peek() == ']') {
                ++pos_;
                out = joined.str();
                return true;
            }
            if (!consume(',')) {
                return false;
            }
        }
    }

    bool parseNestedObject(std::string& out)
    {
        const std::size_t start = pos_;
        int depth = 0;
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '{') {
                ++depth;
            } else if (c == '}') {
                --depth;
                if (depth == 0) {
                    out = text_.substr(start, pos_ - start);
                    return true;
                }
            }
        }
        return false;
    }
};

bool parseBool(const std::string& v, bool& out)
{
    if (v == "true") {
        out = true;
        return true;
    }
    if (v == "false") {
        out = false;
        return true;
    }
    return false;
}

bool parseInt(const std::string& v, int& out)
{
    try {
        out = std::stoi(v);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseFloat(const std::string& v, float& out)
{
    try {
        out = std::stof(v);
        return true;
    } catch (...) {
        return false;
    }
}

void splitCsv(const std::string& v, std::vector<std::string>& out)
{
    out.clear();
    std::string item;
    for (char c : v) {
        if (c == ',') {
            if (!item.empty()) {
                out.push_back(item);
                item.clear();
            }
        } else {
            item.push_back(c);
        }
    }
    if (!item.empty()) {
        out.push_back(item);
    }
}

SeedProbeDepth parseDepth(const std::string& v)
{
    if (v == "biome_only" || v == "0") {
        return kDepthBiomeOnly;
    }
    if (v == "full_decorate" || v == "2") {
        return kDepthFullDecorate;
    }
    return kDepthTerrain;
}

class NbtParser {
public:
    explicit NbtParser(const std::string& text)
        : text_(text)
    {
    }

    bool parse(NbtValue& out)
    {
        skipWs();
        if (!parseValue(out)) {
            return false;
        }
        skipWs();
        return pos_ == text_.size();
    }

private:
    const std::string& text_;
    std::size_t pos_ = 0;

    char peek() const
    {
        return pos_ < text_.size() ? text_[pos_] : '\0';
    }

    void skipWs()
    {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_])) != 0) {
            ++pos_;
        }
    }

    bool consume(char expected)
    {
        skipWs();
        if (peek() != expected) {
            return false;
        }
        ++pos_;
        return true;
    }

    bool parseString(std::string& out)
    {
        if (!consume('"')) {
            return false;
        }
        out.clear();
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"') {
                return true;
            }
            if (c == '\\' && pos_ < text_.size()) {
                out.push_back(text_[pos_++]);
                continue;
            }
            out.push_back(c);
        }
        return false;
    }

    bool parseNumber(double& out)
    {
        skipWs();
        const std::size_t start = pos_;
        if (peek() == '+' || peek() == '-') {
            ++pos_;
        }
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if ((c >= '0' && c <= '9') || c == '.') {
                ++pos_;
            } else {
                break;
            }
        }
        if (start == pos_) {
            return false;
        }
        try {
            out = std::stod(text_.substr(start, pos_ - start));
            return true;
        } catch (...) {
            return false;
        }
    }

    bool startsWith(const char* token) const
    {
        std::size_t i = 0;
        while (token[i] != '\0') {
            if (pos_ + i >= text_.size() || text_[pos_ + i] != token[i]) {
                return false;
            }
            ++i;
        }
        return true;
    }

    bool parseValue(NbtValue& out)
    {
        skipWs();
        if (peek() == '{') {
            return parseCompound(out);
        }
        if (peek() == '[') {
            return parseList(out);
        }
        if (peek() == '"') {
            out.kind = NbtValueKind::String;
            return parseString(out.string_value);
        }
        if (startsWith("true")) {
            out.kind = NbtValueKind::Bool;
            out.bool_value = true;
            pos_ += 4;
            return true;
        }
        if (startsWith("false")) {
            out.kind = NbtValueKind::Bool;
            out.bool_value = false;
            pos_ += 5;
            return true;
        }
        out.kind = NbtValueKind::Number;
        return parseNumber(out.number_value);
    }

    bool parseCompound(NbtValue& out)
    {
        if (!consume('{')) {
            return false;
        }
        out = NbtValue {};
        out.kind = NbtValueKind::Compound;
        skipWs();
        if (peek() == '}') {
            ++pos_;
            return true;
        }
        for (;;) {
            std::string key;
            if (!parseString(key)) {
                return false;
            }
            if (!consume(':')) {
                return false;
            }
            NbtValue value;
            if (!parseValue(value)) {
                return false;
            }
            out.compound.emplace(std::move(key), std::move(value));
            skipWs();
            if (peek() == '}') {
                ++pos_;
                return true;
            }
            if (!consume(',')) {
                return false;
            }
        }
    }

    bool parseList(NbtValue& out)
    {
        if (!consume('[')) {
            return false;
        }
        out = NbtValue {};
        out.kind = NbtValueKind::List;
        skipWs();
        if (peek() == ']') {
            ++pos_;
            return true;
        }
        for (;;) {
            NbtValue item;
            if (!parseValue(item)) {
                return false;
            }
            out.list.push_back(std::move(item));
            skipWs();
            if (peek() == ']') {
                ++pos_;
                return true;
            }
            if (!consume(',')) {
                return false;
            }
        }
    }
};

const NbtValue* compoundGet(const NbtValue& compound, const char* key)
{
    if (compound.kind != NbtValueKind::Compound) {
        return nullptr;
    }
    const auto it = compound.compound.find(key);
    return it != compound.compound.end() ? &it->second : nullptr;
}

std::string nbtString(const NbtValue* node)
{
    if (node != nullptr && node->kind == NbtValueKind::String) {
        return node->string_value;
    }
    return {};
}

double nbtNumber(const NbtValue* node, double fallback = -1.0)
{
    if (node != nullptr && node->kind == NbtValueKind::Number) {
        return node->number_value;
    }
    return fallback;
}

std::vector<std::string> nbtStringList(const NbtValue* node)
{
    std::vector<std::string> out;
    if (node == nullptr) {
        return out;
    }
    if (node->kind == NbtValueKind::String) {
        out.push_back(node->string_value);
        return out;
    }
    if (node->kind != NbtValueKind::List) {
        return out;
    }
    for (const NbtValue& item : node->list) {
        if (item.kind == NbtValueKind::String && !item.string_value.empty()) {
            out.push_back(item.string_value);
        }
    }
    return out;
}

void addRuleNodeFromNbt(const NbtValue& source, std::vector<RuleNode>& out)
{
    const std::string type = nbtString(compoundGet(source, "type"));
    if (type == "biome_constraint") {
        RuleNode node;
        node.kind = RuleNodeKind::BiomeConstraint;
        node.metric = "biome";
        node.op = nbtString(compoundGet(source, "op"));
        node.values = nbtStringList(compoundGet(source, "biomes"));
        if (node.values.empty()) {
            node.values = nbtStringList(compoundGet(source, "values"));
        }
        node.min_value = nbtNumber(compoundGet(source, "min_coverage_percent"), -1.0);
        if (node.min_value < 0.0) {
            node.min_value = nbtNumber(compoundGet(source, "min"), -1.0);
        }
        node.max_value = nbtNumber(compoundGet(source, "max_coverage_percent"), -1.0);
        if (node.max_value < 0.0) {
            node.max_value = nbtNumber(compoundGet(source, "max"), -1.0);
        }
        node.value = nbtNumber(compoundGet(source, "min_contiguous_radius_chunks"), -1.0);
        if (node.value < 0.0) {
            node.value = nbtNumber(compoundGet(source, "value"), -1.0);
        }
        out.push_back(std::move(node));
    } else if (type == "block_constraint") {
        RuleNode node;
        node.kind = RuleNodeKind::BlockConstraint;
        node.metric = nbtString(compoundGet(source, "metric"));
        node.op = nbtString(compoundGet(source, "op"));
        node.block_id = static_cast<int>(nbtNumber(compoundGet(source, "block_id"), -1.0));
        node.min_value = nbtNumber(compoundGet(source, "min"), -1.0);
        node.max_value = nbtNumber(compoundGet(source, "max"), -1.0);
        node.value = nbtNumber(compoundGet(source, "value"), 0.0);
        out.push_back(std::move(node));
    } else if (type == "block_on_block") {
        RuleNode node;
        node.kind = RuleNodeKind::BlockOnBlock;
        node.metric = "block_on_block";
        node.op = nbtString(compoundGet(source, "op"));
        node.block_id = static_cast<int>(nbtNumber(compoundGet(source, "top_block_id"), -1.0));
        node.block_below_id = static_cast<int>(nbtNumber(compoundGet(source, "bottom_block_id"), -1.0));
        node.min_value = nbtNumber(compoundGet(source, "min"), -1.0);
        out.push_back(std::move(node));
    } else if (type == "objective") {
        RuleNode node;
        node.kind = RuleNodeKind::MetricObjective;
        node.metric = nbtString(compoundGet(source, "metric"));
        node.direction = nbtString(compoundGet(source, "direction"));
        node.values = nbtStringList(compoundGet(source, "biomes"));
        const double weight = nbtNumber(compoundGet(source, "weight"), 1.0);
        node.weight = weight > 0.0 ? static_cast<float>(weight) : 1.f;
        out.push_back(std::move(node));
    }
}

void applyKey(SearchConfig& cfg, const std::string& key, const std::string& value)
{
    cfg.raw_values[key] = value;
    const ParamDescriptor* desc = findParamByKey(key);
    if (desc != nullptr && desc->wire_status == ParamWireStatus::SchemaOnly) {
        cfg.schema_only_keys.push_back(key);
    }
    if (desc != nullptr && desc->wire_status == ParamWireStatus::Stub) {
        cfg.notes.push_back("stub param accepted: " + key);
    }

    if (key == "seed_range_start") {
        cfg.seed_start = parseSeedToken(value);
    } else if (key == "seed_range_end") {
        cfg.seed_end = parseSeedToken(value);
    } else if (key == "dimension") {
        cfg.dimension = value;
    } else if (key == "scan_origin_x") {
        parseInt(value, cfg.scan_origin_x);
    } else if (key == "scan_origin_z") {
        parseInt(value, cfg.scan_origin_z);
    } else if (key == "scan_radius_chunks") {
        parseInt(value, cfg.scan_radius_chunks);
    } else if (key == "probe_depth") {
        cfg.probe_depth_max = parseDepth(value);
    } else if (key == "compute_spawn") {
        parseBool(value, cfg.compute_spawn);
    } else if (key == "spawn_biome") {
        cfg.spawn_biome = value;
    } else if (key == "spawn_biome_in") {
        splitCsv(value, cfg.spawn_biome_in);
    } else if (key == "spawn_x") {
        parseInt(value, cfg.spawn_x);
    } else if (key == "spawn_z") {
        parseInt(value, cfg.spawn_z);
    } else if (key == "spawn_surface_block") {
        cfg.spawn_surface_block = value;
    } else if (key == "spawn_y_min") {
        parseInt(value, cfg.spawn_y_min);
    } else if (key == "spawn_y_max") {
        parseInt(value, cfg.spawn_y_max);
    } else if (key == "spawn_distance_chunks") {
        parseInt(value, cfg.spawn_distance_chunks_max);
    } else if (key == "spawn_chunk_x") {
        parseInt(value, cfg.spawn_chunk_x);
    } else if (key == "biome_at_x") {
        parseInt(value, cfg.biome_at_x);
    } else if (key == "biome_at_z") {
        parseInt(value, cfg.biome_at_z);
    } else if (key == "biome_at_required") {
        cfg.biome_at_required = value;
    } else if (key == "unique_biome_min") {
        parseInt(value, cfg.unique_biome_min);
    } else if (key == "unique_biome_max") {
        parseInt(value, cfg.unique_biome_max);
    } else if (key == "temperature_min") {
        parseFloat(value, cfg.temperature_min);
    } else if (key == "temperature_max") {
        parseFloat(value, cfg.temperature_max);
    } else if (key == "downfall_min") {
        parseFloat(value, cfg.downfall_min);
    } else if (key == "downfall_max") {
        parseFloat(value, cfg.downfall_max);
    } else if (key == "require_snow") {
        parseBool(value, cfg.require_snow);
    } else if (key == "require_rain") {
        parseBool(value, cfg.require_rain);
    } else if (key == "forbidden_biomes") {
        splitCsv(value, cfg.forbidden_biomes);
    } else if (key == "required_biomes") {
        splitCsv(value, cfg.required_biomes);
    } else if (key == "required_biome_min_coverage_percent") {
        parseFloat(value, cfg.required_biome_min_coverage_percent);
    } else if (key == "forbidden_biome_max_coverage_percent") {
        parseFloat(value, cfg.forbidden_biome_max_coverage_percent);
    } else if (key == "required_biome_min_contiguous_radius_chunks") {
        parseInt(value, cfg.required_biome_min_contiguous_radius_chunks);
    } else if (key == "required_biome_min_compactness_percent") {
        parseFloat(value, cfg.required_biome_min_compactness_percent);
    } else if (key == "avg_surface_y_min") {
        parseFloat(value, cfg.avg_surface_y_min);
    } else if (key == "avg_surface_y_max") {
        parseFloat(value, cfg.avg_surface_y_max);
    } else if (key == "flatness_max") {
        parseFloat(value, cfg.flatness_max);
    } else if (key == "underwater_percent_max") {
        parseFloat(value, cfg.underwater_percent_max);
    } else if (key == "cave_proxy_min") {
        parseInt(value, cfg.cave_proxy_min);
    } else if (key == "threads") {
        parseInt(value, cfg.threads);
    } else if (key == "top_k") {
        parseInt(value, cfg.top_k);
    } else if (key == "checkpoint") {
        cfg.checkpoint_path = value;
    } else if (key == "output") {
        cfg.output_path = value;
    } else if (key == "search_mode") {
        cfg.search_mode = value;
    } else if (key == "advanced_rule_data_nbt") {
        cfg.advanced_rule_data_nbt = value;
        cfg.advanced_rule_nodes.clear();
        cfg.advanced_rule_errors.clear();
        if (!parseAdvancedRuleDataNbt(value, cfg.advanced_rule_nodes, cfg.advanced_rule_errors)) {
            cfg.notes.push_back("advanced_rule_data_nbt parse failed");
        }
    }
}

LoadResult buildFromMap(const std::unordered_map<std::string, std::string>& map)
{
    LoadResult result;
    result.config = SearchConfig {};
    for (const auto& entry : map) {
        applyKey(result.config, entry.first, entry.second);
    }
    if (result.config.dimension != "overworld") {
        result.ok = false;
        result.error = "v1 supports overworld dimension only";
        return result;
    }
    if (result.config.probe_depth_max > kDepthTerrain) {
        result.config.notes.push_back("probe_depth capped to terrain in v1");
        result.config.probe_depth_max = kDepthTerrain;
    }
    if (result.config.threads < 1) {
        result.config.threads = 1;
    }
    for (const std::string& error : result.config.advanced_rule_errors) {
        result.config.notes.push_back(error);
    }
    result.ok = true;
    return result;
}

} // namespace

const ParamDescriptor* findParamByKey(const std::string& key)
{
    for (const ParamDescriptor& desc : kParamRegistry) {
        if (key == desc.json_key) {
            return &desc;
        }
    }
    return nullptr;
}

bool parseAdvancedRuleDataNbt(
    const std::string& blob,
    std::vector<RuleNode>& out,
    std::vector<std::string>& errors)
{
    out.clear();
    errors.clear();
    if (blob.empty()) {
        return true;
    }

    NbtValue root;
    NbtParser parser(blob);
    if (!parser.parse(root)) {
        errors.push_back("advanced_rule_data_nbt: invalid payload");
        return false;
    }
    if (root.kind != NbtValueKind::Compound) {
        errors.push_back("advanced_rule_data_nbt: root must be compound");
        return false;
    }
    const NbtValue* rules = compoundGet(root, "rules");
    if (rules != nullptr && rules->kind == NbtValueKind::List) {
        for (const NbtValue& item : rules->list) {
            if (item.kind == NbtValueKind::Compound) {
                addRuleNodeFromNbt(item, out);
            }
        }
    }
    const NbtValue* objectives = compoundGet(root, "objectives");
    if (objectives != nullptr && objectives->kind == NbtValueKind::List) {
        for (const NbtValue& item : objectives->list) {
            if (item.kind == NbtValueKind::Compound) {
                addRuleNodeFromNbt(item, out);
            }
        }
    }
    if (out.empty()) {
        errors.push_back("advanced_rule_data_nbt: no supported rules/objectives");
        return false;
    }
    return true;
}

namespace {

std::string nbtEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 2);
    for (const char c : s) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

std::string formatNbtNumber(double v)
{
    const long long iv = static_cast<long long>(v);
    if (static_cast<double>(iv) == v) {
        return std::to_string(iv);
    }
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

void appendStringList(std::ostringstream& out, const std::vector<std::string>& values)
{
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            out << ',';
        }
        out << '"' << nbtEscape(values[i]) << '"';
    }
    out << ']';
}

} // namespace

std::string serializeAdvancedRuleDataNbt(const std::vector<RuleNode>& nodes)
{
    std::ostringstream rules;
    std::ostringstream objectives;
    int ruleCount = 0;
    int objectiveCount = 0;

    for (const RuleNode& node : nodes) {
        if (node.kind == RuleNodeKind::MetricObjective) {
            if (objectiveCount++ != 0) {
                objectives << ',';
            }
            objectives << "{\"type\":\"objective\""
                       << ",\"metric\":\"" << nbtEscape(node.metric) << '"'
                       << ",\"direction\":\"" << nbtEscape(node.direction.empty() ? "maximize" : node.direction) << '"'
                       << ",\"weight\":" << formatNbtNumber(node.weight > 0.f ? node.weight : 1.0);
            if (!node.values.empty()) {
                objectives << ",\"biomes\":";
                appendStringList(objectives, node.values);
            }
            objectives << '}';
            continue;
        }

        if (ruleCount++ != 0) {
            rules << ',';
        }
        switch (node.kind) {
        case RuleNodeKind::BiomeConstraint:
            rules << "{\"type\":\"biome_constraint\""
                  << ",\"op\":\"" << nbtEscape(node.op.empty() ? "any_of" : node.op) << '"'
                  << ",\"biomes\":";
            appendStringList(rules, node.values);
            if (node.min_value >= 0.0) {
                rules << ",\"min_coverage_percent\":" << formatNbtNumber(node.min_value);
            }
            if (node.max_value >= 0.0) {
                rules << ",\"max_coverage_percent\":" << formatNbtNumber(node.max_value);
            }
            if (node.value >= 1.0) {
                rules << ",\"min_contiguous_radius_chunks\":" << formatNbtNumber(node.value);
            }
            rules << '}';
            break;
        case RuleNodeKind::BlockConstraint:
            rules << "{\"type\":\"block_constraint\""
                  << ",\"metric\":\"" << nbtEscape(node.metric.empty() ? "spawn_surface_block_id" : node.metric) << '"'
                  << ",\"op\":\"" << nbtEscape(node.op.empty() ? "eq" : node.op) << '"'
                  << ",\"block_id\":" << node.block_id
                  << ",\"min\":" << formatNbtNumber(node.min_value)
                  << ",\"max\":" << formatNbtNumber(node.max_value)
                  << ",\"value\":" << formatNbtNumber(node.value) << '}';
            break;
        case RuleNodeKind::BlockOnBlock:
            rules << "{\"type\":\"block_on_block\""
                  << ",\"op\":\"" << nbtEscape(node.op.empty() ? "on" : node.op) << '"'
                  << ",\"top_block_id\":" << node.block_id
                  << ",\"bottom_block_id\":" << node.block_below_id
                  << ",\"min\":" << formatNbtNumber(node.min_value) << '}';
            break;
        case RuleNodeKind::MetricObjective:
            break;
        }
    }

    std::ostringstream out;
    out << "{\"rules\":[" << rules.str() << "],\"objectives\":[" << objectives.str() << "]}";
    return out.str();
}

LoadResult loadConfigFromString(const std::string& json)
{
    JsonParser parser(json);
    std::unordered_map<std::string, std::string> map;
    if (!parser.parseObject(map)) {
        return {false, "invalid JSON object", {}};
    }
    return buildFromMap(map);
}

LoadResult loadConfigFromFile(const std::string& path)
{
    std::ifstream in(path);
    if (!in) {
        return {false, "cannot open config: " + path, {}};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return loadConfigFromString(buffer.str());
}

} // namespace seedfinder::config
