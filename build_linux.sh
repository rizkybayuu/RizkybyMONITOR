#!/usr/bin/env bash
# =======================================================
#       RizkybyMONITOR v1.1 - Linux 1-Click Builder
# =======================================================

# 1. Ensure the script runs in a terminal emulator so build output is visible
if [ ! -t 0 ]; then
    for term in konsole gnome-terminal xfce4-terminal kitty alacritty foot xterm; do
        if command -v "$term" >/dev/null 2>&1; then
            case "$term" in
                konsole)              exec konsole -e bash -c "bash \"$0\"; read -rp 'Press Enter to exit...'" ;;
                gnome-terminal)       exec gnome-terminal -- bash -c "bash \"$0\"; read -rp 'Press Enter to exit...'" ;;
                xfce4-terminal)       exec xfce4-terminal -e "bash -c 'bash \"$0\"; read -rp \"Press Enter to exit...\"'" ;;
                kitty|alacritty|foot) exec "$term" -e bash -c "bash \"$0\"; read -rp 'Press Enter to exit...'" ;;
                xterm)                exec xterm -e bash -c "bash \"$0\"; read -rp 'Press Enter to exit...'" ;;
            esac
        fi
    done
fi

echo "======================================================="
echo "   RizkybyMONITOR v1.1 - Linux 1-Click Builder"
echo "======================================================="
echo ""

# =======================================================================
# [PRIORITY 1] Check System Specs & Architecture
# =======================================================================
SYS_ARCH=$(uname -m)
DISTRO_FAMILY="unknown"
SYS_DISTRO="Linux"

detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        SYS_DISTRO="${PRETTY_NAME:-$NAME}"
        local combined="${ID:-} ${ID_LIKE:-}"

        case "$combined" in
            *void*)                                   DISTRO_FAMILY="xbps" ;;
            *debian*|*ubuntu*|*devuan*|*mx*|*antix*|*deepin*|*kali*|*raspbian*|*linuxmint*|*pop*|*zorin*|*elementary*)
                                                        DISTRO_FAMILY="apt" ;;
            *fedora*|*rhel*|*centos*|*rocky*|*alma*|*oracle*|*mageia*)
                                                        DISTRO_FAMILY="dnf" ;;
            *suse*|*sles*|*sled*)                      DISTRO_FAMILY="zypper" ;;
            *arch*|*manjaro*|*endeavouros*|*garuda*|*arco*|*artix*)
                                                        DISTRO_FAMILY="pacman" ;;
            *alpine*|*postmarketos*)                   DISTRO_FAMILY="apk" ;;
            *gentoo*|*calculate*)                      DISTRO_FAMILY="emerge" ;;
            *solus*)                                   DISTRO_FAMILY="eopkg" ;;
            *nixos*)                                   DISTRO_FAMILY="nix" ;;
            *slackware*)                                DISTRO_FAMILY="slackware" ;;
        esac
    fi

    # Fallback: detect by binary presence if os-release didn't resolve it
    if [ "$DISTRO_FAMILY" = "unknown" ]; then
        if   command -v xbps-install  >/dev/null 2>&1; then DISTRO_FAMILY="xbps"
        elif command -v apt-get       >/dev/null 2>&1; then DISTRO_FAMILY="apt"
        elif command -v dnf           >/dev/null 2>&1; then DISTRO_FAMILY="dnf"
        elif command -v yum           >/dev/null 2>&1; then DISTRO_FAMILY="dnf"
        elif command -v zypper        >/dev/null 2>&1; then DISTRO_FAMILY="zypper"
        elif command -v pacman        >/dev/null 2>&1; then DISTRO_FAMILY="pacman"
        elif command -v apk           >/dev/null 2>&1; then DISTRO_FAMILY="apk"
        elif command -v emerge        >/dev/null 2>&1; then DISTRO_FAMILY="emerge"
        elif command -v eopkg         >/dev/null 2>&1; then DISTRO_FAMILY="eopkg"
        elif command -v nix-env       >/dev/null 2>&1; then DISTRO_FAMILY="nix"
        elif command -v slackpkg      >/dev/null 2>&1; then DISTRO_FAMILY="slackware"
        fi
    fi
}

detect_distro

