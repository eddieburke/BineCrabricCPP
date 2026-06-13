#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"

#include "net/minecraft/client/Minecraft.hpp"

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif

#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace net::minecraft::client::resource {
namespace {

constexpr const char* kResourceBaseUrl = "http://s3.amazonaws.com/MinecraftResources/";

#ifdef _WIN32
std::wstring toWide(const std::string& text)
{
    return std::wstring(text.begin(), text.end());
}

bool crackUrl(const std::string& url, std::wstring& host, std::wstring& path, INTERNET_PORT& port, bool& secure)
{
    URL_COMPONENTS components {};
    components.dwStructSize = sizeof(components);
    const std::wstring wide = toWide(url);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wide.c_str(), 0, 0, &components)) {
        return false;
    }
    host.assign(components.lpszHostName, components.dwHostNameLength);
    path.assign(components.lpszUrlPath, components.dwUrlPathLength);
    port = components.nPort;
    secure = components.nScheme == INTERNET_SCHEME_HTTPS;
    return true;
}
#endif

std::string extractTagValue(const std::string& xml, const std::string& tag, std::size_t& searchFrom)
{
    const std::string open = "<" + tag + ">";
    const std::string close = "</" + tag + ">";
    const std::size_t start = xml.find(open, searchFrom);
    if (start == std::string::npos) {
        return {};
    }
    const std::size_t valueStart = start + open.size();
    const std::size_t end = xml.find(close, valueStart);
    if (end == std::string::npos) {
        return {};
    }
    searchFrom = end + close.size();
    return xml.substr(valueStart, end - valueStart);
}

void parseResourceListing(const std::string& xml, const std::function<void(const std::string&, long long)>& consumer)
{
    std::size_t searchFrom = 0;
    while (searchFrom < xml.size()) {
        const std::size_t blockStart = xml.find("<Contents>", searchFrom);
        if (blockStart == std::string::npos) {
            break;
        }
        const std::size_t blockEnd = xml.find("</Contents>", blockStart);
        if (blockEnd == std::string::npos) {
            break;
        }
        const std::string block = xml.substr(blockStart, blockEnd - blockStart);
        std::size_t tagPos = 0;
        const std::string key = extractTagValue(block, "Key", tagPos);
        const std::string sizeText = extractTagValue(block, "Size", tagPos);
        if (!key.empty() && !sizeText.empty()) {
            consumer(key, std::stoll(sizeText));
        }
        searchFrom = blockEnd + 11;
    }
}

bool downloadFile(const std::string& url, const std::filesystem::path& destination)
{
    const HttpResponse response = fetchUrl(url, true);
    if (!response.ok() || response.body.empty()) {
        return false;
    }
    std::ofstream out(destination, std::ios::binary);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char*>(response.body.data()),
        static_cast<std::streamsize>(response.body.size()));
    return out.good();
}

std::string encodeUrlPath(std::string path)
{
    std::string encoded;
    encoded.reserve(path.size());
    for (char ch : path) {
        if (ch == ' ') {
            encoded += "%20";
        } else {
            encoded += ch;
        }
    }
    return encoded;
}

#ifdef _WIN32
std::wstring buildHeaderBlock(const std::vector<HttpHeader>& headers)
{
    std::wstring block;
    for (const HttpHeader& header : headers) {
        block += toWide(header.name);
        block += L": ";
        block += toWide(header.value);
        block += L"\r\n";
    }
    return block;
}
#endif

} // namespace

