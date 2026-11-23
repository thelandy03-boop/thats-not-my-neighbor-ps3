# That's Not My Neighbor - PlayStation 3 Port (Native PSL1GHT v2 Demake)

Native PlayStation 3 homebrew demake of **"That's Not My Neighbor"** targeting Cell OS / PPU cross-compilers (PSL1GHT v2), RSX 720p hardware rendering via **Tiny3D**, and an embedded **Vanilla ANSI C Lua 5.4** gameplay scripting engine.

![That's Not My Neighbor PS3](usrdir/assets/banner.png)

---

## 🏛️ Architecture Overview

Previous attempts wrapping heavy desktop runtimes (like Godot 4.x) are fatal on PlayStation 3: Godot 4 requires Vulkan or OpenGL ES 3.0+, exceeds the Cell 256MB XDR RAM budget, and crashes RPCS3 with illegal instruction traps and missing POSIX/graphics syscalls.

This repository implements a **ground-up C demake** with embedded Lua scripting:

```
+-------------------------------------------------------------+
|                     PlayStation 3                           |
|  +--------------------+             +--------------------+  |
|  |     Cell PPU       |             |     RSX Reality    |  |
|  |  256 MB XDR RAM    |             |   256 MB GDDR3     |  |
|  +--------------------+             +--------------------+  |
|            |                                   |            |
|      [Vanilla Lua 5.4]                  [Tiny3D Quad Engine]|
|   State Machine & Logic                 VRAM Texture Cache  |
|            |                                   |            |
|      [lua_bridge.c] <==========================+            |
|   Draw, Audio, Input                                        |
+-------------------------------------------------------------+
```

### Key Technical Specs:
- **No Godot Runtime**: Zero Godot dependencies, zero Vulkan, zero desktop bloat.
- **Vanilla Lua 5.4 Core**: Pure ANSI C Lua engine embedded directly in `src/lua/`. LuaJIT is avoided due to Cell LV2 kernel W^X executable memory protections.
- **Hardware RSX via Tiny3D**: 1280x720 (720p) double-buffered rendering pipeline with static texture caching to prevent runtime memory fragmentation.
- **Split Memory Safety**: Assets are loaded and cached into RSX VRAM once, keeping CPU XDR RAM footprint minimal.
- **DualShock 3 Input**: Virtual cursor system reading analog sticks and buttons through PSL1GHT `ioPad`.

---

## 📁 Repository Structure

```
thats-not-my-neighbor-ps3/
├── Makefile                # PSL1GHT v2 cross-compilation & PKG packaging
├── src/
│   ├── main.c             # PPU bootstrap, video setup, pad input, 60 FPS loop
│   ├── renderer.c / .h    # Tiny3D RSX 720p sprite & quad drawing wrappers
│   ├── audio.c / .h       # Sound effects and BGM playback routines
│   ├── lua_bridge.c / .h  # Native C functions exposed to Lua
│   ├── stb_image.h        # Public domain PNG image decoder
│   └── lua/               # Vanilla Lua 5.4.6 ANSI C core source files
├── usrdir/
│   ├── assets/            # Extracted game assets
│   │   ├── audio/         # Sound effects (.wav) and music (.ogg)
│   │   ├── gfx/           # High-resolution booth, UI, and tenant textures
│   │   └── banner.png
│   └── scripts/
│       ├── main.lua       # Game state machine, virtual cursor, booth controls
│       └── neighbors.lua  # Tenants registry, doppelgänger checks & anomalies
├── ps3/                   # Package metadata (PARAM.SFO, ICON0.PNG, PIC1.PNG)
└── README.md
```

---

## 🎮 DualShock 3 Controls

| Control | Action |
|---|---|
| **Left Analog Stick** | Move Virtual Cursor smoothly across Security Desk |
| **D-Pad (Arrows)** | Nudge Cursor |
| **Cross (✖)** | **Click / Interact**: Toggle Shutter, Pick up documents, Dial phone, Unlock/Lock door, Trigger DDD Alarm |
| **Circle (●)** | **Cancel / Back**: Close examined document, Hang up phone |
| **Start + Select** | Clean exit to XMB / RPCS3 |

---

## 🕹️ Game States & Security Flow

1. **Wait Neighbor**: Shutter is down. A knock or doorbell announces an arrival.
2. **Examine Neighbor**: Shutter lever opens the booth hatch. Examine the visitor's face, clothes, and anomalies.
3. **Inspect Documents**: Click the ID Card or Entry Request on the desk to view details (ID Number, Expiration date, Photo, Apartment). Compare against **Today's List**.
4. **Rotary Telephone (DDD Hotline)**: Click the telephone to dial `3312` to contact the **Department of Doppelganger Detection**.
5. **Door Controls**:
   - **Unlock**: Grant entry to a legitimate tenant.
   - **Lock**: Deny access.
   - **DDD Alarm**: Lift the security cover and hit the red button to dispatch cleaning squads to eliminate a verified Doppelgänger!

---

## 🛠️ Building & Packaging

### Prerequisites
- [PS3DEV PSL1GHT v2 Toolchain](https://github.com/ps3dev/ps3toolchain) (`ppu-gcc`, `make_self`, `pkg.py`).
- Alternatively, compile with Docker/Podman using `hldtux/ps3dev-sdl2:latest`.

### Compilation
```bash
# Build ELF and SELF executable
make

# Build installable NPDRM PKG for PS3 CFW / HEN / RPCS3
make pkg
```

### Running on RPCS3
1. Open RPCS3.
2. Drag and drop `thats-not-my-neighbor.pkg` into RPCS3 to install.
3. Or boot directly: **File -> Boot (S)ELF -> `thats-not-my-neighbor.self`**.

---

## ⚖️ Disclaimer & Credits

- Original game concept and universe created by **NachoSama**.
- This port is an open-source, non-commercial homebrew demake built strictly for technical research and platform preservation.
