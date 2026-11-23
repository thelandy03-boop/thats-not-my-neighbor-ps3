# Reverse Engineering & Asset Extraction Plan

## 🔍 Game Engine Analysis
- **Engine**: Godot Engine (Windows 64-bit export).
- **Executable**: `That's not my neighbor.exe` (~495 MB).
- **Format**: Self-contained Windows PE binary containing an embedded **Godot PCK (`GDPC`)** package at the end of the executable.

## 📦 What the PCK Contains
- **Scripts**: `.gdc` (compiled GDScript bytecode) or `.gd` (plain text GDScript).
- **Scenes**: `.tscn` / `.scn` (Node hierarchies: Booth, Door, Emergency UI, Neighbors).
- **Sprites / Textures**: `.png` / `.stex` (Doppelgangers, IDs, entry permits, D.D.D. logo).
- **Audio**: `.ogg` / `.wav` / `.sample` (Dialogues, phone calls, buzzer, atmospheric music).
- **Game Data**: Dialogue tables, apartment lists, doppelganger rules.

## 🛠️ Extraction Strategy
1. **Locate PCK Header/Footer**: The Godot PCK magic bytes `GDPC` (`0x43504447`) indicate the embedded resource package offset.
2. **Unpack Assets**: Extract raw images, sounds, fonts, and script definitions.
3. **Decompile Bytecode**: Convert `.gdc` back to readable GDScript to inspect game mechanics and Doppelganger detection logic.
4. **Target for PS3 Port**: Re-implement core state machine and render extracted 2D assets using the PS3 C/C++ engine.