HttpResponse httpRequest(const HttpRequest& request)
{
    HttpResponse response;
#ifndef _WIN32
    (void)request;
    return response;
#else
    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = 0;
    bool secure = false;
    if (!crackUrl(request.url, host, path, port, secure)) {
        return response;
    }

    const wchar_t* proxyName = WINHTTP_NO_PROXY_NAME;
    std::wstring proxyStorage;
    if (request.useBetacraftProxy) {
        proxyStorage = toWide(kBetacraftProxyHost) + L":" + std::to_wstring(kBetacraftProxyPortBeta173);
        proxyName = proxyStorage.c_str();
    }

    HINTERNET session = WinHttpOpen(L"MinecraftNative/1.0",
        request.useBetacraftProxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, proxyName,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == nullptr) {
        return response;
    }

    HINTERNET connection = WinHttpConnect(session, host.c_str(), port, 0);
    if (connection == nullptr) {
        WinHttpCloseHandle(session);
        return response;
    }

    const DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
    const std::wstring method = toWide(request.method.empty() ? "GET" : request.method);
    HINTERNET httpHandle = WinHttpOpenRequest(connection, method.c_str(), path.c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (httpHandle == nullptr) {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return response;
    }

    const std::wstring headerBlock = buildHeaderBlock(request.headers);
    const wchar_t* headerPtr = headerBlock.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headerBlock.c_str();
    const DWORD headerLength = headerBlock.empty() ? 0 : static_cast<DWORD>(-1);
    LPVOID bodyPtr = WINHTTP_NO_REQUEST_DATA;
    DWORD bodyLength = 0;
    if (!request.body.empty()) {
        bodyPtr = const_cast<char*>(request.body.data());
        bodyLength = static_cast<DWORD>(request.body.size());
    }

    if (WinHttpSendRequest(httpHandle, headerPtr, headerLength, bodyPtr, bodyLength, bodyLength, 0)
        && WinHttpReceiveResponse(httpHandle, nullptr)) {
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        WinHttpQueryHeaders(httpHandle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
        response.statusCode = static_cast<int>(statusCode);
        for (;;) {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(httpHandle, &available) || available == 0) {
                break;
            }
            const std::size_t offset = response.body.size();
            response.body.resize(offset + available);
            DWORD read = 0;
            if (!WinHttpReadData(httpHandle, response.body.data() + offset, available, &read) || read == 0) {
                break;
            }
            response.body.resize(offset + read);
        }
    }

    WinHttpCloseHandle(httpHandle);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return response;
#endif
}

HttpResponse fetchUrl(const std::string& url, bool useBetacraftProxy)
{
    HttpRequest request;
    request.method = "GET";
    request.url = url;
    request.useBetacraftProxy = useBetacraftProxy;
    return httpRequest(request);
}

ResourceDownloadThread::ResourceDownloadThread(std::filesystem::path runDirectory, Minecraft* minecraft)
    : resourcesDirectory(std::move(runDirectory) / "resources"),
      minecraft_(minecraft)
{
    if (!std::filesystem::exists(resourcesDirectory) && !std::filesystem::create_directories(resourcesDirectory)) {
        throw std::runtime_error("The working directory could not be created: " + resourcesDirectory.string());
    }
}

ResourceDownloadThread::~ResourceDownloadThread()
{
    cancel();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void ResourceDownloadThread::start()
{
    if (started_.exchange(true)) {
        return;
    }
    worker_ = std::thread([this]() { run(); });
}

void ResourceDownloadThread::cancel()
{
    cancelled_.store(true);
}

void ResourceDownloadThread::reload()
{
    loadFromDirectory(resourcesDirectory, "");
}

void ResourceDownloadThread::run()
{
    try {
        const HttpResponse listing = fetchUrl(kResourceBaseUrl, true);
        if (!listing.ok()) {
            throw std::runtime_error(
                "resource listing failed (HTTP " + std::to_string(listing.statusCode)
                + "; betacraft proxy required for s3.amazonaws.com/MinecraftResources)");
        }
        const std::string xml = listing.bodyAsString();
        for (int pass = 0; pass < 2; ++pass) {
            parseResourceListing(xml, [this, pass](const std::string& path, long long size) {
                if (size <= 0 || cancelled_.load()) {
                    return;
                }
                loadFromUrl(path, size, pass);
            });
            if (cancelled_.load()) {
                return;
            }
        }
    } catch (const std::exception& exception) {
        loadFromDirectory(resourcesDirectory, "");
        std::cerr << exception.what() << std::endl;
    }
}

void ResourceDownloadThread::loadFromDirectory(const std::filesystem::path& directory, const std::string& type)
{
    if (minecraft_ == nullptr || cancelled_.load() || !std::filesystem::exists(directory)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (cancelled_.load()) {
            return;
        }
        if (entry.is_directory()) {
            loadFromDirectory(entry.path(), type + entry.path().filename().string() + "/");
            continue;
        }
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string resourcePath = type + entry.path().filename().string();
        try {
            minecraft_->loadResource(resourcePath, entry.path());
        } catch (const std::exception&) {
            std::cout << "Failed to add " << resourcePath << '\n';
        }
    }
}

void ResourceDownloadThread::loadFromUrl(const std::string& path, long long size, int type)
{
    if (minecraft_ == nullptr || cancelled_.load()) {
        return;
    }
    try {
        const std::size_t slash = path.find('/');
        if (slash == std::string::npos) {
            return;
        }
        const std::string prefix = path.substr(0, slash);
        const bool isSound = prefix == "sound" || prefix == "newsound";
        if (isSound ? type != 0 : type != 1) {
            return;
        }
        const std::filesystem::path file = resourcesDirectory / path;
        if (!std::filesystem::exists(file) || static_cast<long long>(std::filesystem::file_size(file)) != size) {
            std::filesystem::create_directories(file.parent_path());
            const std::string url = kResourceBaseUrl + encodeUrlPath(path);
            if (!downloadFile(url, file)) {
                std::cerr << "ResourceDownloadThread: failed to download " << path << '\n';
                return;
            }
            if (cancelled_.load()) {
                return;
            }
        }
        minecraft_->loadResource(path, file);
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << std::endl;
    }
}

} // namespace net::minecraft::client::resource