echo "[INFO] System Architecture : $SYS_ARCH"
echo "[INFO] OS Distribution     : $SYS_DISTRO"
echo "[INFO] Package Manager     : $DISTRO_FAMILY"
echo ""

# =======================================================================
# [PRIORITY 2] Multi-Distro Dependency Resolver
# =======================================================================

# Candidate package names per dependency, per package manager.
# Ordered from most likely/modern to oldest fallback name.
get_candidates() {
    local dep="$1"
    case "$DISTRO_FAMILY:$dep" in
        apt:compiler)         echo "build-essential g++ gcc" ;;
        apt:pkgconfig)        echo "pkg-config pkgconf" ;;
        apt:gtk3)             echo "libgtk-3-dev" ;;
        apt:webkit2gtk)       echo "libwebkit2gtk-4.1-dev libwebkit2gtk-4.0-dev libwebkit2gtk-3.0-dev" ;;
        apt:smartmontools)    echo "smartmontools" ;;

        dnf:compiler)         echo "gcc-c++ gcc" ;;
        dnf:pkgconfig)        echo "pkgconf-pkg-config pkgconfig" ;;
        dnf:gtk3)             echo "gtk3-devel" ;;
        dnf:webkit2gtk)       echo "webkit2gtk4.1-devel webkit2gtk4.0-devel webkit2gtk3-devel" ;;
        dnf:smartmontools)    echo "smartmontools" ;;

        zypper:compiler)      echo "gcc-c++ gcc" ;;
        zypper:pkgconfig)     echo "pkgconf-pkg-config pkg-config" ;;
        zypper:gtk3)          echo "gtk3-devel" ;;
        zypper:webkit2gtk)    echo "webkit2gtk3-soup2-devel webkit2gtk3-devel webkit2gtk4-devel" ;;
        zypper:smartmontools) echo "smartmontools" ;;

        pacman:compiler)      echo "base-devel gcc" ;;
        pacman:pkgconfig)     echo "pkgconf" ;;
        pacman:gtk3)          echo "gtk3" ;;
        pacman:webkit2gtk)    echo "webkit2gtk-4.1 webkit2gtk" ;;
        pacman:smartmontools) echo "smartmontools" ;;

        xbps:compiler)        echo "gcc" ;;
        xbps:pkgconfig)       echo "pkgconf" ;;
        xbps:gtk3)            echo "gtk+3-devel" ;;
        xbps:webkit2gtk)      echo "webkitgtk-devel" ;;
        xbps:smartmontools)   echo "smartmontools" ;;

        apk:compiler)         echo "build-base g++ gcc" ;;
        apk:pkgconfig)        echo "pkgconf pkgconfig" ;;
        apk:gtk3)             echo "gtk+3.0-dev" ;;
        apk:webkit2gtk)       echo "webkit2gtk-4.1-dev webkit2gtk-dev" ;;
        apk:smartmontools)    echo "smartmontools" ;;

        emerge:compiler)      echo "sys-devel/gcc" ;;
        emerge:pkgconfig)     echo "dev-util/pkgconf" ;;
        emerge:gtk3)          echo "x11-libs/gtk+:3" ;;
        emerge:webkit2gtk)    echo "net-libs/webkit-gtk:4.1 net-libs/webkit-gtk:4" ;;
        emerge:smartmontools) echo "sys-apps/smartmontools" ;;

        eopkg:compiler)       echo "gcc g++" ;;
        eopkg:pkgconfig)      echo "pkg-config" ;;
        eopkg:gtk3)           echo "libgtk-3-devel" ;;
        eopkg:webkit2gtk)     echo "libwebkit-gtk-devel" ;;
        eopkg:smartmontools)  echo "smartmontools" ;;

        nix:compiler)         echo "gcc" ;;
        nix:pkgconfig)        echo "pkg-config" ;;
        nix:gtk3)             echo "gtk3" ;;
        nix:webkit2gtk)       echo "webkitgtk" ;;
        nix:smartmontools)    echo "smartmontools" ;;

        *) echo "" ;;
    esac
}

# Wraps the install command for the current package manager.
# Captures combined stdout+stderr into $PM_INSTALL_LOG so the caller
# can show/classify the real error instead of just a pass/fail flag.
PM_INSTALL_LOG=""

