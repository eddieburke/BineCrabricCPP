# BineCrabicCPP

A native **C++20 port of Minecraft Beta 1.7.3** with extensive enhancements beyond the original game.

The `mcp/` directory contains the original decompiled Java reference sources, used for protocol parity testing.  
The main codebase lives in `native/`.

---

## What's Added vs Vanilla Minecraft Beta 1.7.3

### Language & Platform

|                  | Vanilla Java          | BineCrabicCPP                                      |
|------------------|-----------------------|----------------------------------------------------|
| **Language**     | Java 6 (JVM)          | **C++20** native executable                        |
| **Windowing**    | AWT/Swing Canvas      | **GLFW 3**                                         |
| **Audio**        | paulscode (LWJGL)     | **XAudio2** backend + **Ogg Vorbis** decoder       |
| **Build**        | Manual MCP pipeline   | **CMake + Ninja**, self-bootstrapping build script |
| **Toolchain**    | JDK required          | **Bundled MinGW GCC 15.2** — no system deps needed |
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

Replaces the original fixed-function OpenGL 1.x pipeline with a **deferred-style FBO renderer** and configurable shaderpacks:
- **Vanilla** — faithful vanilla-like gbuffer pipeline
- **Vibrant** — bloom, volumetric lighting, shadows, color grading
- **PSX** — retro PlayStation 1 aesthetic

### Content Registration System

Typed registries with lifecycle phases for blocks, items, entities, and block entities, usable from both C++ and Lua.

### Testing Infrastructure
- **GoogleTest** unit tests for C++ engine components
- **Java parity integration tests** — test protocol compatibility between the native server and the original Java client

### Summary

| Feature              | Vanilla Beta 1.7.3          | BineCrabicCPP                          |
|----------------------|-----------------------------|----------------------------------------|
| Rendering            | Fixed-function OpenGL 1.x   | GLSL shader pipeline, FBOs, shaderpacks|
| Modding              | None                        | Lua 5.4 scripting engine + 23 mods     |
| Audio                | paulscode LWJGL             | XAudio2 + Ogg Vorbis                   |
| Auth                 | Offline                     | MultiMC Microsoft login/auth           |
| Build                | Java/MCP                    | CMake + Ninja + bundled MinGW          |
| Tests                | None                        | GoogleTest + Java parity tests         |
