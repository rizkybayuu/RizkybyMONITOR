# RizkybyMONITOR ⚡

[![Release](https://img.shields.io/badge/release-v1.2.1-blue.svg)](https://github.com/rizkybayuu/RizkybyMONITOR/releases)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-00599C.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-FCC624.svg)](https://github.com/rizkybayuu/RizkybyMONITOR)
[![Linux Distros](https://img.shields.io/badge/linux-10%2B%20distro%20families-333333.svg)](#-linux-1-click-autonomous-builder)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

https://github.com/user-attachments/assets/60072469-1e95-41e1-89f1-4d564b85b6c1

---

**RizkybyMONITOR** is an ultra-fast, lightweight, cross-platform native **C++17** hardware monitor and real-time system telemetry suite built with **GTK3 + WebKit2GTK** (Linux) and **Win32 + Microsoft WebView2** (Windows). Designed for power users, it provides sub-millisecond precision telemetry, a frameless borderless glassmorphism multi-window shell, adaptive per-device memory, and vibrant interactive UI themes — without Python, virtual environments, or heavy runtime overhead. Both platform backends serve the exact same shared HTML/JS/CSS frontend, so the experience is 1:1 identical whichever OS you build it on.

---

## 🌟 What's New in v1.2.1
- 🪟 **Fluid Non-Focused Super Drag & Resize**: Immediate window moving (`Super + Left-Click Drag`) and resizing (`Super + Right-Click Drag`) triggered directly from background state without requiring the window to be pre-focused or active.
- 📜 **Simplified Smart Autoscroll on Hover**: Removed redundant click/scroll interaction requirements on Smart Autoscroll (`⏸`). Autoscroll now smoothly runs as soon as the mouse cursor enters the dashboard container (hover) and idles when moving out.
- 🎯 **Seamless Autoscroll Position Continuity (Anti-Jump Fix)**: Completely eliminated scroll jumping upon autoscroll resume by snapshotting exact physical `scrollTop` coordinates and synchronizing animation baseline offsets.
- ⏱️ **Dynamic Real-Time Telemetry Latency in About Panel**: The `Telemetry Latency` specification row in the About dialog is now bound to dynamic DOM state, instantly mirroring live polling intervals set via `Alt + F` (e.g. `1000ms Non-Blocking Polling`).
- 🛡️ **Clean Process Lifecycle & Directory Lock Immunity**: Hardened application exit routines to ensure thorough cleanup of telemetry threads, pipes, and OS handles, preventing background locks on host directories across FAT32 and NTFS filesystems.

---

## 🌟 About Previous in v1.2
- 📜 **Tri-State Smart Autoscroll Engine (`▶` `⏸` `■`)**: Multi-mode autoscroll supporting **Always Active** (`▶`), **Smart Contextual** (`⏸` runs on active window or on inactive windows upon mouse wheel/click interaction and pauses on mouseleave), and **Disabled** (`■` for maximum GPU power saving). Controlled via button click or `Alt + S`.
- ⏱️ **Adaptive Real-Time Telemetry Control (`Alt + F`)**: Frosted glassmorphism refresh rate panel with live slider, mouse wheel scrolling directly on the interval value (min 500ms), glow styling, theme adaptation, and instant `POST /api/config` synchronization across Win32 and GTK3 backends.
- 🌡️ **Dedicated .NET 8 CPU Thermal Sensor Helper**: High-precision package and per-core temperature telemetry via `rzkmon_sensor.exe` (powered by LibreHardwareMonitorLib) with 2-second caching to minimize CPU overhead.
- 🪟 **Super Key Non-Sticking Window Drag**: Fixed low-level keyboard hook release lifecycle so `Super + Drag` and `Super + Right-Click Resize` never leave the Windows key stuck down in the operating system.
- 🛠️ **Hardened 1-Click Autonomous Builders**: Added forced package resolution, real-time NuGet restore feedback, and automated official Microsoft bootstrap fallback.

---

## 📐 Dashboard Layout Architecture

```
+-------------------------------------------------------------------------------------------+
|  RizkybyMONITOR  |  🎨 Neon Cyberpunk ▼  |  🌙 ☀️  |  ▶ ⏸ ■  |  🌡️ 45°C  🔋 95%  🗗  📌  ⏻	|
+-------------------------------------------------------------------------------------------+
|  ⚙️ CPU (P/E Cores & Smart Cache)   	|  ⚡ GPU (Intel Iris Xe / NVIDIA eGPU)				|
|  - Left: Usage Chart & Top 5 Procs 	|  - Dynamic GPU Switcher Dropdown         			|
|  - Right: P-Cores & E-Cores Grid   	|  - RCS/CUDA, Tensor & RT Core Gauges     			|
|  - L1/L2/L3 Smart Cache Speed      	|  - Dedicated VRAM & Shared VRAM Meters   			|
+-------------------------------------------------------------------------------------------+
|  🧠 MEMORY HIERARCHY STACK        	 	|  🌐 NETWORK TELEMETRY                     			|
|  - Smart Cache -> VRAM -> RAM      	|  - Live Bandwidth Throughput (RX ▼ / TX ▲)		|
|  - ZRAM (Compressed Pool)          	|  - Interface Diagnostic & Active IP      			|
|  - External SWAP Drive Partition   	|  - Top 5 Network Processes (RX/TX split) 			|
+-------------------------------------------------------------------------------------------+
|  💽 MULTI-DISK STORAGE (Selected: NVMe / SATA SSD / USB Flash Drive)         				|
|  - Quick-Select Disk Dropdown with Persistent Window Memory                   			|
|  - Read / Write Throughput & Free Storage Bar                                 			|
|  - SMART Health (PASSED), TBW Written & Drive Temperature                     			|
|  - Isolated Per-Device Top 5 Disk I/O Processes (ETW / procfs)                			|
+-------------------------------------------------------------------------------------------+
|  🔋 POWER & SENSORS                                                          				|
|  - Battery Model, Chemistry, Health %, Cycle Count, Voltage, Runtime ETA      			|
|  - CPU Package Temperature (hwmon / MSR-level helper)                        				|
+-------------------------------------------------------------------------------------------+
```

---

## ⌨️ Keyboard Shortcuts & Gestures

| Shortcut / Gesture | Action |
| :--- | :--- |
| **Super + Left-Click Drag** *(or Header Drag)* | Move and drag the frameless window freely across screens |
| **Super + Right-Click Drag** *(or Alt + Right-Click)* | Interactively resize the frameless window from anywhere |
| **Alt + S** *(or Left-Click Autoscroll Button)* | Cycle autoscroll mode (`▶ Always Active` / `⏸ Smart` / `■ Disabled`) |
| **Alt + F** *(or Right-Click Autoscroll Button)* | Open/close floating Telemetry Refresh Interval settings panel (min 500ms) |
| **Mouse Wheel on Refresh Interval** | Increase / decrease telemetry refresh rate directly on the panel |
| **Left Click on Card** | Toggle **Fullscreen Zoom Mode** for focused inspection |
| **Right Click on Card** | Toggle **Detailed Text Mode** (CLI diagnostic logs) for that card |
| **Middle Click on Card** | Copy that card's visible telemetry metrics to the system clipboard |
| **Middle Click on Title / Tooltip** | Copy the full hover-tooltip hardware detail text to the system clipboard |
| **Mouse Wheel on Color Selector** | Instantly cycle through vibrant color palettes |
| **Left Click on Color Selector** | Open full floating palette selection menu (14 Dark / 14 Light) |
| **Mouse Wheel on Mode Switcher** | Toggle between Dark Mode (`🌙`) and Light Mode (`☀️`) |
| **Ctrl + Mouse Wheel** | Zoom UI in / out (adjust base font scale from 8px to 32px) |
| **Ctrl + N** | Duplicate active window into a new independent instance |
| **Ctrl + Shift + A** | Toggle Always-On-Top (Pin above all other desktop windows) |
| **Alt + Q** | Close the current window and clear its local session memory |
| **Ctrl + Q** *(or Left-Click `⏻`)* | Save all window layouts and safely exit the entire application |
| **Right-Click `⏻`** | Clean exit and reset window layout cache |

> 💡 Every window title can also be renamed directly in the UI — the custom name, theme, font size, selected disk/GPU, and pinned detail cards all persist per-window and are restored automatically on the next launch.

---

## 🚀 Building & Running

RizkybyMONITOR features autonomous 1-click builders for both Linux and Windows that automatically resolve compilers, install dependencies, and build the native executable — no manual toolchain setup required on either OS.

---

### 🐧 Linux (1-Click Autonomous Builder)

```bash
chmod +x build_linux.sh
./build_linux.sh
```

`build_linux.sh` will, in order:

1. **Re-launch itself inside a visible terminal** if it wasn't started from one, trying `konsole`, `gnome-terminal`, `xfce4-terminal`, `kitty`, `alacritty`, `foot`, then `xterm`.
2. **Detect your distro & package-manager family** from `/etc/os-release` (`ID`/`ID_LIKE`), with a binary-presence fallback if that file is missing or inconclusive. Supported families:

   | Family | Detected distros (examples) | Install command used |
   | :--- | :--- | :--- |
   | `apt` | Debian, Ubuntu, Mint, Pop!_OS, Devuan, MX Linux, antiX, deepin, Kali, Raspbian, Zorin, elementary OS | `apt-get install -y` |
   | `dnf` | Fedora, RHEL, CentOS, Rocky, AlmaLinux, Oracle Linux, Mageia | `dnf install -y` (falls back to `yum`) |
   | `zypper` | openSUSE, SLES, SLED | `zypper install` |
   | `pacman` | Arch, Manjaro, EndeavourOS, Garuda, ArcoLinux, Artix | `pacman -S --needed` |
   | `xbps` | Void Linux | `xbps-install -Sy` |
   | `apk` | Alpine, postmarketOS | `apk add` |
   | `emerge` | Gentoo, Calculate Linux | `emerge --ask=n` |
   | `eopkg` | Solus | `eopkg install -y` |
   | `nix` | NixOS | `nix-env -iA nixpkgs.<pkg>` |
   | `slackware` | Slackware | No official devel binaries — prints SlackBuilds.org search links |

3. **Check for missing build dependencies**: a C++ compiler (`g++`/`clang++`), `pkg-config`, GTK+3 dev headers, WebKit2GTK 4.0/4.1 dev headers, and `smartmontools`.
4. **Resolve each missing dependency automatically**, trying up to 5 candidate package names per dependency (package names differ across distros/releases — e.g. `libwebkit2gtk-4.1-dev` vs `webkit2gtk4.1-devel` vs `webkit2gtk-4.1`), with the real error reason (no internet, disk full, wrong sudo password, package not found, dependency conflict, or locked package manager) surfaced from the raw package-manager log instead of a generic failure message.
5. If anything still fails to install, **print copy-pasteable manual install commands** for your specific package manager and exit cleanly — no silent half-broken builds.
6. **Configure passwordless `smartctl` access** once, via a dedicated `/etc/sudoers.d/rizkybymonitor_smartctl` rule scoped only to the `smartctl` binary path(s) (never a blanket `NOPASSWD: ALL`), so SMART/TBW disk telemetry works without a password prompt on every refresh.
7. **Compile** `src/main_linux.cpp` (or `main_linux.cpp`) with `g++`/`clang++ -std=c++17 -O2 -pthread`, linking against whichever of `webkit2gtk-4.0` / `webkit2gtk-4.1` `pkg-config` resolves, streaming the full compiler log on failure.

To run the application:
```bash
./rizkybymonitor_linux
```

---

### 🪟 Windows (1-Click Autonomous Builder)

1. Double-click **`build_windows.bat`** or run in Command Prompt:
```bat
build_windows.bat
```
2. Run the compiled executable:
```bat
rizkybymonitor_windows.exe
```

`build_windows.bat` will, in order:

1. **Self-elevate to Administrator** via UAC (`powershell Start-Process -Verb RunAs`) and re-launch itself inside a persistent `cmd /k` window so the output stays visible.
2. **Kill any already-running instance** (`rizkybymonitor_windows.exe`, `rzkmon_sensor.exe`) so the rebuild isn't blocked by a locked file, and **add the build folder to Windows Defender's exclusion list** to prevent false-positive flags on the freshly self-compiled binary.
3. **Enumerate active drives** (via `fsutil fsinfo drives`, with a brute-force `C:`–`Z:` fallback) to scope every subsequent filesystem scan.
4. **Discover a C++ compiler** through a layered search, stopping at the first hit:
   - MSVC via `vswhere.exe` (any Visual Studio edition, including pre-release)
   - `g++`/`clang++` already on `PATH`
   - Known install locations on every active drive (Code::Blocks MinGW, LLVM/Clang, MSYS2 `ucrt64`/`mingw64`/`clang64`, plain `mingw64`/`mingw32`/`MinGW`, TDM-GCC, Strawberry Perl's bundled GCC, `w64devkit`, WinLibs, Qt's bundled MinGW)
   - `winget` / `choco` / `scoop`, offered interactively (installs WinLibs POSIX UCRT / `mingw` / `gcc` respectively)
   - A recursive PowerShell scan of every mounted filesystem drive for `g++.exe`
   - As a last resort, **auto-downloads the latest portable `w64devkit`** release straight from its GitHub Releases API into `tools\w64devkit`
5. **Detect or install the .NET SDK** (via `winget`/`choco`/`scoop`) and, if available, **publish the CPU sensor helper** (`sensor\rzkmon_sensor.exe`, built on LibreHardwareMonitorLib) for MSR-level CPU package temperature — the app still runs fine without it, just with CPU temp reported as N/A.
6. **Locate or download the Microsoft WebView2 SDK** — checking the local NuGet package cache, a local `packages\` folder, `vcpkg` installs on any active drive, and finally downloading it fresh via `nuget.exe`/NuGet API if none are found.
7. **Compile** `src/main_windows.cpp` (or `main_windows.cpp`) — using `cl.exe` (linked against `ws2_32`, `iphlpapi`, `pdh`, `psapi`, `powrprof`, `dxgi`, `ole32`, `oleaut32`, `uuid`, `shlwapi`, `setupapi`, `rpcrt4`, `WebView2LoaderStatic`) when MSVC was found, or a fully static MinGW/Clang build (`-static -static-libgcc -static-libstdc++`, plus `wlanapi`, `shell32`, `wbemuuid`, `dwmapi`, `WebView2Loader.dll.lib`) otherwise — then copies `WebView2Loader.dll` next to the output binary.

> **Note**: `rizkybymonitor_windows.exe` and the `ui/` folder must reside in the same folder.
> `WebView2Loader.dll` is copied alongside the executable automatically by the build script.

---

## 📁 Project Structure

```
RizkybyMONITOR/
├── .github/                 # GitHub platform templates & security policy
│   ├── ISSUE_TEMPLATE/     # Interactive bug & feature request forms
│   │   ├── bug_report.md
│   │   └── feature_request.md
│   ├── pull_request_template.md # Auto-filled PR review checklist
│   └── SECURITY.md          # Integrated GitHub security advisory policy
├── docs/                    # Community & contributor documentation
│   ├── CODE_OF_CONDUCT.md   # Contributor pledge & standards
│   └── CONTRIBUTING.md      # Developer guide, build steps & workflow
├── ui/                      # Modular frontend dashboard UI
│   ├── index.html          # Clean HTML5 structure & layout
│   ├── style.css           # Styling, themes, animations & glassmorphism
│   └── app.js              # Real-time telemetry, charts & UI controller
├── assets/
│   └── overview.mp4        # Hero overview demonstration video
├── src/
│   ├── main_linux.cpp      # Linux backend: C++17 daemon, GTK3, sysfs/procfs telemetry
│   └── main_windows.cpp    # Windows backend: Win32, WebView2, ETW, WMI, IOCTL
├── build_linux.sh          # Linux 1-click multi-distro dependency resolver & compiler
├── build_windows.bat       # Windows 1-click compiler scanner, SDK fetcher & build script
├── README.md               # Project documentation
├── LICENSE                 # MIT License
└── .gitignore              # Git exclusion rules
```

Generated at build time (not tracked in source control):
- **Linux**: `rizkybymonitor_linux` (compiled binary)
- **Windows**: `rizkybymonitor_windows.exe`, `WebView2Loader.dll`, `sensor/rzkmon_sensor.exe` (CPU thermal helper), `tools/` (portable compiler if auto-downloaded), `packages/` (WebView2 SDK if auto-downloaded)

---

## ⚙️ Architecture

RizkybyMONITOR uses the same **shared HTML/JS/CSS frontend** (`index.html`) on both platforms, served locally by an embedded native loopback HTTP server that auto-binds to the first free port in `127.0.0.1:8080-8095` (never exposed beyond localhost). Only the C++ backend differs per operating system:

| Component | Linux | Windows |
| :--- | :--- | :--- |
| **Window Host** | GTK3 (`libgtk-3`), transparent/rounded via Cairo compositing | Win32 API (`CreateWindowExW`), transparent/rounded via DWM (`DWMWA_WINDOW_CORNER_PREFERENCE`, `DWMWA_BORDER_COLOR`) |
| **Web Engine** | WebKit2GTK 4.0 / 4.1 | Microsoft WebView2 (Edge Chromium) |
| **HTTP Daemon** | POSIX Sockets (`sys/socket.h`) | Winsock2 (`ws2_32.lib`) |
| **CPU Telemetry** | `/proc/stat` + dynamic topology | `GetSystemTimes` + `NtQuerySystemInformation` |
| **P/E-Core Split** | Dynamic sysfs core topology detection | `GetLogicalProcessorInformationEx` (RelationProcessorCore) |
| **CPU Smart Cache**| `/sys/devices/system/cpu/cpu*/cache/` | `GetLogicalProcessorInformation` L1/L2/L3 aggregation |
| **CPU Package Temp**| `hwmon` (`coretemp`/`k10temp`/`zenpower`) → `x86_pkg_temp` thermal zone fallback | `rzkmon_sensor.exe` helper (LibreHardwareMonitorLib, MSR-level) |
| **RAM Hierarchy** | `/proc/meminfo` | `GlobalMemoryStatusEx` + `GetPerformanceInfo` |
| **ZRAM / Swap** | `/sys/block/zram0/`, `/proc/swaps` | Windows Pagefile Memory Manager |
| **GPU Telemetry** | Linux DRM/KMS + Intel i915 / `nvidia-smi` / `lspci` multi-adapter enumeration | DXGI Adapter Enum + PDH GPU Engine Counters |
| **Multi-GPU Enumeration** | `lspci` (VGA/3D/Display class) + sysfs DRM / `nvidia-smi` per-adapter VRAM | DXGI `EnumAdapters1` |
| **Disk I/O** | `/proc/diskstats` + `/sys/block/` | PDH `PhysicalDisk` + `DeviceIoControl` |
| **Disk SMART** | `smartctl` with automated sudoers rule | Universal NVMe SMART + IOCTL_ATA_PASS_THROUGH + WMI |
| **Process Disk I/O**| Per-device `/proc/[pid]/io` isolation | **ETW (Event Tracing for Windows)** DiskIo Session |
| **Network** | `/proc/net/dev` | `GetIfTable2` (`iphlpapi`) |
| **Battery / Power** | `/sys/class/power_supply/*` (energy/charge, voltage, cycle count, temp) | `rzkmon_sensor` (.NET helper) + `GetSystemPowerStatus` |
| **Window Persistence** | `config.json` (per-window position, theme, font size, selected disk/GPU, custom title) | `config.json` (active-window registry incl. custom title, restored via `CreateWindowExW`) |

---

## 🛠️ Troubleshooting

- **"Target library (webkit2gtk-4.0 / 4.1) not found by pkg-config" (Linux)** — your distro's WebKit2GTK devel package uses a different name than the script guessed. Re-run `build_linux.sh`, let it fail, then use the manual install command it prints (or search your package manager for `webkit2gtk` / `webkitgtk` devel packages).
- **Slackware / other niche distros** — there's no reliable official devel binary for GTK3/WebKit2GTK; the script will point you to [SlackBuilds.org](https://slackbuilds.org) instead of attempting an automatic install.
- **CPU temperature shows N/A (Windows)** — the `.NET SDK` wasn't available to build `rzkmon_sensor.exe` during setup. Install the .NET SDK and re-run `build_windows.bat`, or leave it as-is — every other metric still works normally.
- **"Output binary not found" (Windows)** — usually a missing WebView2 SDK header/lib mismatch; delete the `packages\` folder and re-run so it re-downloads via NuGet.
- **SMART/TBW shows N/A (Linux)** — make sure `smartmontools` installed correctly and re-run the script once so it can (re)write the passwordless sudoers rule for `smartctl`.
- **Window appears solid black instead of transparent (Linux)** — your compositor isn't active; enable compositing in your window manager/desktop environment settings.

---

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---

## 👤 Author

Developed with ❤️ by **Rizky Bayu** ([@rizkybayuu](https://github.com/rizkybayuu)).