pm_install() {
    local pkg="$1"
    PM_INSTALL_LOG="$(mktemp)"
    local status

    case "$DISTRO_FAMILY" in
        apt)    sudo apt-get install -y "$pkg" >"$PM_INSTALL_LOG" 2>&1 ;;
        dnf)    if command -v dnf >/dev/null 2>&1; then
                    sudo dnf install -y "$pkg" >"$PM_INSTALL_LOG" 2>&1
                else
                    sudo yum install -y "$pkg" >"$PM_INSTALL_LOG" 2>&1
                fi ;;
        zypper) sudo zypper --non-interactive install "$pkg" >"$PM_INSTALL_LOG" 2>&1 ;;
        pacman) sudo pacman -S --noconfirm --needed "$pkg" >"$PM_INSTALL_LOG" 2>&1 ;;
        xbps)   sudo xbps-install -Sy "$pkg" >"$PM_INSTALL_LOG" 2>&1 ;;
        apk)    sudo apk add "$pkg" >"$PM_INSTALL_LOG" 2>&1 ;;
        emerge) sudo emerge --ask=n "$pkg" >"$PM_INSTALL_LOG" 2>&1 ;;
        eopkg)  sudo eopkg install -y "$pkg" >"$PM_INSTALL_LOG" 2>&1 ;;
        nix)    nix-env -iA "nixpkgs.$pkg" >"$PM_INSTALL_LOG" 2>&1 ;;
        *)      echo "No handler for package manager '$DISTRO_FAMILY'" >"$PM_INSTALL_LOG"
                return 1 ;;
    esac
    status=$?
    return $status
}

INSTALL_SUCCESS=()
INSTALL_FAILED=()
INSTALL_FAILED_KEY=()   # keeps the dep-key ("gtk3", "webkit2gtk", ...) for manual-instructions lookup

# Reads $PM_INSTALL_LOG and guesses the failure reason from common
# package-manager error phrases, so the person sees a real cause
# instead of a generic "failed" message.
classify_install_failure() {
    local log="$1"
    if [ ! -f "$log" ]; then
        echo "Unknown error (no output captured)."
        return
    fi

    if grep -qiE "could not resolve|temporary failure in name resolution|network is unreachable|unable to connect|no route to host|connection timed out" "$log"; then
        echo "No internet connection / repository mirror unreachable."
    elif grep -qiE "no space left on device" "$log"; then
        echo "Disk is full — free up space and retry."
    elif grep -qiE "incorrect password|sorry, try again|authentication failure|a password is required|not in the sudoers file" "$log"; then
        echo "Sudo password rejected or user lacks sudo privileges."
    elif grep -qiE "unable to locate package|no package.*found|not found in repositor|no matching package|package '.*' was not found|no candidate version found" "$log"; then
        echo "Package name does not exist in this distro's repository (name may differ per release)."
    elif grep -qiE "conflicts with|dependency problem|unmet dependencies|breaks package|file.*conflict" "$log"; then
        echo "Conflicts with an already-installed package."
    elif grep -qiE "lock.*held by|could not get lock|another instance.*running|database is locked" "$log"; then
        echo "Package manager is locked by another running process (close other installers and retry)."
    else
        echo "Unrecognized error — see raw output below."
    fi
}

# Tries each candidate package name in order for one dependency.
# Stops at the first success. Tries at most 5 candidate names, or
# every candidate available if there are fewer than 5.
install_dependency() {
    local dep_label="$1"
    local dep_key="$2"
    shift 2
    local candidates=("$@")
    local max_attempts=5
    local attempt=0

    echo "-------------------------------------------------------"
    echo "   [INSTALL] Resolving: $dep_label"

    for pkg in "${candidates[@]}"; do
        [ -z "$pkg" ] && continue
        attempt=$((attempt + 1))
        [ "$attempt" -gt "$max_attempts" ] && break

        echo "[TRY $attempt] $dep_label -> trying package name: $pkg"
        if pm_install "$pkg"; then
            echo "[OK] $dep_label installed successfully as '$pkg'"
            INSTALL_SUCCESS+=("$dep_label ($pkg)")
            rm -f "$PM_INSTALL_LOG"
            return 0
        else
            local reason
            reason="$(classify_install_failure "$PM_INSTALL_LOG")"
            echo "[FAILED] '$pkg' -> $reason"
            echo "         Last output lines:"
            tail -n 3 "$PM_INSTALL_LOG" 2>/dev/null | sed 's/^/         | /'
            rm -f "$PM_INSTALL_LOG"
        fi
    done

    echo "[SKIP] Could not install $dep_label after $attempt attempt(s). Continuing with the remaining dependencies..."
    INSTALL_FAILED+=("$dep_label")
    INSTALL_FAILED_KEY+=("$dep_key")
    return 1
}

