#include "net/minecraft/util/http/HttpClient.hpp"
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif
#include <string>
#include <vector>
namespace net::minecraft::util::http {
namespace {
#ifdef _WIN32
std::wstring toWide(const std::string& text) {
 return std::wstring(text.begin(), text.end());
}
bool crackUrl(const std::string& url, std::wstring& host, std::wstring& path, INTERNET_PORT& port, bool& secure) {
 URL_COMPONENTS components{};
 components.dwStructSize = sizeof(components);
 const std::wstring wide = toWide(url);
 components.dwHostNameLength = static_cast<DWORD>(-1);
 components.dwUrlPathLength = static_cast<DWORD>(-1);
 if(!WinHttpCrackUrl(wide.c_str(), 0, 0, &components)) {
  return false;
 }
 host.assign(components.lpszHostName, components.dwHostNameLength);
 path.assign(components.lpszUrlPath, components.dwUrlPathLength);
 port = components.nPort;
 secure = components.nScheme == INTERNET_SCHEME_HTTPS;
 return true;
}
std::wstring buildHeaderBlock(const std::vector<HttpHeader>& headers) {
 std::wstring block;
 for(const HttpHeader& header : headers) {
  block += toWide(header.name);
  block += L": ";
  block += toWide(header.value);
  block += L"\r\n";
 }
 return block;
}
#endif
} // namespace
HttpResponse httpRequest(const HttpRequest& request) {
 HttpResponse response;
#ifndef _WIN32
 (void)request;
 return response;
#else
 const auto cancelled = [&request] {
  return request.cancelled != nullptr && request.cancelled->load(std::memory_order_acquire);
 };
 if(cancelled()) {
  return response;
 }
 std::wstring host;
 std::wstring path;
 INTERNET_PORT port = 0;
 bool secure = false;
 if(!crackUrl(request.url, host, path, port, secure)) {
  return response;
 }
 const wchar_t* proxyName = WINHTTP_NO_PROXY_NAME;
 std::wstring proxyStorage;
 if(request.useBetacraftProxy) {
  proxyStorage = toWide(kBetacraftProxyHost) + L":" + std::to_wstring(kBetacraftProxyPortBeta173);
  proxyName = proxyStorage.c_str();
 }
 HINTERNET session =
     WinHttpOpen(L"MinecraftNative/1.0",
                 request.useBetacraftProxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                 proxyName,
                 WINHTTP_NO_PROXY_BYPASS,
                 0);
 if(session == nullptr) {
  return response;
 }
 WinHttpSetTimeouts(
     session, request.connectTimeoutMs, request.connectTimeoutMs, request.sendTimeoutMs, request.receiveTimeoutMs);
 if(cancelled()) {
  WinHttpCloseHandle(session);
  return response;
 }
 HINTERNET connection = WinHttpConnect(session, host.c_str(), port, 0);
 if(connection == nullptr) {
  WinHttpCloseHandle(session);
  return response;
 }
 const DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
 const std::wstring method = toWide(request.method.empty() ? "GET" : request.method);
 HINTERNET httpHandle = WinHttpOpenRequest(
     connection, method.c_str(), path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
 if(httpHandle == nullptr) {
  WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);
  return response;
 }
 if(cancelled()) {
  WinHttpCloseHandle(httpHandle);
  WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);
  return response;
 }
 const std::wstring headerBlock = buildHeaderBlock(request.headers);
 const wchar_t* headerPtr = headerBlock.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headerBlock.c_str();
 const DWORD headerLength = headerBlock.empty() ? 0 : static_cast<DWORD>(-1);
 LPVOID bodyPtr = WINHTTP_NO_REQUEST_DATA;
 DWORD bodyLength = 0;
 if(!request.body.empty()) {
  bodyPtr = const_cast<char*>(request.body.data());
  bodyLength = static_cast<DWORD>(request.body.size());
 }
 if(WinHttpSendRequest(httpHandle, headerPtr, headerLength, bodyPtr, bodyLength, bodyLength, 0) && !cancelled() &&
    WinHttpReceiveResponse(httpHandle, nullptr) && !cancelled()) {
  DWORD statusCode = 0;
  DWORD statusSize = sizeof(statusCode);
  WinHttpQueryHeaders(httpHandle,
                      WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                      WINHTTP_HEADER_NAME_BY_INDEX,
                      &statusCode,
                      &statusSize,
                      WINHTTP_NO_HEADER_INDEX);
  response.statusCode = static_cast<int>(statusCode);
  for(;;) {
   DWORD available = 0;
   if(!WinHttpQueryDataAvailable(httpHandle, &available) || available == 0) {
    break;
   }
   if(cancelled() || response.body.size() + static_cast<std::size_t>(available) > request.maxResponseBytes) {
    response = {};
    break;
   }
   const std::size_t offset = response.body.size();
   response.body.resize(offset + available);
   DWORD read = 0;
   if(!WinHttpReadData(httpHandle, response.body.data() + offset, available, &read) || read == 0) {
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
HttpResponse fetchUrl(const std::string& url, bool useBetacraftProxy) {
 HttpRequest request;
 request.method = "GET";
 request.url = url;
 request.useBetacraftProxy = useBetacraftProxy;
 return httpRequest(request);
}
} // namespace net::minecraft::util::http
