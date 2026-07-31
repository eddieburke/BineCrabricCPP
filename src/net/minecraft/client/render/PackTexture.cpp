#include "net/minecraft/client/render/shaderpack/ShaderTexture.hpp"
#include <cstring>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <gdiplus.h>
#endif
namespace net::minecraft::client::render::shaderpack {
DecodedTexture decodeTexture(const std::string& bytes) {
 DecodedTexture output;
#ifdef _WIN32
 static ULONG_PTR token = 0;
 static bool ready = [] {
  Gdiplus::GdiplusStartupInput input;
  return Gdiplus::GdiplusStartup(&token, &input, nullptr) == Gdiplus::Ok;
 }();
 if(!ready || bytes.empty()) return output;
 HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes.size());
 if(memory == nullptr) return output;
 void* destination = GlobalLock(memory);
 std::memcpy(destination, bytes.data(), bytes.size());
 GlobalUnlock(memory);
 IStream* stream = nullptr;
 if(CreateStreamOnHGlobal(memory, TRUE, &stream) != S_OK) {
  GlobalFree(memory);
  return output;
 }
 Gdiplus::Bitmap bitmap(stream);
 if(bitmap.GetLastStatus() == Gdiplus::Ok) {
  output.width = static_cast<int>(bitmap.GetWidth());
  output.height = static_cast<int>(bitmap.GetHeight());
  output.rgba.resize(static_cast<std::size_t>(output.width) * output.height * 4);
  Gdiplus::Rect rect(0, 0, output.width, output.height);
  Gdiplus::BitmapData data{};
  if(bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) == Gdiplus::Ok) {
   for(int y = 0; y < output.height; ++y) {
    const auto* source = static_cast<const std::uint8_t*>(data.Scan0) + static_cast<std::size_t>(y) * data.Stride;
    auto* target = output.rgba.data() + static_cast<std::size_t>(y) * output.width * 4;
    for(int x = 0; x < output.width; ++x) {
     target[x * 4] = source[x * 4 + 2];
     target[x * 4 + 1] = source[x * 4 + 1];
     target[x * 4 + 2] = source[x * 4];
     target[x * 4 + 3] = source[x * 4 + 3];
    }
   }
   bitmap.UnlockBits(&data);
  } else {
   output = {};
  }
 }
 stream->Release();
#endif
 return output;
}
} // namespace net::minecraft::client::render::shaderpack