# Prints copy-pasteable manual install commands for anything that failed.
print_manual_instructions() {
    local i=0
    for dep_label in "${INSTALL_FAILED[@]}"; do
        local dep_key="${INSTALL_FAILED_KEY[$i]}"
        local names
        names=$(get_candidates "$dep_key")
        i=$((i + 1))

        echo ""
        echo "  * $dep_label"
        case "$DISTRO_FAMILY" in
            apt)    echo "      sudo apt-get install -y $names" ;;
            dnf)    echo "      sudo dnf install -y $names   (or: sudo yum install -y $names)" ;;
            zypper) echo "      sudo zypper install $names" ;;
            pacman) echo "      sudo pacman -S $names" ;;
            xbps)   echo "      sudo xbps-install -S $names" ;;
            apk)    echo "      sudo apk add $names" ;;
            emerge) echo "      sudo emerge $names" ;;
            eopkg)  echo "      sudo eopkg install $names" ;;
            nix)    echo "      nix-env -iA nixpkgs.<package>   (try each: $names)" ;;
            slackware)
                echo "      No official binary devel package. Build from SlackBuilds.org:"
                echo "      https://slackbuilds.org/result/?search=$dep_key"
                ;;
            *)      echo "      (unknown package manager - install one of: $names)" ;;
        esac
        echo "      Note: try each name in order, some may not exist on your specific release."
    done
}

echo "[INFO] Scanning for available compilers and libraries..."

# --- Detect what's actually missing before touching anything ---
NEED_COMPILER=false
NEED_PKGCONFIG=false
NEED_GTK3=false
NEED_WEBKIT=false
NEED_SMARTMON=false

if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
    NEED_COMPILER=true
fi
if ! command -v pkg-config >/dev/null 2>&1 && ! command -v pkgconf >/dev/null 2>&1; then
    NEED_PKGCONFIG=true
fi
if ! command -v pkg-config >/dev/null 2>&1 || ! pkg-config --exists gtk+-3.0; then
    NEED_GTK3=true
fi
if ! command -v pkg-config >/dev/null 2>&1 || (! pkg-config --exists webkit2gtk-4.0 && ! pkg-config --exists webkit2gtk-4.1); then
    NEED_WEBKIT=true
fi
if ! command -v smartctl >/dev/null 2>&1; then
    NEED_SMARTMON=true
fi

