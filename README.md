# BineCrabric — Native C++ Minecraft Port

A native C++ port of Minecraft Beta 1.7.3.

## Branches

| Branch | Description |
|--------|-------------|
| `main` | Stable single-threaded renderer |
| `thread-test` | Async multithreaded chunk building (experimental) |

---

## Getting Resources

Game assets are not included (copyrighted by Mojang). You must own a copy of Minecraft Beta 1.7.3 and extract the assets yourself.

### What you need

- `minecraft.jar` from Minecraft Beta 1.7.3  
  Default location on Windows: `%APPDATA%\.minecraft\bin\minecraft.jar`

### Extraction steps

The `.jar` file is a ZIP archive. Extract its contents directly into the `resources/` folder at the root of this repo.

**Using 7-Zip (recommended):**
1. Right-click `minecraft.jar` → 7-Zip → Extract to folder
2. Copy all extracted folders/files into `native/resources/`

**Using PowerShell:**
```powershell
$jar = "$env:APPDATA\.minecraft\bin\minecraft.jar"
$dest = "path\to\native\resources"
Expand-Archive -Path $jar -DestinationPath $dest -Force
```

**Using command line (requires Java installed):**
```bat
cd path\to\native\resources
jar xf "%APPDATA%\.minecraft\bin\minecraft.jar"
```

**Using any ZIP tool:**
Just rename `minecraft.jar` to `minecraft.zip` and extract normally.

### Expected folder structure after extraction

```
resources/
  terrain.png
  terrain/
    sun.png
    moon.png
  environment/
    clouds.png
  gui/
  mob/
  item/
  armor/
  art/
  achievement/
  misc/
  particles.png
  pack.png
  font/
  lang/
  ...
```

### Which version?

The game expects **Beta 1.7.3** assets. Other versions may work partially but are not tested. The jar can be downloaded via the official Minecraft launcher by creating a Beta 1.7.3 installation profile.

---

## Building

### Prerequisites

- CMake 3.20+
- A C++20 compiler (MinGW-w64 / MSVC / Clang)
- OpenGL
- zlib (MSYS2: `pacman -S mingw-w64-x86_64-zlib`)

### Steps

```powershell
cmake -S . -B build -G Ninja
cmake --build build
.\build\minecraft_native.exe
```
