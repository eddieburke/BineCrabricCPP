#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include <cstdint>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
namespace net::minecraft::client::resource {
namespace {
constexpr const char* kResourceBaseUrl = "http://s3.amazonaws.com/MinecraftResources/";
std::string extractTagValue(const std::string& xml, const std::string& tag, std::size_t& searchFrom) {
  const std::string open = "<" + tag + ">";
  const std::string close = "</" + tag + ">";
  const std::size_t start = xml.find(open, searchFrom);
  if(start == std::string::npos) {
    return {};
  }
  const std::size_t valueStart = start + open.size();
  const std::size_t end = xml.find(close, valueStart);
  if(end == std::string::npos) {
    return {};
  }
  searchFrom = end + close.size();
  return xml.substr(valueStart, end - valueStart);
}
void parseResourceListing(const std::string& xml, const std::function<void(const std::string&, long long)>& consumer) {
  std::size_t searchFrom = 0;
  while(searchFrom < xml.size()) {
    const std::size_t blockStart = xml.find("<Contents>", searchFrom);
    if(blockStart == std::string::npos) {
      break;
    }
    const std::size_t blockEnd = xml.find("</Contents>", blockStart);
    if(blockEnd == std::string::npos) {
      break;
    }
    const std::string block = xml.substr(blockStart, blockEnd - blockStart);
    std::size_t tagPos = 0;
    const std::string key = extractTagValue(block, "Key", tagPos);
    const std::string sizeText = extractTagValue(block, "Size", tagPos);
    if(!key.empty() && !sizeText.empty()) {
      consumer(key, std::stoll(sizeText));
    }
    searchFrom = blockEnd + 11;
  }
}
bool downloadFile(const std::string& url, const std::filesystem::path& destination) {
  const HttpResponse response = fetchUrl(url, true);
  if(!response.ok() || response.body.empty()) {
    return false;
  }
  std::ofstream out(destination, std::ios::binary);
  if(!out) {
    return false;
  }
  out.write(reinterpret_cast<const char*>(response.body.data()), static_cast<std::streamsize>(response.body.size()));
  return out.good();
}
std::string encodeUrlPath(std::string path) {
  std::string encoded;
  encoded.reserve(path.size());
  for(char ch : path) {
    if(ch == ' ') {
      encoded += "%20";
    } else {
      encoded += ch;
    }
  }
  return encoded;
}
} // namespace
ResourceDownloadThread::ResourceDownloadThread(std::filesystem::path runDirectory, Minecraft* minecraft)
    : resourcesDirectory(std::move(runDirectory) / "resources"), minecraft_(minecraft) {
  if(!std::filesystem::exists(resourcesDirectory) && !std::filesystem::create_directories(resourcesDirectory)) {
    throw std::runtime_error("The working directory could not be created: " + resourcesDirectory.string());
  }
}
ResourceDownloadThread::~ResourceDownloadThread() {
  cancel();
  if(worker_.joinable()) {
    worker_.join();
  }
}
void ResourceDownloadThread::start() {
  if(started_.exchange(true)) {
    return;
  }
  worker_ = std::thread([this]() { run(); });
}
void ResourceDownloadThread::cancel() {
  cancelled_.store(true);
}
void ResourceDownloadThread::reload() {
  loadFromDirectory(resourcesDirectory, "");
}
void ResourceDownloadThread::run() {
  try {
    const HttpResponse listing = fetchUrl(kResourceBaseUrl, true);
    if(!listing.ok()) {
      throw std::runtime_error("resource listing failed (HTTP " + std::to_string(listing.statusCode) +
                               "; betacraft proxy required for s3.amazonaws.com/MinecraftResources)");
    }
    const std::string xml = listing.bodyAsString();
    for(int pass = 0; pass < 2; ++pass) {
      parseResourceListing(xml, [this, pass](const std::string& path, long long size) {
        if(size <= 0 || cancelled_.load()) {
          return;
        }
        loadFromUrl(path, size, pass);
      });
      if(cancelled_.load()) {
        return;
      }
    }
  } catch(const std::exception&) {
    loadFromDirectory(resourcesDirectory, "");
  }
}
void ResourceDownloadThread::loadFromDirectory(const std::filesystem::path& directory, const std::string& type) {
  if(minecraft_ == nullptr || cancelled_.load() || !std::filesystem::exists(directory)) {
    return;
  }
  for(const auto& entry : std::filesystem::directory_iterator(directory)) {
    if(cancelled_.load()) {
      return;
    }
    if(entry.is_directory()) {
      loadFromDirectory(entry.path(), type + entry.path().filename().string() + "/");
      continue;
    }
    if(!entry.is_regular_file()) {
      continue;
    }
    const std::string resourcePath = type + entry.path().filename().string();
    try {
      minecraft_->loadResource(resourcePath, entry.path());
    } catch(const std::exception&) {
    }
  }
}
void ResourceDownloadThread::loadFromUrl(const std::string& path, long long size, int type) {
  if(minecraft_ == nullptr || cancelled_.load()) {
    return;
  }
  try {
    const std::size_t slash = path.find('/');
    if(slash == std::string::npos) {
      return;
    }
    const std::string prefix = path.substr(0, slash);
    const bool isSound = prefix == "sound" || prefix == "newsound";
    if(isSound ? type != 0 : type != 1) {
      return;
    }
    const std::filesystem::path file = resourcesDirectory / path;
    if(!std::filesystem::exists(file) || static_cast<long long>(std::filesystem::file_size(file)) != size) {
      std::filesystem::create_directories(file.parent_path());
      const std::string url = kResourceBaseUrl + encodeUrlPath(path);
      if(!downloadFile(url, file)) {
        return;
      }
      if(cancelled_.load()) {
        return;
      }
    }
    minecraft_->loadResource(path, file);
  } catch(const std::exception&) {
  }
}
} // namespace net::minecraft::client::resource
