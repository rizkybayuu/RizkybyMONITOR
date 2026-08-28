# 🎮 RizkybyMONITOR

A state-of-the-art, frameless, multi-window system monitoring dashboard built for Linux Desktop Environments (KDE Plasma, GNOME, XFCE, etc.). Powered by Python, PyWebView, WebKitGTK, and Chart.js.

![RizkybyMONITOR](https://img.shields.io/badge/Platform-Linux%20%7C%20PyWebView-blue?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)

---

## ✨ Features

- **🚀 Multi-Window Architecture**: Duplicate independent monitoring windows (`Ctrl + N`) with persistent geometry, color themes, and per-card states saved in `config.json`.
- **📊 Advanced Hardware Telemetry**:
  - **CPU**: Core-by-core frequency & load monitoring with live line charts.
  - **GPU**: Multi-metric GPU load, VRAM utilization, temperature, and power draw.
  - **RAM & Swap**: Real-time physical RAM, zRAM (zstd engine), and System Swap tracking.
  - **Storage (NVMe/SSD)**: Live disk I/O rates, TBW written/remaining lifetime, and SMART health monitoring.
  - **Network & Nethogs**: Active interface IP/MAC addresses, live per-process bandwidth speeds, and Wi-Fi signal strength.
  - **Battery & Thermal**: Real-time battery charge level, status, and CPU package temperature.
- **🎨 Custom Color Palettes & Dual Theme Modes**:
  - 12 Dark Palettes + 10 Light Palettes with automatic dark text contrast.
  - Independent palette memory per window instance.
- **🔍 Rich Interactivity**:
  - **Left Click**: Toggle Fullscreen Zoom mode on any hardware card.
  - **Right Click**: Toggle Raw Monospaced Log/Detail view.
  - **Middle Click**: Instant copy tooltip or card text content to clipboard with Toast feedback.
  - **Ctrl + Mouse Wheel**: Adjust dashboard UI font zoom dynamically with proportional layout scaling.
  - **Bi-directional Auto-Scroll**: Smooth horizontal auto-scroll with hover-pause and seamless position continuation.

---

## ⌨️ Keyboard Shortcuts

| Shortcut | Description |
| :--- | :--- |
| **`Ctrl + Q`** | Quit all instances and save active layout |
| **`Alt + Q`** | Exit current window instance & release memory |
| **`Ctrl + N`** | Duplicate new independent monitoring window |
| **`Ctrl + Shift + A`** | Toggle Keep Above Other (Pin Window Always-on-Top) |
| **`Alt + F3`** | Open OS Window Operations Menu |
| **`Ctrl + Mouse Wheel`** | Zoom UI font size & dashboard layout scale |

---

## 🛠️ Installation & Setup

### Prerequisites

On Arch Linux / Manjaro:
```bash
sudo pacman -S python-pip python-gobject webkit2gtk nethogs wmctrl xdotool
```

On Ubuntu / Debian:
```bash
sudo apt update
sudo apt install python3-pip python3-gi gir1.2-gtk-3.0 libwebkit2gtk-4.0-dev nethogs wmctrl xdotool
```

### Setup Virtual Environment & Run

```bash
git clone https://github.com/rizkybayuu/RizkybyMONITOR.git
cd RizkybyMONITOR

python3 -m venv venv
source venv/bin/activate
pip install pywebview psutil requests
```

Run the application:
```bash
python3 app.py
```

---

## 📄 License

Distributed under the MIT License. See `LICENSE` for more information.
