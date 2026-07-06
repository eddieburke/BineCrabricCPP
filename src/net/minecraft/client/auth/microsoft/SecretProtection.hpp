#pragma once
// Current-user secret protection for persisted Microsoft refresh credentials.
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#endif
namespace msauth::secret {
inline void wipeString(std::string& value) noexcept {
  if(value.empty()) {
    return;
  }
#ifdef _WIN32
  SecureZeroMemory(value.data(), value.size());
#else
  volatile char* data = value.data();
  for(std::size_t i = 0; i < value.size(); ++i) {
    data[i] = 0;
  }
#endif
  value.clear();
}
inline std::optional<std::string> protectForCurrentUser(std::string_view plaintext) {
#ifndef _WIN32
  (void)plaintext;
  return std::nullopt;
#else
  static unsigned char entropyBytes[] = "MinecraftNative::msauth::refresh-token";
  DATA_BLOB inputBlob{};
  inputBlob.pbData = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(plaintext.data()));
  inputBlob.cbData = static_cast<DWORD>(plaintext.size());
  DATA_BLOB entropyBlob{};
  entropyBlob.pbData = entropyBytes;
  entropyBlob.cbData = static_cast<DWORD>(sizeof(entropyBytes) - 1);
  DATA_BLOB protectedBlob{};
  if(!CryptProtectData(&inputBlob, L"Minecraft Native refresh token", &entropyBlob, nullptr, nullptr,
                       CRYPTPROTECT_UI_FORBIDDEN, &protectedBlob)) {
    return std::nullopt;
  }
  DWORD encodedLength = 0;
  if(!CryptBinaryToStringA(protectedBlob.pbData, protectedBlob.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                           nullptr, &encodedLength)) {
    SecureZeroMemory(protectedBlob.pbData, protectedBlob.cbData);
    LocalFree(protectedBlob.pbData);
    return std::nullopt;
  }
  std::string encoded(encodedLength, '\0');
  const bool encodedOk =
      CryptBinaryToStringA(protectedBlob.pbData, protectedBlob.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                           encoded.data(), &encodedLength);
  SecureZeroMemory(protectedBlob.pbData, protectedBlob.cbData);
  LocalFree(protectedBlob.pbData);
  if(!encodedOk) {
    wipeString(encoded);
    return std::nullopt;
  }
  if(!encoded.empty() && encoded.back() == '\0') {
    encoded.pop_back();
  }
  return encoded;
#endif
}
inline std::optional<std::string> unprotectForCurrentUser(std::string_view protectedText) {
#ifndef _WIN32
  (void)protectedText;
  return std::nullopt;
#else
  static unsigned char entropyBytes[] = "MinecraftNative::msauth::refresh-token";
  DWORD binaryLength = 0;
  if(!CryptStringToBinaryA(protectedText.data(), static_cast<DWORD>(protectedText.size()), CRYPT_STRING_BASE64,
                           nullptr, &binaryLength, nullptr, nullptr)) {
    return std::nullopt;
  }
  std::string protectedBytes(binaryLength, '\0');
  if(!CryptStringToBinaryA(protectedText.data(), static_cast<DWORD>(protectedText.size()), CRYPT_STRING_BASE64,
                           reinterpret_cast<BYTE*>(protectedBytes.data()), &binaryLength, nullptr, nullptr)) {
    wipeString(protectedBytes);
    return std::nullopt;
  }
  DATA_BLOB inputBlob{};
  inputBlob.pbData = reinterpret_cast<BYTE*>(protectedBytes.data());
  inputBlob.cbData = binaryLength;
  DATA_BLOB entropyBlob{};
  entropyBlob.pbData = entropyBytes;
  entropyBlob.cbData = static_cast<DWORD>(sizeof(entropyBytes) - 1);
  DATA_BLOB plainBlob{};
  const bool ok =
      CryptUnprotectData(&inputBlob, nullptr, &entropyBlob, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &plainBlob);
  wipeString(protectedBytes);
  if(!ok) {
    return std::nullopt;
  }
  std::string plaintext(reinterpret_cast<const char*>(plainBlob.pbData),
                        reinterpret_cast<const char*>(plainBlob.pbData) + plainBlob.cbData);
  SecureZeroMemory(plainBlob.pbData, plainBlob.cbData);
  LocalFree(plainBlob.pbData);
  return plaintext;
#endif
}
} // namespace msauth::secret
