#include "net/minecraft/client/render/shaders/GlslSource.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include "net/minecraft/client/render/shaders/PreProcessor.hpp"

namespace net::minecraft::client::render {
std::size_t sourceDeclarationOffset(const std::string& source) {
 std::size_t offset = 0;
 std::istringstream stream(source);
 bool inBlockComment = false;
 for(std::string line; std::getline(stream, line);) {
  bool hasCode = false;
  std::size_t index = 0;
  while(index < line.size()) {
   if(line[index] == '/' && index + 1 < line.size() && line[index + 1] == '/') {
    break;
   }
   if(line[index] == '/' && index + 1 < line.size() && line[index + 1] == '*') {
    const std::size_t close = line.find("*/", index + 2);
    if(close == std::string::npos) {
     inBlockComment = true;
     break;
    }
    index = close + 2;
    continue;
   }
   if(inBlockComment) {
    const std::size_t close = line.find("*/", index);
    if(close == std::string::npos) break;
    inBlockComment = false;
    index = close + 2;
    continue;
   }
   if(line[index] != ' ' && line[index] != '\t' && line[index] != '\r') {
    hasCode = true;
    break;
   }
   ++index;
  }
  if(hasCode) {
   const std::size_t first = line.find_first_not_of(" \t");
   if(first == std::string::npos || line[first] != '#') break;
  }
  offset += line.size() + 1;
 }
 return offset;
}

std::vector<bool> codeMask(const std::string& source) {
 std::vector<bool> mask(source.size(), true);
 bool lineComment = false;
 bool blockComment = false;
 bool quoted = false;
 char quote = '\0';
 for(std::size_t index = 0; index < source.size(); ++index) {
  const char ch = source[index];
  const char next = index + 1 < source.size() ? source[index + 1] : '\0';
  if(lineComment) {
   mask[index] = false;
   if(ch == '\n') lineComment = false;
   continue;
  }
  if(blockComment) {
   mask[index] = false;
   if(ch == '*' && next == '/') {
    mask[index + 1] = false;
    blockComment = false;
    ++index;
   }
   continue;
  }
  if(quoted) {
   mask[index] = false;
   if(ch == '\\' && next != '\0') {
    mask[index + 1] = false;
    ++index;
   } else if(ch == quote) {
    quoted = false;
   }
   continue;
  }
  if(ch == '/' && next == '/') {
   mask[index] = false;
   mask[index + 1] = false;
   lineComment = true;
   ++index;
  } else if(ch == '/' && next == '*') {
   mask[index] = false;
   mask[index + 1] = false;
   blockComment = true;
   ++index;
  } else if(ch == '"' || ch == '\'') {
   mask[index] = false;
   quoted = true;
   quote = ch;
  }
 }
 return mask;
}

bool tokenAt(const std::string& source,
             const std::vector<bool>& mask,
             std::size_t at,
             std::string_view token) {
 const std::size_t end = at + token.size();
 const bool left = at == 0 || !isIdentChar(source[at - 1]);
 const bool right = end >= source.size() || !isIdentChar(source[end]);
 return left && right && end <= mask.size() &&
        std::all_of(mask.begin() + static_cast<std::ptrdiff_t>(at),
                    mask.begin() + static_cast<std::ptrdiff_t>(end),
                    [](bool value) { return value; });
}

void replaceAllToken(std::string& source, std::string_view from, std::string_view to) {
 if(from.empty()) return;
 const std::vector<bool> mask = codeMask(source);
 std::vector<std::size_t> matches;
 std::size_t at = 0;
 while((at = source.find(from, at)) != std::string::npos) {
  if(tokenAt(source, mask, at, from)) matches.push_back(at);
  at += from.size();
 }
 for(auto it = matches.rbegin(); it != matches.rend(); ++it) source.replace(*it, from.size(), to);
}

void replaceGlobalStorageQualifier(std::string& source,
                                   std::string_view from,
                                   std::string_view to) {
 if(from.empty()) return;
 const std::vector<bool> mask = codeMask(source);
 std::vector<std::size_t> matches;
 int braceDepth = 0;
 for(std::size_t index = 0; index < source.size(); ++index) {
  if(!mask[index]) continue;
  if(source[index] == '{') {
   ++braceDepth;
   continue;
  }
  if(source[index] == '}') {
   braceDepth = std::max(0, braceDepth - 1);
   continue;
  }
  if(braceDepth != 0 || source.compare(index, from.size(), from) != 0 ||
     !tokenAt(source, mask, index, from)) continue;
  const std::size_t lineStartAt = source.rfind('\n', index);
  std::size_t first = lineStartAt == std::string::npos ? 0 : lineStartAt + 1;
  while(first < index && std::isspace(static_cast<unsigned char>(source[first]))) ++first;
  if(first == index) matches.push_back(index);
  index += from.size() - 1;
 }
 for(auto it = matches.rbegin(); it != matches.rend(); ++it) source.replace(*it, from.size(), to);
}

bool referencesToken(const std::string& source, std::string_view token) {
 if(token.empty()) return false;
 const std::vector<bool> mask = codeMask(source);
 std::size_t at = 0;
 while((at = source.find(token, at)) != std::string::npos) {
  if(tokenAt(source, mask, at, token)) return true;
  at += token.size();
 }
 return false;
}

bool hasStorageDeclaration(const std::string& source,
                           std::string_view storage,
                           std::string_view name) {
 const std::vector<bool> mask = codeMask(source);
 int braceDepth = 0;
 int parenDepth = 0;
 bool declaration = false;
 for(std::size_t index = 0; index < source.size();) {
  if(!mask[index]) {
   ++index;
   continue;
  }
  const char ch = source[index];
  if(isIdentStart(ch)) {
   const std::size_t start = index++;
   while(index < source.size() && mask[index] && isIdentChar(source[index])) ++index;
   const std::string_view token(source.data() + start, index - start);
   if(!declaration && braceDepth == 0 && parenDepth == 0 && token == storage) {
    declaration = true;
   } else if(declaration && braceDepth == 0 && parenDepth == 0 && token == name) {
    return true;
   }
   continue;
  }
  if(ch == '{') ++braceDepth;
  else if(ch == '}') braceDepth = std::max(0, braceDepth - 1);
  else if(ch == '(') ++parenDepth;
  else if(ch == ')') parenDepth = std::max(0, parenDepth - 1);
  else if(ch == ';' && braceDepth == 0 && parenDepth == 0) declaration = false;
  ++index;
 }
 return false;
}

namespace {
std::size_t mainOpenBrace(const std::string& source, const std::vector<bool>& mask) {
 std::size_t mainId = 0;
 while((mainId = source.find("main", mainId)) != std::string::npos) {
  if(!tokenAt(source, mask, mainId, "main")) {
   mainId += 4;
   continue;
  }
  std::size_t cursor = mainId + 4;
  while(cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor]))) ++cursor;
  if(cursor >= source.size() || !mask[cursor] || source[cursor] != '(') {
   mainId += 4;
   continue;
  }
  int parenDepth = 1;
  for(++cursor; cursor < source.size() && parenDepth > 0; ++cursor) {
   if(!mask[cursor]) continue;
   if(source[cursor] == '(') ++parenDepth;
   else if(source[cursor] == ')') --parenDepth;
  }
  if(parenDepth != 0) return std::string::npos;
  while(cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor]))) ++cursor;
  if(cursor < source.size() && mask[cursor] && source[cursor] == '{') return cursor;
  mainId += 4;
 }
 return std::string::npos;
}
}

bool appendBeforeMainClose(std::string& source, const std::string& snippet) {
 const std::vector<bool> mask = codeMask(source);
 const std::size_t openBrace = mainOpenBrace(source, mask);
 if(openBrace == std::string::npos) return false;
 int depth = 1;
 for(std::size_t index = openBrace + 1; index < source.size(); ++index) {
  if(!mask[index]) continue;
  if(source[index] == '{') ++depth;
  else if(source[index] == '}' && --depth == 0) {
   source.insert(index, snippet);
   return true;
  }
 }
 return false;
}

bool prependToMainBody(std::string& source, const std::string& snippet) {
 const std::vector<bool> mask = codeMask(source);
 const std::size_t openBrace = mainOpenBrace(source, mask);
 if(openBrace == std::string::npos) return false;
 source.insert(openBrace + 1, snippet);
 return true;
}
}
