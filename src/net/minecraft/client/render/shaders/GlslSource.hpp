#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace net::minecraft::client::render {
std::vector<bool> codeMask(const std::string& source);
bool tokenAt(const std::string& source,
             const std::vector<bool>& mask,
             std::size_t at,
             std::string_view token);
void replaceAllToken(std::string& source, std::string_view from, std::string_view to);
void replaceGlobalStorageQualifier(std::string& source,
                                   std::string_view from,
                                   std::string_view to);
bool referencesToken(const std::string& source, std::string_view token);
bool hasStorageDeclaration(const std::string& source,
                           std::string_view storage,
                           std::string_view name);
std::size_t sourceDeclarationOffset(const std::string& source);
bool appendBeforeMainClose(std::string& source, const std::string& snippet);
bool prependToMainBody(std::string& source, const std::string& snippet);
}
