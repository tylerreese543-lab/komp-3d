# KOMP-3D

Instance-based C++-written 3D game engine using OpenGL as graphics. Useful to create video games, interactive experiences, and many more.

## What's new?
- **`SpatialInstance` Class:** Place and transform instances relative to parent groups in 3D space.
- **Expanded Lighting:** Added directional lights and omni-positional sun lights alongside traditional point lights.
- **Storage Folders:** Mark folders as storage to keep descendants inactive, excluding them from rendering and engine updates.
## Built With
- **C++** — Core engine architecture
- **GLSL** — Custom shaders and rendering pipeline
- **Lua** — Core engine and editor scripting (integrated via `sol2`)
- **XML** — Tooltips and localization strings
## Features
- **OpenGL Rendering:** Custom shader manipulation, post-processing effects, and dynamic lighting.
- **Instance-Based Hierarchy:** Organize game objects, groups, and inactive storage folders cleanly.
- **Audio Engine:** Support for both 3D positional audio and ambient sound playback.
- **Lua Scripting:** Fast, flexible gameplay and editor scripting powered by `sol2`.
- **Serialization:** Custom file stream writing and decoding for saving projects and scenes.
## Roadmap
- [ ] File compression and decompression for saved scenes using `zlib` or `lzma`
- [ ] Dedicated instance-based UI system
- [ ] Multi-language support using XML locale files
