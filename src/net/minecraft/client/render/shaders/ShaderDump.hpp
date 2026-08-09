#pragma once
#include <string>
#include <vector>
namespace net::minecraft::client::render {
// One stage of a failed program: the extension to write it under, and the patched
// body the shared preamble gets prepended to.
struct ShaderDumpStage {
 const char* suffix;
 const std::string* body;
};
// A driver reports "0(3872) : error ..." against the source it was handed, which is
// the preamble plus the fully-included, fully-patched stage body -- a string that
// exists nowhere on disk. Reconstructing it by hand means re-deriving the pack's
// macro state, so dumping it is worth having; writing it on every broken pack a
// player loads is not. Off unless BINECRABRIC_SHADER_DUMPS is set to something
// other than empty or "0".
//
// Returns the clause to append to the compile-failure log line: where the dump
// landed, or how to switch dumping on. Stages with an empty body are skipped.
std::string dumpFailedProgram(const std::string& programName, const std::string& preamble,
                              const std::vector<ShaderDumpStage>& stages, const std::string& error);
} // namespace net::minecraft::client::render
