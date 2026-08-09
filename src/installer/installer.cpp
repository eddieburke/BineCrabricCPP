#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <urlmon.h>
#include <string>
#include <vector>
#undef IDC_ARROW
#define IDC_ARROW MAKEINTRESOURCEW(32512)
static HWND pathBox, statusBox;
static std::wstring sourceDir, installDir, toolkitDir;
static std::wstring Join(const std::wstring& a, const std::wstring& b) { return a.empty() ? b : a + L"\\" + b; }
static bool Exists(const std::wstring& p) { return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES; }
static void Status(const std::wstring& s) {
 SetWindowTextW(statusBox, s.c_str());
 UpdateWindow(statusBox);
}
static std::wstring Env(const wchar_t* n) {
 DWORD z = GetEnvironmentVariableW(n, 0, 0);
 if(!z) return {};
 std::wstring v(z, L'\0');
 GetEnvironmentVariableW(n, v.data(), z);
 v.resize(wcslen(v.c_str()));
 return v;
}
static std::wstring RegRead(const wchar_t* n) {
 HKEY k;
 wchar_t b[4096];
 DWORD z = sizeof b, t = 0;
 if(RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\BineCrabricCPP", 0, KEY_READ, &k) != ERROR_SUCCESS) return {};
 auto r = RegQueryValueExW(k, n, 0, &t, (BYTE*)b, &z);
 RegCloseKey(k);
 return r == ERROR_SUCCESS && t == REG_SZ ? std::wstring(b) : std::wstring();
}
static void RegWrite(const wchar_t* n, const std::wstring& v) {
 HKEY k;
 if(RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\BineCrabricCPP", 0, 0, 0, KEY_WRITE, 0, &k, 0) == ERROR_SUCCESS) {
  RegSetValueExW(k, n, 0, REG_SZ, (BYTE*)v.c_str(), DWORD((v.size() + 1) * sizeof(wchar_t)));
  RegCloseKey(k);
 }
}
static std::wstring Search(const wchar_t* n) {
 wchar_t b[4096];
 DWORD z = SearchPathW(0, n, 0, 4096, b, 0);
 return z && z < 4096 ? std::wstring(b) : std::wstring();
}
static std::wstring Parent(std::wstring p) {
 auto x = p.find_last_of(L"\\/");
 return x == std::wstring::npos ? std::wstring() : p.substr(0, x);
}
static bool FullToolkit(const std::wstring& r) { return Exists(Join(r, L"bin\\cmake.exe")) && Exists(Join(r, L"bin\\ninja.exe")) && Exists(Join(r, L"bin\\g++.exe")) && Exists(Join(r, L"lib\\libz.a")) && Exists(Join(r, L"lib\\libogg.a")) && Exists(Join(r, L"lib\\libvorbis.a")) && Exists(Join(r, L"lib\\libvorbisfile.a")); }
static std::wstring DetectToolkit() {
 std::vector<std::wstring> roots = {RegRead(L"ToolkitPath"), Env(L"BINECRABRIC_TOOLCHAIN"), Env(L"MINECRAFT_TOOLCHAIN_ROOT"), Join(sourceDir, L"toolchain\\mingw64")};
 auto cxx = Env(L"CXX");
 if(!cxx.empty()) roots.push_back(Parent(Parent(cxx)));
 auto gcc = Search(L"g++.exe");
 if(!gcc.empty()) roots.push_back(Parent(Parent(gcc)));
 for(auto& r : roots)
  if(!r.empty() && FullToolkit(r)) {
   RegWrite(L"ToolkitPath", r);
   return r;
  }
 return {};
}
static std::wstring DetectCompiler() {
 auto gcc = Search(L"g++.exe");
 if(!gcc.empty()) return gcc;
 auto cl = Search(L"cl.exe");
 if(!cl.empty()) return cl;
 auto pf = Env(L"ProgramFiles(x86)");
 auto vswhere = Join(pf, L"Microsoft Visual Studio\\Installer\\vswhere.exe");
 if(Exists(vswhere)) return vswhere;
 return {};
}
static bool Run(std::wstring cmd) {
 STARTUPINFOW si{};
 si.cb = sizeof si;
 PROCESS_INFORMATION pi{};
 if(!CreateProcessW(0, cmd.data(), 0, 0, FALSE, CREATE_NO_WINDOW, 0, 0, &si, &pi)) return false;
 WaitForSingleObject(pi.hProcess, INFINITE);
 DWORD c = 1;
 GetExitCodeProcess(pi.hProcess, &c);
 CloseHandle(pi.hThread);
 CloseHandle(pi.hProcess);
 return c == 0;
}
static bool CopyTree(const std::wstring& a, const std::wstring& b) {
 if(!Exists(a)) return false;
 SHFILEOPSTRUCTW op{};
 std::wstring f = a + L"\\*\0\0", t = b + L"\0\0";
 CreateDirectoryW(b.c_str(), 0);
 op.wFunc = FO_COPY;
 op.pFrom = f.c_str();
 op.pTo = t.c_str();
 op.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_SILENT;
 return SHFileOperationW(&op) == 0;
}
static bool Download(const std::wstring& u, const std::wstring& p) {
 DeleteFileW(p.c_str());
 return URLDownloadToFileW(0, u.c_str(), p.c_str(), 0, 0) == S_OK;
}
static bool DownloadZip(const wchar_t* asset, const std::wstring& dest) {
 wchar_t t[MAX_PATH];
 GetTempPathW(MAX_PATH, t);
 auto z = Join(t, L"binecrabric-download.zip");
 auto u = L"https://github.com/eddieburke/BineCrabricCPP/releases/latest/download/" + std::wstring(asset) + L".zip";
 CreateDirectoryW(dest.c_str(), 0);
 return Download(u, z) && Run(L"tar.exe -xf \"" + z + L"\" -C \"" + dest + L"\"");
}
static void EnsureDir(const std::wstring& p) {
 if(p.empty() || Exists(p)) return;
 EnsureDir(Parent(p));
 CreateDirectoryW(p.c_str(), 0);
}
// Mirrors build-omega.ps1's toolchain/manifest.json: keep URLs/versions in sync with that file.
static const wchar_t* kWinlibsUrl = L"https://github.com/brechtsanders/winlibs_mingw/releases/download/15.2.0posix-14.0.0-ucrt-r7/winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64ucrt-14.0.0-r7.zip";
static const wchar_t* kZstdUrl = L"https://github.com/facebook/zstd/releases/download/v1.5.7/zstd-v1.5.7-win64.zip";
static const wchar_t* kZstdExeSubpath = L"zstd-v1.5.7-win64\\zstd.exe";
struct UcrtPkg { const wchar_t* name; const wchar_t* url; };
static const UcrtPkg kUcrtPackages[] = {
 {L"zlib", L"https://mirror.msys2.org/mingw/ucrt64/mingw-w64-ucrt-x86_64-zlib-1.3.2-2-any.pkg.tar.zst"},
 {L"libogg", L"https://mirror.msys2.org/mingw/ucrt64/mingw-w64-ucrt-x86_64-libogg-1.3.6-1-any.pkg.tar.zst"},
 {L"libvorbis", L"https://mirror.msys2.org/mingw/ucrt64/mingw-w64-ucrt-x86_64-libvorbis-1.3.7-2-any.pkg.tar.zst"},
};
static std::wstring FindMingwRoot(const std::wstring& staging) {
 auto direct = Join(staging, L"mingw64");
 if(Exists(Join(direct, L"bin\\g++.exe"))) return direct;
 WIN32_FIND_DATAW fd;
 HANDLE h = FindFirstFileW((staging + L"\\*").c_str(), &fd);
 if(h == INVALID_HANDLE_VALUE) return {};
 std::wstring found;
 do {
  if((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")) {
   auto cand = Join(staging, fd.cFileName);
   if(Exists(Join(cand, L"bin\\g++.exe"))) { found = cand; break; }
  }
 } while(FindNextFileW(h, &fd));
 FindClose(h);
 return found;
}
static bool EnsureWinlibsToolchain(const std::wstring& mingwRoot, const std::wstring& downloadsDir, const std::wstring& toolchainRoot) {
 EnsureDir(downloadsDir);
 auto ar = Join(downloadsDir, L"winlibs-gcc.zip");
 Status(L"Downloading GCC toolchain (this can take a few minutes)...");
 if(!Download(kWinlibsUrl, ar)) { Status(L"WinLibs GCC toolchain download failed."); return false; }
 auto staging = Join(toolchainRoot, L"_staging");
 EnsureDir(staging);
 Status(L"Extracting GCC toolchain...");
 if(!Run(L"tar.exe -xf \"" + ar + L"\" -C \"" + staging + L"\"")) { Status(L"Failed to extract WinLibs archive."); return false; }
 auto found = FindMingwRoot(staging);
 if(found.empty()) { Status(L"WinLibs archive did not contain mingw64/bin/g++.exe"); return false; }
 if(Exists(mingwRoot)) {
  SHFILEOPSTRUCTW op{};
  std::wstring f = mingwRoot + L'\0' + L'\0';
  op.wFunc = FO_DELETE;
  op.pFrom = f.c_str();
  op.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
  SHFileOperationW(&op);
 }
 if(!MoveFileW(found.c_str(), mingwRoot.c_str())) { Status(L"Could not move extracted toolchain into place."); return false; }
 return true;
}
static std::wstring EnsureZstdTool(const std::wstring& toolsDir, const std::wstring& downloadsDir) {
 auto ze = Join(toolsDir, L"zstd.exe");
 if(Exists(ze)) return ze;
 EnsureDir(toolsDir);
 EnsureDir(downloadsDir);
 auto ar = Join(downloadsDir, L"zstd-win64.zip");
 if(!Download(kZstdUrl, ar)) return {};
 auto extractDir = Join(toolsDir, L"zstd-extract");
 EnsureDir(extractDir);
 if(!Run(L"tar.exe -xf \"" + ar + L"\" -C \"" + extractDir + L"\"")) return {};
 auto se = Join(extractDir, kZstdExeSubpath);
 if(!Exists(se)) return {};
 CopyFileW(se.c_str(), ze.c_str(), FALSE);
 return Exists(ze) ? ze : std::wstring();
}
static bool MergeUcrt64(const std::wstring& stageDir, const std::wstring& mingwRoot) {
 auto ucrt = Join(stageDir, L"ucrt64");
 if(!Exists(ucrt)) return false;
 for(auto sub : {L"bin", L"include", L"lib", L"share"}) {
  auto from = Join(ucrt, sub);
  if(!Exists(from)) continue;
  auto to = Join(mingwRoot, sub);
  EnsureDir(to);
  CopyTree(from, to);
 }
 return true;
}
static bool EnsureUcrtDeps(const std::wstring& mingwRoot, const std::wstring& zstdExe, const std::wstring& downloadsDir) {
 for(auto& pkg : kUcrtPackages) {
  Status(L"Installing dependency: " + std::wstring(pkg.name));
  std::wstring url = pkg.url;
  auto fn = url.substr(url.find_last_of(L'/') + 1);
  auto archivePath = Join(downloadsDir, fn);
  if(!Download(url, archivePath)) { Status(L"Download failed for " + std::wstring(pkg.name)); return false; }
  auto tarPath = archivePath + L".tar";
  DeleteFileW(tarPath.c_str());
  if(!Run(L"\"" + zstdExe + L"\" -d \"" + archivePath + L"\" -o \"" + tarPath + L"\" -f")) { Status(L"zstd extraction failed for " + std::wstring(pkg.name)); return false; }
  auto stage = Join(downloadsDir, L"stage-" + std::wstring(pkg.name));
  EnsureDir(stage);
  if(!Run(L"tar.exe -xf \"" + tarPath + L"\" -C \"" + stage + L"\"")) { Status(L"tar extraction failed for " + std::wstring(pkg.name)); return false; }
  if(!MergeUcrt64(stage, mingwRoot)) { Status(L"MSYS2 package did not contain ucrt64/: " + std::wstring(pkg.name)); return false; }
 }
 return true;
}
static bool EnsureToolkit() {
 toolkitDir = DetectToolkit();
 if(!toolkitDir.empty()) {
  Status(L"Toolkit ready:\r\n" + toolkitDir);
  return true;
 }
 auto compiler = DetectCompiler();
 Status(compiler.empty() ? L"No compatible compiler found. Downloading complete toolkit..." : L"Compiler found, but required zlib/Ogg libraries are missing. Downloading complete toolkit...");
 wchar_t local[MAX_PATH];
 SHGetFolderPathW(0, CSIDL_LOCAL_APPDATA, 0, 0, local);
 auto toolchainRoot = Join(local, L"BineCrabricCPP\\toolchain");
 auto mingwRoot = Join(toolchainRoot, L"mingw64");
 auto downloadsDir = Join(toolchainRoot, L"_downloads");
 auto toolsDir = Join(toolchainRoot, L"tools");
 EnsureDir(toolchainRoot);
 if(!Exists(Join(mingwRoot, L"bin\\g++.exe")) && !EnsureWinlibsToolchain(mingwRoot, downloadsDir, toolchainRoot)) return false;
 auto zstdExe = EnsureZstdTool(toolsDir, downloadsDir);
 if(zstdExe.empty()) { Status(L"Could not obtain zstd tool for dependency extraction."); return false; }
 if(!EnsureUcrtDeps(mingwRoot, zstdExe, downloadsDir)) return false;
 if(!FullToolkit(mingwRoot)) {
  Status(L"Downloaded toolkit is incomplete: cmake, ninja, GCC, zlib and Ogg are required.");
  return false;
 }
 toolkitDir = mingwRoot;
 RegWrite(L"ToolkitPath", mingwRoot);
 Status(L"Toolkit installed:\r\n" + mingwRoot);
 return true;
}
static void LoadInstallPath() {
 installDir = RegRead(L"InstallPath");
 if(installDir.empty()) {
  wchar_t local[MAX_PATH];
  SHGetFolderPathW(0, CSIDL_LOCAL_APPDATA, 0, 0, local);
  installDir = Join(local, L"Programs\\BineCrabricCPP");
 }
 SetWindowTextW(pathBox, installDir.c_str());
}
static void Install() {
 wchar_t b[4096];
 GetWindowTextW(pathBox, b, 4096);
 installDir = b;
 if(installDir.empty()) {
  Status(L"Choose an install folder.");
  return;
 }
 if(!EnsureToolkit()) return;
 auto client = Join(sourceDir, L"minecraft_native.exe"), server = Join(sourceDir, L"minecraft_server.exe");
 if(!Exists(client)) {
  Status(L"minecraft_native.exe is missing beside setup.exe.");
  return;
 }
 CreateDirectoryW(installDir.c_str(), 0);
 if(!CopyFileW(client.c_str(), Join(installDir, L"minecraft_native.exe").c_str(), FALSE)) {
  Status(L"Could not install minecraft_native.exe. Close the game and retry.");
  return;
 }
 if(Exists(server) && !CopyFileW(server.c_str(), Join(installDir, L"minecraft_server.exe").c_str(), FALSE)) {
  Status(L"Could not install minecraft_server.exe.");
  return;
 }
 if(!CopyTree(Join(sourceDir, L"resources"), Join(installDir, L"resources"))) {
  Status(L"Resource payload is missing or could not be copied.");
  return;
 }
 wchar_t app[MAX_PATH];
 SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, app);
 if(!CopyTree(Join(sourceDir, L"resources"), Join(app, L".minecraft\\resources"))) {
  Status(L"Could not install resources into AppData.");
  return;
 }
 RegWrite(L"InstallPath", installDir);
 Status(L"Installation complete.\r\n" + installDir);
}
static void Optional(bool shaders) {
 wchar_t app[MAX_PATH];
 SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, app);
  auto d = Join(Join(app, L".minecraft"), shaders ? L"shaders" : L"mods");
 Status(shaders ? L"Downloading test shaders..." : L"Downloading test mods...");
 Status(DownloadZip(shaders ? L"shaders" : L"mods", d) ? (shaders ? L"Test shaders installed." : L"Test mods installed.") : L"Optional content download failed.");
}
static void Shortcut() {
 wchar_t b[4096];
 GetWindowTextW(pathBox, b, 4096);
 installDir = b;
 auto exe = Join(installDir, L"minecraft_native.exe");
 if(!Exists(exe)) {
  Status(L"Install the game before creating a shortcut.");
  return;
 }
 wchar_t desk[MAX_PATH];
 SHGetFolderPathW(0, CSIDL_DESKTOPDIRECTORY, 0, 0, desk);
 CoInitialize(0);
 IShellLinkW* s = 0;
 if(SUCCEEDED(CoCreateInstance(CLSID_ShellLink, 0, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&s))) {
  s->SetPath(exe.c_str());
  s->SetWorkingDirectory(installDir.c_str());
  IPersistFile* f = 0;
  if(SUCCEEDED(s->QueryInterface(IID_IPersistFile, (void**)&f))) {
   f->Save(Join(desk, L"BineCrabricCPP.lnk").c_str(), TRUE);
   f->Release();
  }
  s->Release();
 }
 CoUninitialize();
 Status(L"Desktop shortcut created.");
}
static LRESULT CALLBACK Window(HWND h, UINT m, WPARAM w, LPARAM l) {
 if(m == WM_CREATE) {
  wchar_t e[4096];
  GetModuleFileNameW(0, e, 4096);
  sourceDir = Parent(e);
  CreateWindowW(L"STATIC", L"Install location", WS_CHILD | WS_VISIBLE, 18, 16, 150, 20, h, 0, 0, 0);
  pathBox = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 18, 38, 540, 25, h, (HMENU)1, 0, 0);
  CreateWindowW(L"BUTTON", L"Install / Repair", WS_CHILD | WS_VISIBLE, 18, 75, 125, 32, h, (HMENU)2, 0, 0);
  CreateWindowW(L"BUTTON", L"Toolkit", WS_CHILD | WS_VISIBLE, 153, 75, 95, 32, h, (HMENU)3, 0, 0);
  CreateWindowW(L"BUTTON", L"Test mods", WS_CHILD | WS_VISIBLE, 258, 75, 95, 32, h, (HMENU)4, 0, 0);
  CreateWindowW(L"BUTTON", L"Test shaders", WS_CHILD | WS_VISIBLE, 363, 75, 95, 32, h, (HMENU)5, 0, 0);
  CreateWindowW(L"BUTTON", L"Shortcut", WS_CHILD | WS_VISIBLE, 468, 75, 90, 32, h, (HMENU)6, 0, 0);
  statusBox = CreateWindowW(L"STATIC", L"Detecting existing installation and toolkit...", WS_CHILD | WS_VISIBLE, 18, 120, 540, 65, h, 0, 0, 0);
  LoadInstallPath();
  toolkitDir = DetectToolkit();
  Status(toolkitDir.empty() ? L"No complete toolkit detected. It will download only when needed." : L"Existing toolkit detected:\r\n" + toolkitDir);
  return 0;
 }
 if(m == WM_COMMAND && HIWORD(w) == BN_CLICKED) {
  switch(LOWORD(w)) {
  case 2: Install(); break;
  case 3: EnsureToolkit(); break;
  case 4: Optional(false); break;
  case 5: Optional(true); break;
  case 6: Shortcut(); break;
  }
 }
 if(m == WM_DESTROY) PostQuitMessage(0);
 return DefWindowProcW(h, m, w, l);
}
int WINAPI WinMain(HINSTANCE i, HINSTANCE, LPSTR, int) {
 WNDCLASSW c{};
 c.lpfnWndProc = Window;
 c.hInstance = i;
 c.lpszClassName = L"BineCrabricSetup";
 c.hCursor = LoadCursorW(0, IDC_ARROW);
 RegisterClassW(&c);
 CreateWindowW(c.lpszClassName, L"BineCrabricCPP Setup", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 595, 235, 0, 0, i, 0);
 MSG m;
 while(GetMessageW(&m, 0, 0, 0) > 0) {
  TranslateMessage(&m);
  DispatchMessageW(&m);
 }
 return 0;
}
