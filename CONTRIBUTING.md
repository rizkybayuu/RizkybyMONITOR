# Contributing to RizkybyMONITOR

Thank you for your interest in contributing to **RizkybyMONITOR**! We welcome bug reports, feature suggestions, documentation improvements, and code contributions.

---

## 🛠️ Development & Coding Standards

RizkybyMONITOR is engineered as an ultra-fast, zero-dependency native hardware monitor. Please keep the following principles in mind when contributing:

1. **C++17 ISO Standard**:
   - Write clean, modern, standard C++17.
   - Avoid external third-party runtime dependencies (no Qt, Electron, Boost, or Python runtimes).
   - Maintain fast startup (< 15ms) and low memory consumption (< 25MB).

2. **Cross-Platform Parity**:
   - Feature changes should maintain 1:1 behavioral parity between Linux (`src/main_linux.cpp`) and Windows (`src/main_windows.cpp`).
   - Both native backends serve the shared frontend (`index.html`).

3. **Performance First**:
   - Telemetry loops must remain strictly non-blocking.
   - Heavy operations (e.g. storage SMART queries, MSR sensor queries) must be cached or decoupled asynchronously.

---

## 🐛 Reporting Bugs

Before reporting a bug, please check the existing [Issues](https://github.com/rizkybayuu/RizkybyMONITOR/issues) to ensure it hasn't already been reported.

When opening a bug report, please provide:
- A clear, concise title and description.
- Your OS version, CPU/GPU hardware details, and architecture (x64 / ARM64).
- Steps to reproduce the issue.
- Expected behavior vs actual observed behavior.

---

## 💡 Suggesting Enhancements

We welcome new hardware sensors and UI ideas! When suggesting a feature:
- Explain the use case and why it benefits users.
- Provide hardware details or mockups if applicable.
- Open a feature request via the issue tracker.

---

## 🔀 Submitting Pull Requests

1. Fork the repository and create a descriptive branch:
   ```bash
   git checkout -b feature/my-new-feature
   ```
2. Test builds on both target platforms:
   - Linux: `./build_linux.sh`
   - Windows: `build_windows.bat`
3. Commit your changes with clear, semantic commit messages.
4. Push to your fork and submit a Pull Request against the `v1.2` or `main` branch.
5. Fill out the pull request template checklist.
