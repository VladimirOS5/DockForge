# DockForge 🚀

> **The ultimate Windows 11 taskbar replacement** — inspired by macOS Dock, built for power users.

[![Version](https://img.shields.io/badge/version-1.0.0-blue)](https://github.com/VladimirOS5/DockForge/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2011%20%7C%20Windows%2010%2020H1+-blue)](https://www.microsoft.com/windows)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE.txt)

![DockForge Preview](docs/preview.png)

## Features

- 🎨 **Stunning Visual Effects** — Acrylic, Liquid Glass, and Audio-Reactive backgrounds
- 🎵 **Audio-Reactive Dock** — Particles and gradients that dance to your music
- 🖥️ **Multi-Monitor Support** — Independent docks on every display
- ⚡ **Performance Profiles** — Eco, Balanced, Performance, and Custom modes
- 🔄 **Auto-Updates** — Silent OTA updates with rollback protection
- 🧩 **Plugin System** — Widgets, custom extensions, hot-reload
- 🎯 **Per-Monitor DPI** — Crisp rendering on any display scale
- 🌈 **HDR Aware** — Respects HDR display settings
- 🔒 **Crash Recovery** — Safe mode, self-tests, automatic fallback

## System Requirements

| Requirement | Minimum | Recommended |
|-------------|---------|-------------|
| OS | Windows 10 20H1 (19041) | Windows 11 23H2+ |
| RAM | 4 GB | 8 GB |
| GPU | Direct2D compatible | DirectX 11 capable |
| Display | 96 DPI | 144 DPI+ |

## Installation

### Installer (Recommended)
Download `DockForge_1.0.0_Setup.exe` from [Releases](https://github.com/VladimirOS5/DockForge/releases) and run it.

### Portable
1. Download `DockForge_1.0.0_Portable.zip`
2. Extract to any folder
3. Run `DockForge.exe`

## Building from Source

### Prerequisites
- Visual Studio 2022 (C++20 workload)
- CMake 3.20+
- Inno Setup 6.2+ (for installer)

### Build
```batch
# Clone repository
git clone https://github.com/VladimirOS5/DockForge.git
cd DockForge

# Build
scripts\build_release.bat
```

Output: `dist\DockForge_1.0.0_Setup.exe`

## Configuration

Config is stored in `%LOCALAPPDATA%\DockForge\config.json`.

### Key Settings

```json
{
    "backgroundEffect": "acrylic",
    "audioReactiveBackground": false,
    "autoCheckUpdates": true,
    "updateChannel": "stable",
    "performanceProfile": "balanced",
    "multiMonitorMode": "primary",
    "enableMemoryTracking": true
}
```

### Command-Line Arguments

| Argument | Description |
|----------|-------------|
| `/safemode` | Start with all effects disabled |
| `/selftest` | Run diagnostics and exit |
| `/rollback` | Restore previous version |
| `/uninstall` | Clean up all data and exit |

## Architecture

```
DockForge/
├── Core/           # DockWindow, Application, MonitorManager
├── Renderer/       # D2D, Effects, Animations, Audio-Reactive
├── Shell/          # Taskbar, ShellHooks, Tray, Thumbnails
├── Updater/        # OTA, VersionInfo
├── Testing/        # MemoryTracker, StabilityTest
├── Utils/          # Config, Logger, Theme, DPI, HDR
├── Plugin/         # PluginManager, Built-in widgets
└── Settings/       # Settings window
```

## Development Chats

| Chat | Topic | Status |
|------|-------|--------|
| 01 | Architecture & Skeleton | ✅ |
| 02 | Render Engine & Effects | ✅ |
| 03 | Icons & Resources | ✅ |
| 04 | Animation Engine | ✅ |
| 05 | Shell Integration | ⏳ |
| 06 | System Tray & Start Menu | ⏳ |
| 07 | Settings & Theming | ⏳ |
| 08 | Plugin System | ⏳ |
| 09 | Multi-Monitor & Performance | ⏳ |
| 10 | Audio-Reactive Background | ✅ |
| 11 | OTA Updates & Installer | ✅ |
| 12 | Polish, Testing & Release | ✅ |

## License

MIT License — see [LICENSE.txt](LICENSE.txt)

## Credits

- Built with ❤️ by the DockForge Team
- Icons powered by Windows Shell APIs
- Audio capture via WASAPI Loopback
