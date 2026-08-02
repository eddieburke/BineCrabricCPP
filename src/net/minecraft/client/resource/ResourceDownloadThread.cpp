#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include <cstdint>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
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
ResourceDownloadThread::ResourceDownloadThread(std::filesystem::path resourcesRoot, Minecraft* minecraft)
    : resourcesDirectory(std::move(resourcesRoot)), minecraft_(minecraft), state_(std::make_shared<State>(resourcesDirectory)) {
 if(!std::filesystem::exists(resourcesDirectory) && !std::filesystem::create_directories(resourcesDirectory)) {
  throw std::runtime_error("The working directory could not be created: " + resourcesDirectory.string());
 }
}
ResourceDownloadThread::~ResourceDownloadThread() {
 cancel();
}
void ResourceDownloadThread::start() {
 if(state_->started.exchange(true)) {
  return;
 }
 const auto state = state_;
 net::minecraft::util::concurrent::ThreadCoordinator::instance()
     .pool(net::minecraft::util::concurrent::Domain::Io)
     .submit([state]() { run(state); });
}
void ResourceDownloadThread::cancel() {
 state_->cancelled.store(true);
 state_->completed.request_stop();
}
void ResourceDownloadThread::reload() {
 loadFromDirectory(state_, resourcesDirectory, "");
}
void ResourceDownloadThread::tick() {
 if(minecraft_ == nullptr) {
  return;
 }
 PendingResource resource;
 while(state_->completed.tryPop(resource)) {
  try {
   minecraft_->loadResource(resource.path, resource.file);
  } catch(const std::exception&) {
  }
 }
}
void ResourceDownloadThread::run(const std::shared_ptr<State>& state) {
 try {
  const HttpResponse listing = fetchUrl(kResourceBaseUrl, true);
  if(!listing.ok()) {
   throw std::runtime_error("resource listing failed (HTTP " + std::to_string(listing.statusCode) +
                            "; betacraft proxy required for s3.amazonaws.com/MinecraftResources)");
  }
  const std::string xml = listing.bodyAsString();
  for(int pass = 0; pass < 2; ++pass) {
   parseResourceListing(xml, [state, pass](const std::string& path, long long size) {
    if(size <= 0 || state->cancelled.load()) {
     return;
    }
    loadFromUrl(state, path, size, pass);
   });
   if(state->cancelled.load()) {
    return;
   }
  }
 } catch(const std::exception& ex) {
  (void)ex;
  loadFromDirectory(state, state->resourcesDirectory, "");
 }
}
void ResourceDownloadThread::loadFromDirectory(const std::shared_ptr<State>& state,
                                               const std::filesystem::path& directory,
                                               const std::string& type) {
 if(state->cancelled.load() || !std::filesystem::exists(directory)) {
  return;
 }
 for(const auto& entry : std::filesystem::directory_iterator(directory)) {
  if(state->cancelled.load()) {
   return;
  }
  if(entry.is_directory()) {
   loadFromDirectory(state, entry.path(), type + entry.path().filename().string() + "/");
   continue;
  }
  if(!entry.is_regular_file()) {
   continue;
  }
  state->completed.push(PendingResource{type + entry.path().filename().string(), entry.path()});
 }
}
void ResourceDownloadThread::loadFromUrl(const std::shared_ptr<State>& state,
                                         const std::string& path,
                                         long long size,
                                         int type) {
 if(state->cancelled.load()) {
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
  const std::filesystem::path file = state->resourcesDirectory / path;
  if(!std::filesystem::exists(file) || static_cast<long long>(std::filesystem::file_size(file)) != size) {
   std::filesystem::create_directories(file.parent_path());
   const std::string url = kResourceBaseUrl + encodeUrlPath(path);
   if(!downloadFile(url, file)) {
    return;
   }
   if(state->cancelled.load()) {
    return;
   }
  }
  state->completed.push(PendingResource{path, file});
 } catch(const std::exception&) {
 }
}
} // namespace net::minecraft::client::resource
