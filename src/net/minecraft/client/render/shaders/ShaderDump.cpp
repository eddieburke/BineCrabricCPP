#include "net/minecraft/client/render/shaders/ShaderDump.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
namespace net::minecraft::client::render {
namespace {
constexpr const char* kDumpEnvVar = "BINECRABRIC_SHADER_DUMPS";
bool dumpsEnabled() {
 const char* flag = std::getenv(kDumpEnvVar);
 return flag != nullptr && std::string_view(flag) != "" && std::string_view(flag) != "0";
}
} // namespace
std::string dumpFailedProgram(const std::string& programName, const std::string& preamble,
                              const std::vector<ShaderDumpStage>& stages, const std::string& error) {
 if(!dumpsEnabled()) {
  return std::string(" [set ") + kDumpEnvVar + "=1 to dump the patched source]";
 }
 std::error_code ec;
 const std::filesystem::path dir =
     net::minecraft::client::util::MinecraftDirectories::getRunDirectory() / "shader-dumps";
 std::filesystem::create_directories(dir, ec);
 if(ec) {
  return " [shader-dumps directory could not be created]";
 }
 std::string safeName = programName;
 for(char& c : safeName)
  if(c == '/' || c == '\\' || c == ':') c = '_';
 std::string written;
 for(const ShaderDumpStage& stage : stages) {
  if(stage.body == nullptr || stage.body->empty()) {
   continue;
  }
  std::ofstream out(dir / (safeName + stage.suffix), std::ios::binary | std::ios::trunc);
  if(!out) {
   continue;
  }
  // Source first and nothing before it: file line N is driver line N. The driver
  // log goes at the end, where it cannot shift the numbering it refers to.
  const std::string assembled = gl::ShaderProgram::assembleStageSource(preamble, *stage.body);
  out << assembled;
  if(!assembled.empty() && assembled.back() != '\n') out << '\n';
  out << "\n// === " << programName << stage.suffix << " -- driver log ===\n";
  std::istringstream errorLines(error);
  for(std::string line; std::getline(errorLines, line);) out << "//   " << line << '\n';
  if(!written.empty()) written += ", ";
  written += safeName + stage.suffix;
 }
 if(written.empty()) {
  return {};
 }
 return " [patched source dumped to shader-dumps/" + written + "]";
}
} // namespace net::minecraft::client::render
