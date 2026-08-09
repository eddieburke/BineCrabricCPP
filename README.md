# BineCrabicCPP

A native **C++20 port of Minecraft Beta 1.7.3** with extensive enhancements beyond the original game.

For textures to load:
**Extract minecraft.jar's contents into %appdata%/.minecraft/resources for the game to display the textures**
*or*
**Use the custom installer provided** (`src/installer/installer.cpp`, build target `minecraft_installer`)

---

## What's Added vs Vanilla Minecraft Beta 1.7.3

### Language & Platform

|                  | Vanilla Java          | BineCrabicCPP                                      |
|------------------|-----------------------|----------------------------------------------------|
| **Language**     | Java 6 (JVM)          | **C++20** native executable                        |
| **Windowing**    | AWT/Swing Canvas      | **GLFW 3**                                         |
| **Audio**        | paulscode (LWJGL)     | **XAudio2** backend + **Ogg Vorbis** decoder       |
| **Build**        | Manual MCP pipeline   | **CMake + Ninja**, self-bootstrapping build script |
| **Toolchain**    | JDK required          | **Bundled MinGW GCC 15.2**, no system deps needed  |
| **Auth**         | Offline only          | **Microsoft account authentication**               |

### Lua Modding Engine

A full **Lua 5.4 scripting runtime** embedded into the game, letting mods register custom:
- Blocks, items, entities, crafting recipes
- GUI screens, inventory interactions
- World manipulation and raycasting
- Sound playback and texture management
- Camera controls and rendering hooks
- Lots of example mods included
- Some functionality may be buggy.

### GLSL Shaderpack System

Replaces the original fixed-function OpenGL 1.x pipeline with a **deferred-style FBO renderer** driven entirely by shaderpacks. There is no separate legacy path: the vanilla look is itself a shipped pack (embedded in the binary), so vanilla and modded rendering go through the same code.

The loader targets **Iris/OptiFine shaderpack conventions**: `gbuffers_*`/`deferred*`/`composite*` program stages, `#include` resolution, GLSL preprocessing and core-profile transformation, custom uniforms, compute passes, colortex format declarations, and pack option menus.

Third-party shaderpacks are **not** distributed with this repo (`shaders/` is gitignored). Drop an unzipped pack into `shaders/<packname>/` yourself and it will be picked up. Packs written for Iris are the target, so most should load without modification.

### Content Registration System

Typed registries with lifecycle phases for blocks, items, entities, and block entities, usable from both C++ and Lua.

### Testing Infrastructure
- **GoogleTest** unit tests for C++ engine components
- **Java parity integration tests** that check protocol compatibility between the native server and the original Java client

### Summary

| Feature              | Vanilla Beta 1.7.3          | BineCrabicCPP                          |
|----------------------|-----------------------------|----------------------------------------|
| Rendering            | Fixed-function OpenGL 1.x   | GLSL deferred pipeline, Iris-style packs|
| Modding              | None                        | Lua 5.4 scripting engine + 20 mods     |
| Audio                | paulscode LWJGL             | XAudio2 + Ogg Vorbis                   |
| Auth                 | Offline                     | MultiMC Microsoft login/auth           |
| Build                | Java/MCP                    | CMake + Ninja + bundled MinGW          |
| Tests                | None                        | GoogleTest + Java parity tests         |
