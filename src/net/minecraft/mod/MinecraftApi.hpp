#pragma once
#if defined(_WIN32)
#if defined(MINECRAFT_MOD_PLUGIN)
#define MINECRAFT_API __declspec(dllimport)
#elif defined(MINECRAFT_NATIVE_EXPORTS)
#define MINECRAFT_API __declspec(dllexport)
#else
#define MINECRAFT_API
#endif
#else
#define MINECRAFT_API
#endif
