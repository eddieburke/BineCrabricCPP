#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace net::minecraft::client::render {
using CodeMask = std::vector<unsigned char>;
struct TokenReplacement {
 std::string from;
 std::string to;
};
CodeMask codeMask(const std::string& source);
bool tokenAt(const std::string& source,
             const CodeMask& mask,
             std::size_t at,
             std::string_view token);
void replaceAllToken(std::string& source, std::string_view from, std::string_view to);
void replaceAllTokens(std::string& source, const std::vector<TokenReplacement>& replacements);
void replaceGlobalStorageQualifier(std::string& source,
                                   std::string_view from,
                                   std::string_view to);
bool referencesToken(const std::string& source, std::string_view token);
bool referencesToken(const std::string& source, const CodeMask& mask, std::string_view token);
bool hasStorageDeclaration(const std::string& source,
                           std::string_view storage,
                           std::string_view name);
bool hasStorageDeclaration(const std::string& source,
                           const CodeMask& mask,
                           std::string_view storage,
                           std::string_view name);
std::size_t sourceDeclarationOffset(const std::string& source);
bool appendBeforeMainClose(std::string& source, const std::string& snippet);
bool prependToMainBody(std::string& source, const std::string& snippet);
}