if $NEED_COMPILER || $NEED_PKGCONFIG || $NEED_GTK3 || $NEED_WEBKIT || $NEED_SMARTMON; then
    echo "-------------------------------------------------------"
    echo "   [WARNING] The following build dependencies are missing:"
    $NEED_COMPILER   && echo "  - C++ Compiler"
    $NEED_PKGCONFIG  && echo "  - pkg-config"
    $NEED_GTK3       && echo "  - GTK+3 Development Headers"
    $NEED_WEBKIT     && echo "  - WebKit2GTK Development Headers"
    $NEED_SMARTMON   && echo "  - smartmontools"
    echo "-------------------------------------------------------"
    echo ""
    read -rp "[PROMPT] Try installing missing packages automatically via $DISTRO_FAMILY? (y/N): " CONFIRM


    if [[ "$CONFIRM" =~ ^[Yy]$ ]]; then
        echo ""
        echo "[INFO] Detected package manager: $DISTRO_FAMILY"
        echo "[INFO] Refreshing package index..."
        case "$DISTRO_FAMILY" in
            apt)    sudo apt-get update ;;
            dnf)    if command -v dnf >/dev/null 2>&1; then sudo dnf makecache; else sudo yum makecache; fi ;;
            zypper) sudo zypper --non-interactive refresh ;;
            pacman) sudo pacman -Sy ;;
            xbps)   sudo xbps-install -S ;;
            apk)    sudo apk update ;;
            emerge) sudo emaint sync -a 2>/dev/null ;;
            eopkg)  sudo eopkg update-repo ;;
            nix)    nix-channel --update 2>/dev/null ;;
            slackware)
                echo "[INFO] Slackware detected: no reliable binary devel packages for"
                echo "       webkit2gtk/gtk3 exist in the official tree. Skipping auto-install —"
                echo "       see manual instructions at the end of this script."
                ;;
        esac
        echo ""

        if $NEED_COMPILER; then
            mapfile -t CANDS < <(get_candidates compiler)
            install_dependency "C++ Compiler" "compiler" "${CANDS[@]}"
        fi
        if $NEED_PKGCONFIG; then
            mapfile -t CANDS < <(get_candidates pkgconfig)
            install_dependency "pkg-config" "pkgconfig" "${CANDS[@]}"
        fi
        if $NEED_GTK3; then
            mapfile -t CANDS < <(get_candidates gtk3)
            install_dependency "GTK+3 Development Headers" "gtk3" "${CANDS[@]}"
        fi
        if $NEED_WEBKIT; then
            if [ "$DISTRO_FAMILY" = "xbps" ]; then
                WEBKIT_PKG=$(xbps-query -Rs "webkit" 2>/dev/null | grep -E '\-devel\b' | grep -E 'webkit2?gtk' | head -n 1 | awk '{print $2}' | sed 's/-[0-9].*//')
                [ -z "$WEBKIT_PKG" ] && WEBKIT_PKG="webkitgtk-devel"
                install_dependency "WebKit2GTK Development Headers" "webkit2gtk" "$WEBKIT_PKG"
            else
                mapfile -t CANDS < <(get_candidates webkit2gtk)
                install_dependency "WebKit2GTK Development Headers" "webkit2gtk" "${CANDS[@]}"
            fi
        fi
        if $NEED_SMARTMON; then
            mapfile -t CANDS < <(get_candidates smartmontools)
            install_dependency "smartmontools (SMART/TBW disk telemetry)" "smartmontools" "${CANDS[@]}"
        fi

        # --- Final summary ---
        echo ""
        echo "======================================================="
        echo "   DEPENDENCY INSTALL SUMMARY"
        echo "======================================================="
        if [ ${#INSTALL_SUCCESS[@]} -gt 0 ]; then
            echo "[OK] Installed automatically:"
            for item in "${INSTALL_SUCCESS[@]}"; do
                echo "  - $item"
            done
        fi

        if [ ${#INSTALL_FAILED[@]} -eq 0 ]; then
            echo ""
            echo "[SUCCESS] All dependencies are satisfied. Proceeding to compile..."
            echo ""
        else
            echo ""
            echo "[WARNING] The following dependencies could NOT be installed automatically:"
            for item in "${INSTALL_FAILED[@]}"; do
                echo "  - $item"
            done
            echo ""
            echo "-------------------------------------------------------"
            echo "MANUAL INSTALLATION REQUIRED"
            echo "Install the package(s) below yourself, then re-run this script:"
            echo "-------------------------------------------------------"
            print_manual_instructions
            echo ""
            read -rp "Press Enter to exit..."
            exit 1
        fi
    else
        echo "[SKIP] User declined automatic package-manager install."
        echo ""
        echo "======================================================="
        echo "   [ERROR] Failed to find or install required dependencies."
        echo "======================================================="
        read -rp "Press Enter to exit..."
        exit 1
    fi
fi

# =======================================================================
# [PRIORITY 3] Detect Compiler & Target Libraries
# =======================================================================
COMPILER_TYPE=""
COMPILER_BIN=""
if command -v g++ >/dev/null 2>&1; then
    COMPILER_BIN="$(command -v g++)"
    COMPILER_TYPE="GCC"
    echo "[FOUND] $COMPILER_TYPE Compiler    : \"$COMPILER_BIN\""
elif command -v clang++ >/dev/null 2>&1; then
    COMPILER_BIN="$(command -v clang++)"
    COMPILER_TYPE="Clang"
    echo "[FOUND] $COMPILER_TYPE Compiler    : \"$COMPILER_BIN\""
fi

PKG_DEPS=""
if pkg-config --exists gtk+-3.0 webkit2gtk-4.0; then
    PKG_DEPS="gtk+-3.0 webkit2gtk-4.0"
elif pkg-config --exists gtk+-3.0 webkit2gtk-4.1; then
    PKG_DEPS="gtk+-3.0 webkit2gtk-4.1"
else
    echo "[ERROR] Target library (webkit2gtk-4.0 / 4.1) not found by pkg-config."
    read -rp "Press Enter to exit..."
    exit 1
fi

echo "[INFO] WebKit2GTK SDK      : \"$PKG_DEPS\""
echo ""

# =======================================================================
# [PRIORITY 4] Run Compilation
# =======================================================================
# =======================================================================
# [PRIORITY 4] Auto-Configure Disk Telemetry Permissions & Run Compilation
# =======================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Otomatisasi Izin smartctl 1-Click (Mendeteksi File Kosong & Meminta Password 1x Saat Build)
SUDOERS_FILE="/etc/sudoers.d/rizkybymonitor_smartctl"
SMART_BIN_1="$(which smartctl 2>/dev/null)"
SMART_BIN_2="/usr/bin/smartctl"
SMART_BIN_3="/usr/sbin/smartctl"

if [ ! -f "$SUDOERS_FILE" ] || [ ! -s "$SUDOERS_FILE" ]; then
    echo "======================================================="
    echo "[INFO] Configuring SSD & HDD TBW/SMART disk telemetry permissions..."
    echo "[INFO] Enter your sudo password once to grant access:"
    echo "======================================================="
    sudo rm -f /etc/sudoers.d/rizkybymonitor_smartctl
    sudo bash -c "echo '$USER ALL=(ALL) NOPASSWD: $SMART_BIN_1, $SMART_BIN_2, $SMART_BIN_3' > '$SUDOERS_FILE' && echo '%wheel ALL=(ALL) NOPASSWD: $SMART_BIN_1, $SMART_BIN_2, $SMART_BIN_3' >> '$SUDOERS_FILE' && chmod 0440 '$SUDOERS_FILE'"
    if [ $? -eq 0 ]; then
        echo "[OK] smartctl permissions configured successfully."
    fi
    echo ""
fi

TARGET_SRC=""
if [ -f "main_linux.cpp" ]; then
    TARGET_SRC="main_linux.cpp"
elif [ -f "src/main_linux.cpp" ]; then
    TARGET_SRC="src/main_linux.cpp"
elif [ -f "main_linux_5.cpp" ]; then
    TARGET_SRC="main_linux_5.cpp"
else
    echo "[ERROR] Source file not found (main_linux.cpp / src/main_linux.cpp)."
    read -rp "Press Enter to exit..."
    exit 1
fi

OUTPUT_BIN="$SCRIPT_DIR/rizkybymonitor_linux"

echo ""
echo "======================================================="
echo "   DEPENDENCY RESOLUTION SUMMARY"
echo "======================================================="
echo "[OK] Compiler   : $COMPILER_TYPE ($COMPILER_BIN)"
echo "[OK] WebKit2GTK : $PKG_DEPS"
echo "[OK] Source     : $TARGET_SRC"
echo "======================================================="
echo ""
echo "[INFO] Compiling $TARGET_SRC into $(basename "$OUTPUT_BIN")"
echo ""

BUILD_LOG="$(mktemp)"

$COMPILER_BIN -std=c++17 -O2 -pthread "$TARGET_SRC" -o "$OUTPUT_BIN" $(pkg-config --cflags --libs $PKG_DEPS) 2> >(tee "$BUILD_LOG" >&2)
COMPILE_STATUS=$?

if [ $COMPILE_STATUS -ne 0 ]; then
    echo ""
    echo "======================================================="
    echo "   [ERROR] Build Failed! Full compiler output below:"
    echo "======================================================="
    cat "$BUILD_LOG"
    echo "======================================================="
    rm -f "$BUILD_LOG"
    echo ""
    read -rp "Press Enter to exit..."
    exit 1
fi

rm -f "$BUILD_LOG"
echo "[SUCCESS] Compilation finished with no errors."

chmod +x "$OUTPUT_BIN"

echo ""
echo "======================================================="
echo "   [SUCCESS] $(basename "$OUTPUT_BIN") is ready!"
echo "======================================================="
echo ""
read -rp "Press Enter to continue..."
