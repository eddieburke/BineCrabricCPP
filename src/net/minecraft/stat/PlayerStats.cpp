#include "net/minecraft/stat/PlayerStats.hpp"
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include "net/minecraft/stat/StatFile.hpp"
namespace net::minecraft::stat {
namespace {
std::string toLower(std::string value) {
 for(char& ch : value) {
  ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
 }
 return value;
}
bool parseIntField(const std::string& objectText, int& outValue) {
 std::size_t colon = objectText.find(':');
 if(colon == std::string::npos) {
  return false;
 }
 ++colon;
 while(colon < objectText.size() && std::isspace(static_cast<unsigned char>(objectText[colon]))) {
  ++colon;
 }
 std::size_t end = colon;
 while(end < objectText.size() &&
       (std::isdigit(static_cast<unsigned char>(objectText[end])) || objectText[end] == '-')) {
  ++end;
 }
 if(end == colon) {
  return false;
 }
 outValue = std::stoi(objectText.substr(colon, end - colon));
 return true;
}
} // namespace
PlayerStats::PlayerStats(const client::util::Session& session, std::filesystem::path runDirectory)
    : session_(session), runDirectory_(std::move(runDirectory)), statsDirectory_(runDirectory_ / "stats") {
 Stats::initialize();
 std::filesystem::create_directories(statsDirectory_);
 relocateLegacyStats();
 statsFile_ = statsDirectory_ / ("stats_" + usernameKey() + ".dat");
 loadFromDisk();
}
PlayerStats::~PlayerStats() {
 if(dirty_) {
  (void)writeStatFile(statsFile_, stats_);
 }
}
std::string PlayerStats::usernameKey() const {
 return session_.username.empty() ? "local" : toLower(session_.username);
}
void PlayerStats::increment(const StatId& stat, int amount) {
 incrementById(stat.id, amount);
}
void PlayerStats::incrementById(int statId, int amount) {
 doIncrement(stats_, statId, amount);
 dirty_ = true;
}
void PlayerStats::increment(const std::unordered_map<int, int>& amounts) {
 dirty_ = true;
 for(const auto& [statId, amount] : amounts) {
  doIncrement(stats_, statId, amount);
 }
}
void PlayerStats::tick(const std::unordered_map<int, int>& remoteStats) {
 for(const auto& [statId, amount] : remoteStats) {
  stats_[statId] = amount;
 }
}
void PlayerStats::tickUnsent(const std::unordered_map<int, int>& amounts) {
 increment(amounts);
}
bool PlayerStats::hasParentAchievement(int statId) const {
 const achievement::AchievementDef* achievement = achievement::Achievements::getByStatId(statId);
 if(achievement == nullptr) {
  return true;
 }
 if(achievement->parentIndex < 0) {
  return true;
 }
 return hasStat(achievement->parentStatId());
}
void PlayerStats::save() {
 if(writeStatFile(statsFile_, stats_)) {
  dirty_ = false;
 }
}
void PlayerStats::tick() {
 if(saveCooldown_ > 0) {
  --saveCooldown_;
 }
 if(dirty_ && saveCooldown_ <= 0) {
  save();
  saveCooldown_ = 100;
 }
}
void PlayerStats::loadFromDisk() {
 if(readStatFile(statsFile_, stats_)) {
  return;
 }
 if(!std::filesystem::exists(statsFile_)) {
  return;
 }
 std::ifstream input(statsFile_, std::ios::binary);
 if(!input) {
  return;
 }
 std::ostringstream buffer;
 buffer << input.rdbuf();
 const std::string data = buffer.str();
 if(data.empty() || data[0] != '{') {
  return;
 }
 stats_ = importLegacyJson(data);
 if(!stats_.empty()) {
  dirty_ = true;
 }
}
std::unordered_map<int, int> PlayerStats::importLegacyJson(const std::string& data) {
 std::unordered_map<int, int> result;
 const std::size_t arrayPos = data.find("\"stats-change\"");
 if(arrayPos == std::string::npos) {
  return {};
 }
 std::size_t cursor = data.find('[', arrayPos);
 if(cursor == std::string::npos) {
  return {};
 }
 ++cursor;
 while(cursor < data.size()) {
  cursor = data.find('{', cursor);
  if(cursor == std::string::npos) {
   break;
  }
  const std::size_t end = data.find('}', cursor);
  if(end == std::string::npos) {
   break;
  }
  const std::string objectText = data.substr(cursor + 1, end - cursor - 1);
  cursor = end + 1;
  std::size_t quote = objectText.find('"');
  if(quote == std::string::npos) {
   continue;
  }
  ++quote;
  const std::size_t quoteEnd = objectText.find('"', quote);
  if(quoteEnd == std::string::npos) {
   continue;
  }
  const int statId = std::stoi(objectText.substr(quote, quoteEnd - quote));
  int amount = 0;
  if(!parseIntField(objectText.substr(quoteEnd + 1), amount)) {
   continue;
  }
  if(Stats::getStatById(statId) == nullptr) {
   continue;
  }
  result[statId] = amount;
 }
 return result;
}
void PlayerStats::doIncrement(std::unordered_map<int, int>& target, int statId, int amount) {
 target[statId] += amount;
}
int PlayerStats::getFrom(const std::unordered_map<int, int>& source, int statId) {
 auto it = source.find(statId);
 return it != source.end() ? it->second : 0;
}
void PlayerStats::relocateLegacyStats() const {
 if(!std::filesystem::exists(runDirectory_)) {
  return;
 }
 for(const auto& entry : std::filesystem::directory_iterator(runDirectory_)) {
  if(!entry.is_regular_file()) {
   continue;
  }
  const std::string name = entry.path().filename().string();
  if(!name.starts_with("stats_") || entry.path().extension() != ".dat") {
   continue;
  }
  const std::filesystem::path destination = statsDirectory_ / name;
  if(!std::filesystem::exists(destination)) {
   std::filesystem::rename(entry.path(), destination);
  }
 }
}
} // namespace net::minecraft::stat
