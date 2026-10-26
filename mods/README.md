# Lua mods

Full authoring documentation and small-model agent prompt: [one complete document](../docs/modding/AGENT_PROMPT.md). Compact single-file context for LM Studio: all API groups, 37 events, two working examples, and runtime limitations.

Run `python tools/check-lua-mods.py` from the repository root to parse bundled Lua sources and run focused runtime/mod regressions without compiling the engine. This uses the bundled Lua 5.4 shared library; it does not launch the game.
