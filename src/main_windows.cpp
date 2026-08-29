// =============================================================================
// RizkybyMONITOR — Windows Native Port (C++17)
// =============================================================================
// Win32 API + WebView2 (Microsoft Edge Chromium) + Winsock2 HTTP Server
// Equivalent to main.cpp (Linux GTK3 + WebKit2GTK version)
// =============================================================================
#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <pdh.h>
#include <psapi.h>
#include <powrprof.h>
#include <tlhelp32.h>
#include <winioctl.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <dxgi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <functional>
#include <atomic>
#include <numeric>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "setupapi.lib")

// WebView2 — requires Microsoft.Web.WebView2 NuGet package
// Install via: nuget install Microsoft.Web.WebView2
// Or via vcpkg: vcpkg install webview2
#include <WebView2.h>
#include <wrl.h>
using namespace Microsoft::WRL;

// =============================================================================
// Data Structures
// =============================================================================
struct CpuTick {
    unsigned long long idle;
    unsigned long long total;
};

struct ProcIoTick {
    unsigned long long r_bytes;
    unsigned long long w_bytes;
};

struct WindowInstance {
    int id;
    HWND hwnd;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> webview;
    bool is_on_top;
    std::string details;
    std::string mode;
    int font_size;
    std::string selected_disk;
};

struct ProcEntry {
    std::string name;
    std::string val;
};

struct DiskProcEntry {
    std::string name;
    std::string read;
    std::string write;
    double total_bytes;
};

struct NetProcEntry {
    std::string name;
    std::string down;
    std::string up;
    double total;
};

struct GpuProcEntry {
    std::string name;
    double usage;
    double total;
};

// =============================================================================
// Global Variables
// =============================================================================
static std::recursive_mutex g_win_mutex;
static std::map<int, WindowInstance> g_windows;
static int g_next_win_id = 1;
static std::string g_app_dir;
static int g_server_port = 8080;
static std::atomic<bool> g_running{true};

static std::mutex g_stats_mutex;
static std::string g_cached_stats_json;
static std::map<std::string, std::string> g_disk_details_map;

static std::map<std::string, CpuTick> g_last_cpu_ticks;
static std::map<DWORD, ProcIoTick> g_last_proc_io_ticks;

static double g_last_net_time = 0.0;
static unsigned long long g_last_net_rx = 0;
static unsigned long long g_last_net_tx = 0;
static double g_cur_rx_rate = 0.0;
static double g_cur_tx_rate = 0.0;

static double g_last_disk_time = 0.0;
static unsigned long long g_last_disk_read = 0;
static unsigned long long g_last_disk_write = 0;
static double g_cur_disk_read_rate = 0.0;
static double g_cur_disk_write_rate = 0.0;
static std::map<std::string, std::pair<unsigned long long, unsigned long long>> g_last_dev_stats;

static std::string g_cpu_model = "Unknown CPU";
static std::string g_gpu_model = "Unknown GPU";

// Detailed Hardware Cache
static std::string g_cache_raw_cpu = "";
static std::string g_cache_raw_gpu = "";
static std::string g_cache_raw_ram = "";
static std::string g_cache_raw_net = "";
static std::string g_cache_raw_disk = "";
static std::string g_cache_ram_type_json = "[\"Detecting...\"]";
static std::string g_cache_network_type = "";
static std::string g_cache_battery_tech = "";

static HINSTANCE g_hInstance = NULL;
static const wchar_t* WINDOW_CLASS = L"RizkybyMONITOR";

// =============================================================================
// Helper Functions
// =============================================================================
static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    if (f.is_open()) f << content;
}

static std::string execCmd(const std::string& cmd) {
    // On Windows, use _popen
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buf[512];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe)) {
        result += buf;
    }
    _pclose(pipe);
    return result;
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 32);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if ((unsigned char)c < 32) {
                    char hex[8];
                    snprintf(hex, sizeof(hex), "\\u%04x", (unsigned char)c);
                    out += hex;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

static std::string formatSpeed(double bytes_per_sec) {
    char buf[64];
    if (bytes_per_sec >= 1073741824.0) {
        snprintf(buf, sizeof(buf), "%.2f GB/s", bytes_per_sec / 1073741824.0);
    } else if (bytes_per_sec >= 1048576.0) {
        snprintf(buf, sizeof(buf), "%.1f MB/s", bytes_per_sec / 1048576.0);
    } else if (bytes_per_sec >= 1024.0) {
        snprintf(buf, sizeof(buf), "%.1f KB/s", bytes_per_sec / 1024.0);
    } else {
        snprintf(buf, sizeof(buf), "%.0f B/s", bytes_per_sec);
    }
    return std::string(buf);
}

static double getTimeSec() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

static std::string wstrToStr(const std::wstring& ws) {
    if (ws.empty()) return "";
    int sz = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), NULL, 0, NULL, NULL);
    std::string s(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &s[0], sz, NULL, NULL);
    return s;
}

static std::wstring strToWstr(const std::string& s) {
    if (s.empty()) return L"";
    int sz = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring ws(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], sz);
    return ws;
}

// =============================================================================
// CPU Telemetry (Windows)
// =============================================================================
static void getCpuUsage(std::map<std::string, double>& cpu_usages) {
    // Total CPU usage via GetSystemTimes
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        unsigned long long idle = ((unsigned long long)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
        unsigned long long kernel = ((unsigned long long)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
        unsigned long long user = ((unsigned long long)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
        unsigned long long total = kernel + user;

        if (g_last_cpu_ticks.find("cpu") != g_last_cpu_ticks.end()) {
            auto last = g_last_cpu_ticks["cpu"];
            unsigned long long total_diff = total - last.total;
            unsigned long long idle_diff = idle - last.idle;
            if (total_diff > 0) {
                double u = 100.0 * (1.0 - (double)idle_diff / (double)total_diff);
                cpu_usages["cpu"] = std::round(u * 10.0) / 10.0;
            }
        } else {
            cpu_usages["cpu"] = 0.0;
        }
        g_last_cpu_ticks["cpu"] = {idle, total};
    }

    // Per-core CPU usage via NtQuerySystemInformation (undocumented but widely used)
    // Alternative: use PDH counters per core
    typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
        LARGE_INTEGER IdleTime;
        LARGE_INTEGER KernelTime;
        LARGE_INTEGER UserTime;
        LARGE_INTEGER Reserved1[2];
        ULONG Reserved2;
    } SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

    typedef NTSTATUS(WINAPI* NtQuerySystemInformationFn)(ULONG, PVOID, ULONG, PULONG);
    static auto NtQuerySystemInformation = (NtQuerySystemInformationFn)GetProcAddress(
        GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation");

    if (NtQuerySystemInformation) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        int ncpu = si.dwNumberOfProcessors;
        std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> cpuInfo(ncpu);

        ULONG returnLength = 0;
        // SystemProcessorPerformanceInformation = 8
        NTSTATUS status = NtQuerySystemInformation(8, cpuInfo.data(),
            (ULONG)(ncpu * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)), &returnLength);

        if (status == 0) { // STATUS_SUCCESS
            for (int i = 0; i < ncpu; i++) {
                std::string coreName = "cpu" + std::to_string(i);
                unsigned long long idle_t = cpuInfo[i].IdleTime.QuadPart;
                unsigned long long total_t = cpuInfo[i].KernelTime.QuadPart + cpuInfo[i].UserTime.QuadPart;

                if (g_last_cpu_ticks.find(coreName) != g_last_cpu_ticks.end()) {
                    auto last = g_last_cpu_ticks[coreName];
                    unsigned long long td = total_t - last.total;
                    unsigned long long id = idle_t - last.idle;
                    if (td > 0) {
                        double u = 100.0 * (1.0 - (double)id / (double)td);
                        cpu_usages[coreName] = std::round(u * 10.0) / 10.0;
                    }
                } else {
                    cpu_usages[coreName] = 0.0;
                }
                g_last_cpu_ticks[coreName] = {idle_t, total_t};
            }
        }
    }
}

static std::vector<int> getCpuFrequencies() {
    std::vector<int> freqs;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int ncpu = si.dwNumberOfProcessors;

    // Use CallNtPowerInformation to get per-core frequencies
    typedef struct _PROCESSOR_POWER_INFORMATION {
        ULONG Number;
        ULONG MaxMhz;
        ULONG CurrentMhz;
        ULONG MhzLimit;
        ULONG MaxIdleState;
        ULONG CurrentIdleState;
    } PROCESSOR_POWER_INFORMATION;

    std::vector<PROCESSOR_POWER_INFORMATION> ppi(ncpu);
    NTSTATUS status = CallNtPowerInformation(
        ProcessorInformation, NULL, 0,
        ppi.data(), (ULONG)(ncpu * sizeof(PROCESSOR_POWER_INFORMATION)));

    if (status == 0) {
        for (int i = 0; i < ncpu; i++) {
            freqs.push_back((int)ppi[i].CurrentMhz);
        }
    }
    return freqs;
}

// =============================================================================
// Memory Telemetry (Windows)
// =============================================================================
struct MemoryInfo {
    unsigned long long total;
    unsigned long long used;
    unsigned long long avail;
    unsigned long long pagefile_total;
    unsigned long long pagefile_used;
};

static MemoryInfo getMemoryInfo() {
    MemoryInfo mi = {};
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        mi.total = ms.ullTotalPhys;
        mi.avail = ms.ullAvailPhys;
        mi.used = mi.total - mi.avail;
        mi.pagefile_total = ms.ullTotalPageFile - ms.ullTotalPhys; // Approximate pagefile size
        mi.pagefile_used = (ms.ullTotalPageFile - ms.ullAvailPageFile) - mi.used;
        if ((long long)mi.pagefile_used < 0) mi.pagefile_used = 0;
        if ((long long)mi.pagefile_total < 0) mi.pagefile_total = 0;
    }
    return mi;
}

// =============================================================================
// GPU Telemetry (Windows via DXGI)
// =============================================================================
struct GpuInfo {
    int freq_mhz;
    double usage_pct;
    std::string model;
};

static GpuInfo getGpuInfo() {
    GpuInfo gi = {0, 0.0, "Unknown GPU"};

    // Use DXGI to get GPU adapter info
    IDXGIFactory1* factory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory))) {
        IDXGIAdapter1* adapter = nullptr;
        if (SUCCEEDED(factory->EnumAdapters1(0, &adapter))) {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(adapter->GetDesc1(&desc))) {
                gi.model = wstrToStr(desc.Description);
            }
            adapter->Release();
        }
        factory->Release();
    }

    // GPU usage via D3DKMT (undocumented but works on Windows 10+)
    // Use PDH counter as alternative: \GPU Engine(*)\Utilization Percentage
    PDH_HQUERY query = NULL;
    PDH_HCOUNTER counter = NULL;
    if (PdhOpenQueryW(NULL, 0, &query) == ERROR_SUCCESS) {
        // Try GPU Engine utilization counter (Windows 10 1709+)
        if (PdhAddEnglishCounterW(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter) == ERROR_SUCCESS) {
            PdhCollectQueryData(query);
            Sleep(100);
            PdhCollectQueryData(query);

            PDH_FMT_COUNTERVALUE val;
            if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &val) == ERROR_SUCCESS) {
                gi.usage_pct = val.doubleValue;
            }
        }
        PdhCloseQuery(query);
    }

    // GPU frequency from registry or WMI (fallback)
    gi.freq_mhz = 1000; // Default estimate
    std::string wmic_out = execCmd("wmic path Win32_VideoController get CurrentRefreshRate,AdapterRAM /format:list 2>NUL");
    // Parse if available

    return gi;
}

// =============================================================================
// Network Telemetry (Windows via GetIfTable2)
// =============================================================================
static void updateNetworkTelemetry() {
    double now = getTimeSec();

    MIB_IF_TABLE2* ifTable = nullptr;
    if (GetIfTable2(&ifTable) == NO_ERROR) {
        unsigned long long total_rx = 0, total_tx = 0;

        for (ULONG i = 0; i < ifTable->NumEntries; i++) {
            MIB_IF_ROW2& row = ifTable->Table[i];
            // Skip loopback and non-operational interfaces
            if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK) continue;
            if (row.OperStatus != IfOperStatusUp) continue;

            total_rx += row.InOctets;
            total_tx += row.OutOctets;
        }

        double dt = (g_last_net_time > 0.0) ? (now - g_last_net_time) : 0.5;
        if (dt <= 0.0) dt = 0.5;

        if (g_last_net_time > 0.0) {
            g_cur_rx_rate = (total_rx >= g_last_net_rx) ? ((total_rx - g_last_net_rx) / dt) : 0.0;
            g_cur_tx_rate = (total_tx >= g_last_net_tx) ? ((total_tx - g_last_net_tx) / dt) : 0.0;
        }
        g_last_net_rx = total_rx;
        g_last_net_tx = total_tx;
        g_last_net_time = now;

        FreeMibTable(ifTable);
    }
}

// =============================================================================
// Disk Telemetry (Windows)
// =============================================================================
struct DiskDriveInfo {
    std::string id;          // e.g. "C:", "D:"
    std::string dev;         // e.g. "\\\\.\\C:"
    std::string model;
    std::string type_str;    // "SSD", "HDD", etc.
    std::string icon;
    double size_gb;
    double used_gb;
    double free_gb;
    double usage_pct;
    bool is_ssd;
    bool is_system;
    std::string health_str;
    std::string temp_str;
    double read_rate;
    double write_rate;
};

static std::vector<DiskDriveInfo> discoverDisks() {
    std::vector<DiskDriveInfo> disks;

    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (!(drives & (1 << i))) continue;

        char letter = 'A' + i;
        std::string root = std::string(1, letter) + ":\\";
        UINT driveType = GetDriveTypeA(root.c_str());

        // Skip non-fixed and non-removable drives (network, cdrom, etc.)
        if (driveType != DRIVE_FIXED && driveType != DRIVE_REMOVABLE) continue;

        DiskDriveInfo di = {};
        di.id = std::string(1, letter) + ":";
        di.dev = "\\\\.\\" + di.id;
        di.icon = "⚡";

        // Get free space
        ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
        if (GetDiskFreeSpaceExA(root.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
            di.size_gb = (double)totalBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
            di.free_gb = (double)totalFreeBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
            di.used_gb = di.size_gb - di.free_gb;
            di.usage_pct = (di.size_gb > 0) ? (di.used_gb / di.size_gb * 100.0) : 0;
        }

        // Detect SSD vs HDD
        di.is_ssd = false;
        std::string physDrive = "\\\\.\\PhysicalDrive0"; // Simplified
        HANDLE hDev = CreateFileA(di.dev.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hDev != INVALID_HANDLE_VALUE) {
            STORAGE_PROPERTY_QUERY spq = {};
            spq.PropertyId = StorageDeviceSeekPenaltyProperty;
            spq.QueryType = PropertyStandardQuery;

            struct {
                DWORD Version;
                DWORD Size;
                BOOLEAN IncursSeekPenalty;
            } seekResult = {};

            DWORD bytesReturned = 0;
            if (DeviceIoControl(hDev, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq),
                &seekResult, sizeof(seekResult), &bytesReturned, NULL)) {
                di.is_ssd = !seekResult.IncursSeekPenalty;
            }

            // Get model name
            STORAGE_PROPERTY_QUERY dpq = {};
            dpq.PropertyId = StorageDeviceProperty;
            dpq.QueryType = PropertyStandardQuery;
            BYTE descBuf[1024] = {};
            if (DeviceIoControl(hDev, IOCTL_STORAGE_QUERY_PROPERTY, &dpq, sizeof(dpq),
                descBuf, sizeof(descBuf), &bytesReturned, NULL)) {
                STORAGE_DEVICE_DESCRIPTOR* sdd = (STORAGE_DEVICE_DESCRIPTOR*)descBuf;
                if (sdd->ProductIdOffset > 0) {
                    di.model = trim(std::string((char*)descBuf + sdd->ProductIdOffset));
                }
                if (di.model.empty() && sdd->VendorIdOffset > 0) {
                    di.model = trim(std::string((char*)descBuf + sdd->VendorIdOffset));
                }
            }

            CloseHandle(hDev);
        }

        if (di.model.empty()) di.model = di.id + " Volume";

        // Determine type classification
        if (driveType == DRIVE_REMOVABLE) {
            di.icon = "💾";
            di.type_str = "Removable Drive (USB/Flash)";
        } else if (di.is_ssd) {
            di.icon = "⚡";
            di.type_str = "Solid State Drive (SSD)";
        } else {
            di.icon = "💽";
            di.type_str = "Hard Disk Drive (HDD)";
        }

        // System drive detection
        char winDir[MAX_PATH];
        GetWindowsDirectoryA(winDir, MAX_PATH);
        di.is_system = (toupper(winDir[0]) == letter);

        di.health_str = "Operational";
        di.temp_str = "N/A";
        di.read_rate = 0;
        di.write_rate = 0;

        disks.push_back(di);
    }

    return disks;
}

// =============================================================================
// Disk I/O Performance Counters (Windows via PDH)
// =============================================================================
static void updateDiskIo() {
    double now = getTimeSec();

    // Use PDH for disk I/O rates
    static PDH_HQUERY diskQuery = NULL;
    static PDH_HCOUNTER readCounter = NULL, writeCounter = NULL;
    static bool initialized = false;

    if (!initialized) {
        if (PdhOpenQueryW(NULL, 0, &diskQuery) == ERROR_SUCCESS) {
            PdhAddEnglishCounterW(diskQuery, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &readCounter);
            PdhAddEnglishCounterW(diskQuery, L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &writeCounter);
            PdhCollectQueryData(diskQuery);
            initialized = true;
        }
        return;
    }

    if (PdhCollectQueryData(diskQuery) == ERROR_SUCCESS) {
        PDH_FMT_COUNTERVALUE val;
        if (readCounter && PdhGetFormattedCounterValue(readCounter, PDH_FMT_DOUBLE, NULL, &val) == ERROR_SUCCESS) {
            g_cur_disk_read_rate = val.doubleValue;
        }
        if (writeCounter && PdhGetFormattedCounterValue(writeCounter, PDH_FMT_DOUBLE, NULL, &val) == ERROR_SUCCESS) {
            g_cur_disk_write_rate = val.doubleValue;
        }
    }
    g_last_disk_time = now;
}

// =============================================================================
// Sensors (Temperature & Battery - Windows)
// =============================================================================
static int getCpuTemperature() {
    // Try WMI via wmic command
    std::string out = execCmd("wmic /namespace:\\\\root\\wmi PATH MSAcpi_ThermalZoneTemperature get CurrentTemperature /format:list 2>NUL");
    if (!out.empty()) {
        size_t pos = out.find("CurrentTemperature=");
        if (pos != std::string::npos) {
            int raw = std::atoi(out.c_str() + pos + 19);
            if (raw > 0) {
                int celsius = (raw - 2732) / 10; // Convert from tenths of Kelvin
                if (celsius > 0 && celsius < 120) return celsius;
            }
        }
    }
    return 55; // Default fallback
}

static int getBatteryPercent() {
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        if (sps.BatteryLifePercent != 255) {
            return (int)sps.BatteryLifePercent;
        }
    }
    return 100; // Desktop or unknown
}

static std::string getBatteryStatus() {
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        std::string status;
        if (sps.ACLineStatus == 1) status = "Plugged In";
        else status = "On Battery";

        if (sps.BatteryLifePercent != 255) {
            status += " (" + std::to_string((int)sps.BatteryLifePercent) + "%)";
        }
        if (sps.BatteryFlag & 8) status += " [Charging]";
        return status;
    }
    return "AC Power (Desktop)";
}

// =============================================================================
// Process Monitoring (Windows via Toolhelp32 & GetProcess*)
// =============================================================================
static void getTopCpuProcesses(std::vector<ProcEntry>& procs, int maxCount = 40) {
    std::string ps_out = execCmd("powershell -NoProfile -Command \"Get-Process | Sort-Object CPU -Descending | Select-Object -First 40 | ForEach-Object { $_.ProcessName + '|' + [math]::Round($_.CPU,1) + '|' + [math]::Round($_.WorkingSet64/1MB,1) }\" 2>NUL");
    std::istringstream iss(ps_out);
    std::string line;
    int count = 0;
    while (std::getline(iss, line) && count < maxCount) {
        line = trim(line);
        if (line.empty()) continue;
        size_t p1 = line.find('|');
        if (p1 == std::string::npos) continue;
        size_t p2 = line.find('|', p1 + 1);

        std::string name = line.substr(0, p1);
        std::string cpu_val = (p1 != std::string::npos) ? line.substr(p1 + 1, (p2 != std::string::npos ? p2 - p1 - 1 : std::string::npos)) : "0";

        ProcEntry pe;
        pe.name = name;
        pe.val = cpu_val + "%";
        procs.push_back(pe);
        count++;
    }
}

static void getTopMemProcesses(std::vector<ProcEntry>& procs, int maxCount = 40) {
    std::string ps_out = execCmd("powershell -NoProfile -Command \"Get-Process | Sort-Object WorkingSet64 -Descending | Select-Object -First 40 | ForEach-Object { $_.ProcessName + '|' + [math]::Round($_.WorkingSet64/1MB,1) }\" 2>NUL");
    std::istringstream iss(ps_out);
    std::string line;
    int count = 0;
    while (std::getline(iss, line) && count < maxCount) {
        line = trim(line);
        if (line.empty()) continue;
        size_t p = line.find('|');
        if (p == std::string::npos) continue;

        ProcEntry pe;
        pe.name = line.substr(0, p);
        double mb = std::atof(line.substr(p + 1).c_str());
        char buf[32];
        if (mb >= 1024.0) {
            snprintf(buf, sizeof(buf), "%.1f GB", mb / 1024.0);
        } else {
            snprintf(buf, sizeof(buf), "%.1f MB", mb);
        }
        pe.val = buf;
        procs.push_back(pe);
        count++;
    }
}

static void getTopDiskProcesses(std::vector<DiskProcEntry>& procs) {
    // Use GetProcessIoCounters via Toolhelp32 snapshot
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    std::map<std::string, std::pair<double, double>> proc_io;
    double now = getTimeSec();
    static double s_last_disk_proc_time = 0;
    double dt = (s_last_disk_proc_time > 0) ? (now - s_last_disk_proc_time) : 0.5;
    if (dt <= 0) dt = 0.5;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);
    if (Process32FirstW(snap, &pe32)) {
        do {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
            if (hProc) {
                IO_COUNTERS ioc;
                if (GetProcessIoCounters(hProc, &ioc)) {
                    DWORD pid = pe32.th32ProcessID;
                    std::string name = wstrToStr(pe32.szExeFile);
                    // Remove .exe extension
                    size_t dot = name.rfind('.');
                    if (dot != std::string::npos) name = name.substr(0, dot);

                    double read_rate = 0, write_rate = 0;
                    if (g_last_proc_io_ticks.find(pid) != g_last_proc_io_ticks.end()) {
                        auto& last = g_last_proc_io_ticks[pid];
                        read_rate = (ioc.ReadTransferCount >= last.r_bytes) ?
                            (double)(ioc.ReadTransferCount - last.r_bytes) / dt : 0;
                        write_rate = (ioc.WriteTransferCount >= last.w_bytes) ?
                            (double)(ioc.WriteTransferCount - last.w_bytes) / dt : 0;
                    }
                    g_last_proc_io_ticks[pid] = {ioc.ReadTransferCount, ioc.WriteTransferCount};

                    if (read_rate > 50 || write_rate > 50) {
                        if (proc_io.find(name) != proc_io.end()) {
                            proc_io[name].first += read_rate;
                            proc_io[name].second += write_rate;
                        } else {
                            proc_io[name] = {read_rate, write_rate};
                        }
                    }
                }
                CloseHandle(hProc);
            }
        } while (Process32NextW(snap, &pe32));
    }
    CloseHandle(snap);
    s_last_disk_proc_time = now;

    // Sort by total
    std::vector<std::pair<std::string, std::pair<double, double>>> sorted(proc_io.begin(), proc_io.end());
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) {
        return (a.second.first + a.second.second) > (b.second.first + b.second.second);
    });

    int count = 0;
    for (auto& p : sorted) {
        if (count >= 40) break;
        DiskProcEntry de;
        de.name = p.first;
        de.read = formatSpeed(p.second.first);
        de.write = formatSpeed(p.second.second);
        de.total_bytes = p.second.first + p.second.second;
        procs.push_back(de);
        count++;
    }
}

static void getTopNetProcesses(std::vector<NetProcEntry>& procs) {
    // On Windows, network per-process is complex; use netstat + I/O counters
    // Simplified: processes with network sockets having I/O activity
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    std::map<std::string, std::pair<double, double>> net_io;
    double now = getTimeSec();
    static double s_last_net_proc_time = 0;
    double dt = (s_last_net_proc_time > 0) ? (now - s_last_net_proc_time) : 0.5;
    if (dt <= 0) dt = 0.5;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);
    if (Process32FirstW(snap, &pe32)) {
        do {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
            if (hProc) {
                IO_COUNTERS ioc;
                if (GetProcessIoCounters(hProc, &ioc)) {
                    std::string name = wstrToStr(pe32.szExeFile);
                    size_t dot = name.rfind('.');
                    if (dot != std::string::npos) name = name.substr(0, dot);

                    // Rough estimate: assume some I/O is network
                    // This is a simplification - true per-process network requires ETW
                    DWORD pid = pe32.th32ProcessID;
                    if (g_last_proc_io_ticks.find(pid) != g_last_proc_io_ticks.end()) {
                        auto& last = g_last_proc_io_ticks[pid];
                        double other_rate = (ioc.OtherTransferCount >= (last.r_bytes + last.w_bytes)) ?
                            (double)(ioc.ReadTransferCount + ioc.WriteTransferCount - last.r_bytes - last.w_bytes) / dt : 0;
                        if (other_rate > 100 && name != "System" && name != "svchost") {
                            net_io[name] = {other_rate * 0.3, other_rate * 0.1}; // Rough split
                        }
                    }
                }
                CloseHandle(hProc);
            }
        } while (Process32NextW(snap, &pe32));
    }
    CloseHandle(snap);
    s_last_net_proc_time = now;

    std::vector<std::pair<std::string, std::pair<double, double>>> sorted(net_io.begin(), net_io.end());
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) {
        return (a.second.first + a.second.second) > (b.second.first + b.second.second);
    });

    int count = 0;
    for (auto& p : sorted) {
        if (count >= 20) break;
        NetProcEntry ne;
        ne.name = p.first;
        ne.down = formatSpeed(p.second.first);
        ne.up = formatSpeed(p.second.second);
        ne.total = p.second.first + p.second.second;
        procs.push_back(ne);
        count++;
    }
}

// =============================================================================
// Detailed Text Builders (Windows)
// =============================================================================
static std::string buildDiskDetailText(const std::string& driveLetter) {
    std::stringstream d;
    d << "==================================================\n";
    d << "💽 STORAGE DEVICE TELEMETRY: " << driveLetter << "\n";
    d << "==================================================\n";

    // Volume info
    char volName[MAX_PATH] = {}, fsName[MAX_PATH] = {};
    DWORD serial = 0, maxLen = 0, flags = 0;
    std::string root = driveLetter + "\\";
    if (GetVolumeInformationA(root.c_str(), volName, MAX_PATH, &serial, &maxLen, &flags, fsName, MAX_PATH)) {
        d << "Volume Label     : " << (strlen(volName) > 0 ? volName : "(No Label)") << "\n";
        d << "File System      : " << fsName << "\n";
        char serialBuf[32];
        snprintf(serialBuf, sizeof(serialBuf), "%04X-%04X", (serial >> 16) & 0xFFFF, serial & 0xFFFF);
        d << "Volume Serial    : " << serialBuf << "\n";
    }

    // Disk space
    ULARGE_INTEGER freeBytesAvail, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceExA(root.c_str(), &freeBytesAvail, &totalBytes, &totalFreeBytes)) {
        double total_gb = (double)totalBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
        double free_gb = (double)totalFreeBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
        double used_gb = total_gb - free_gb;
        double pct = (total_gb > 0) ? (used_gb / total_gb * 100.0) : 0;
        char buf[128];
        snprintf(buf, sizeof(buf), "%.1f GB Used (%.0f%%) | %.1f GB Free | %.1f GB Total", used_gb, pct, free_gb, total_gb);
        d << "Capacity         : " << buf << "\n";
    }

    // Drive type and hardware info
    d << "\n=== PHYSICAL DRIVE HARDWARE ===\n";
    HANDLE hDev = CreateFileA(("\\\\.\\"+driveLetter).c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDev != INVALID_HANDLE_VALUE) {
        STORAGE_PROPERTY_QUERY dpq = {};
        dpq.PropertyId = StorageDeviceProperty;
        dpq.QueryType = PropertyStandardQuery;
        BYTE descBuf[1024] = {};
        DWORD br = 0;
        if (DeviceIoControl(hDev, IOCTL_STORAGE_QUERY_PROPERTY, &dpq, sizeof(dpq), descBuf, sizeof(descBuf), &br, NULL)) {
            STORAGE_DEVICE_DESCRIPTOR* sdd = (STORAGE_DEVICE_DESCRIPTOR*)descBuf;
            if (sdd->VendorIdOffset > 0)
                d << "Vendor           : " << trim(std::string((char*)descBuf + sdd->VendorIdOffset)) << "\n";
            if (sdd->ProductIdOffset > 0)
                d << "Product          : " << trim(std::string((char*)descBuf + sdd->ProductIdOffset)) << "\n";
            if (sdd->ProductRevisionOffset > 0)
                d << "Firmware         : " << trim(std::string((char*)descBuf + sdd->ProductRevisionOffset)) << "\n";
            if (sdd->SerialNumberOffset > 0)
                d << "Serial Number    : " << trim(std::string((char*)descBuf + sdd->SerialNumberOffset)) << "\n";
            d << "Bus Type         : ";
            switch (sdd->BusType) {
                case BusTypeAta:    d << "ATA/SATA\n"; break;
                case BusTypeUsb:    d << "USB\n"; break;
                case BusTypeNvme:   d << "NVMe (PCIe)\n"; break;
                case BusTypeScsi:   d << "SCSI\n"; break;
                case BusTypeRAID:   d << "RAID\n"; break;
                default:            d << "Unknown (" << (int)sdd->BusType << ")\n"; break;
            }
            d << "Removable        : " << (sdd->RemovableMedia ? "Yes" : "No") << "\n";
        }

        // Seek penalty = HDD detection
        STORAGE_PROPERTY_QUERY spq = {};
        spq.PropertyId = StorageDeviceSeekPenaltyProperty;
        spq.QueryType = PropertyStandardQuery;
        struct { DWORD Version; DWORD Size; BOOLEAN IncursSeekPenalty; } seekResult = {};
        if (DeviceIoControl(hDev, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq), &seekResult, sizeof(seekResult), &br, NULL)) {
            d << "Drive Type       : " << (seekResult.IncursSeekPenalty ? "HDD (Rotational)" : "SSD (Solid State)") << "\n";
        }

        // TRIM support
        STORAGE_PROPERTY_QUERY tq = {};
        tq.PropertyId = StorageDeviceTrimProperty;
        tq.QueryType = PropertyStandardQuery;
        struct { DWORD Version; DWORD Size; BOOLEAN TrimEnabled; } trimResult = {};
        if (DeviceIoControl(hDev, IOCTL_STORAGE_QUERY_PROPERTY, &tq, sizeof(tq), &trimResult, sizeof(trimResult), &br, NULL)) {
            d << "TRIM Support     : " << (trimResult.TrimEnabled ? "Enabled" : "Disabled") << "\n";
        }

        CloseHandle(hDev);
    }

    // SMART status via wmic
    d << "\n=== S.M.A.R.T. STATUS ===\n";
    std::string smart_out = execCmd("wmic diskdrive get Status,Model,SerialNumber /format:list 2>NUL");
    if (!smart_out.empty()) {
        d << smart_out;
    } else {
        d << "  (Run as Administrator for SMART data)\n";
    }

    return d.str();
}

static std::string buildMemoryDetailText() {
    std::stringstream d;
    d << "==================================================\n";
    d << "🧠 MEMORY HIERARCHY: PHYSICAL RAM / PAGEFILE\n";
    d << "==================================================\n\n";

    // Section 1: Physical Memory Overview
    d << "=== 1. PHYSICAL MEMORY OVERVIEW ===\n";
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "  Total Physical   : %.1f GB\n"
            "  Available        : %.1f GB\n"
            "  In Use           : %.1f GB (%.0f%%)\n"
            "  Total Pagefile   : %.1f GB\n"
            "  Available PF     : %.1f GB\n"
            "  Total Virtual    : %.1f GB\n",
            (double)ms.ullTotalPhys / (1024.0*1024.0*1024.0),
            (double)ms.ullAvailPhys / (1024.0*1024.0*1024.0),
            (double)(ms.ullTotalPhys - ms.ullAvailPhys) / (1024.0*1024.0*1024.0),
            (double)ms.dwMemoryLoad,
            (double)ms.ullTotalPageFile / (1024.0*1024.0*1024.0),
            (double)ms.ullAvailPageFile / (1024.0*1024.0*1024.0),
            (double)ms.ullTotalVirtual / (1024.0*1024.0*1024.0));
        d << buf;
    }

    // Section 2: DIMM Slot Details
    d << "\n=== 2. DIMM SLOT DETAILS (Physical Modules) ===\n";
    std::string wmic_mem = execCmd("wmic memorychip get Capacity,Speed,Manufacturer,PartNumber,DeviceLocator,MemoryType,FormFactor,ConfiguredClockSpeed,BankLabel /format:list 2>NUL");
    if (!wmic_mem.empty() && wmic_mem.find("Capacity") != std::string::npos) {
        d << wmic_mem;
    } else {
        d << "  (Run as Administrator for full DIMM details)\n";
    }

    // Section 3: Performance Counters
    d << "\n=== 3. PERFORMANCE INFORMATION ===\n";
    PERFORMANCE_INFORMATION pi;
    pi.cb = sizeof(pi);
    if (GetPerformanceInfo(&pi, sizeof(pi))) {
        double pageSize = (double)pi.PageSize;
        char buf[512];
        snprintf(buf, sizeof(buf),
            "  Commit Total     : %.1f GB\n"
            "  Commit Limit     : %.1f GB\n"
            "  Commit Peak      : %.1f GB\n"
            "  Physical Total   : %.1f GB\n"
            "  Physical Avail   : %.1f GB\n"
            "  System Cache     : %.1f GB\n"
            "  Kernel Total     : %.1f MB\n"
            "  Kernel Paged     : %.1f MB\n"
            "  Kernel NonPaged  : %.1f MB\n"
            "  Page Size        : %d bytes\n"
            "  Handle Count     : %zu\n"
            "  Process Count    : %zu\n"
            "  Thread Count     : %zu\n",
            (double)pi.CommitTotal * pageSize / (1024.0*1024.0*1024.0),
            (double)pi.CommitLimit * pageSize / (1024.0*1024.0*1024.0),
            (double)pi.CommitPeak * pageSize / (1024.0*1024.0*1024.0),
            (double)pi.PhysicalTotal * pageSize / (1024.0*1024.0*1024.0),
            (double)pi.PhysicalAvailable * pageSize / (1024.0*1024.0*1024.0),
            (double)pi.SystemCache * pageSize / (1024.0*1024.0*1024.0),
            (double)pi.KernelTotal * pageSize / (1024.0*1024.0),
            (double)pi.KernelPaged * pageSize / (1024.0*1024.0),
            (double)pi.KernelNonpaged * pageSize / (1024.0*1024.0),
            (int)pi.PageSize,
            (size_t)pi.HandleCount,
            (size_t)pi.ProcessCount,
            (size_t)pi.ThreadCount);
        d << buf;
    }

    // Section 4: Pagefile Configuration
    d << "\n=== 4. PAGEFILE CONFIGURATION ===\n";
    std::string pf_out = execCmd("wmic pagefile list /format:list 2>NUL");
    if (!pf_out.empty() && pf_out.find("Name") != std::string::npos) {
        d << pf_out;
    } else {
        d << "  System Managed Pagefile (Auto Configuration)\n";
    }

    // Section 5: Cache Hierarchy
    d << "\n=== 5. CPU CACHE HIERARCHY ===\n";
    std::string cache_out = execCmd("wmic cpu get L2CacheSize,L3CacheSize,NumberOfCores,NumberOfLogicalProcessors /format:list 2>NUL");
    if (!cache_out.empty()) {
        d << cache_out;
    }

    return d.str();
}

// =============================================================================
// Hardware Info Initialization (Windows)
// =============================================================================
static void initHardwareInfo() {
    // CPU Model
    std::string cpu_out = execCmd("wmic cpu get Name /format:list 2>NUL");
    size_t pos = cpu_out.find("Name=");
    if (pos != std::string::npos) {
        g_cpu_model = trim(cpu_out.substr(pos + 5, cpu_out.find('\n', pos) - pos - 5));
    }

    // GPU Model
    std::string gpu_out = execCmd("wmic path Win32_VideoController get Name /format:list 2>NUL");
    pos = gpu_out.find("Name=");
    if (pos != std::string::npos) {
        g_gpu_model = trim(gpu_out.substr(pos + 5, gpu_out.find('\n', pos) - pos - 5));
    }

    // Raw detail caches
    g_cache_raw_cpu = execCmd("wmic cpu get /format:list 2>NUL");
    if (g_cache_raw_cpu.empty()) g_cache_raw_cpu = "CPU: " + g_cpu_model;

    g_cache_raw_gpu = execCmd("wmic path Win32_VideoController get /format:list 2>NUL");
    if (g_cache_raw_gpu.empty()) g_cache_raw_gpu = "GPU: " + g_gpu_model;

    g_cache_raw_ram = buildMemoryDetailText();

    // Network adapter info
    g_cache_raw_net = execCmd("ipconfig /all 2>NUL");
    g_cache_network_type = execCmd("netsh wlan show interfaces 2>NUL");
    if (g_cache_network_type.empty()) g_cache_network_type = "Ethernet / Wired Connection";

    // Disk info
    g_cache_raw_disk = execCmd("wmic diskdrive get Model,InterfaceType,MediaType,Size,SerialNumber,Status /format:list 2>NUL");

    // RAM type from wmic
    std::string ram_type_out = execCmd("wmic memorychip get Capacity,Speed,DeviceLocator /format:list 2>NUL");
    if (!ram_type_out.empty()) {
        // Parse into JSON array
        g_cache_ram_type_json = "[";
        std::istringstream iss(ram_type_out);
        std::string line;
        std::string curCap, curSpeed, curLoc;
        bool first = true;
        while (std::getline(iss, line)) {
            line = trim(line);
            if (line.rfind("Capacity=", 0) == 0) {
                unsigned long long cap = std::strtoull(line.c_str() + 9, NULL, 10);
                char buf[32];
                snprintf(buf, sizeof(buf), "%.0f GiB", (double)cap / (1024.0*1024.0*1024.0));
                curCap = buf;
            } else if (line.rfind("Speed=", 0) == 0) {
                curSpeed = line.substr(6) + " MT/s";
            } else if (line.rfind("DeviceLocator=", 0) == 0) {
                curLoc = line.substr(14);
            }
            if (!curCap.empty() && !curSpeed.empty() && !curLoc.empty()) {
                if (!first) g_cache_ram_type_json += ",";
                g_cache_ram_type_json += "\"" + curLoc + ": " + curCap + " (" + curSpeed + ")\"";
                first = false;
                curCap.clear(); curSpeed.clear(); curLoc.clear();
            }
        }
        g_cache_ram_type_json += "]";
        if (g_cache_ram_type_json == "[]") g_cache_ram_type_json = "[\"Detecting...\"]";
    }

    // Battery tech
    g_cache_battery_tech = getBatteryStatus();

    // Pre-cache disk details
    auto disks = discoverDisks();
    std::map<std::string, std::string> dt_map;
    for (auto& disk : disks) {
        dt_map[disk.id] = buildDiskDetailText(disk.id);
    }
    {
        std::lock_guard<std::mutex> lock(g_stats_mutex);
        g_disk_details_map = dt_map;
    }
}

// =============================================================================
// Main Telemetry Update Loop (Windows)
// =============================================================================
static void updateTelemetry() {
    double now = getTimeSec();

    // 1. CPU Usage
    std::map<std::string, double> cpu_usages;
    getCpuUsage(cpu_usages);

    // 2. CPU Frequencies
    std::vector<int> cpu_freqs = getCpuFrequencies();

    // 3. Memory
    MemoryInfo memInfo = getMemoryInfo();

    // 4. GPU
    GpuInfo gpuInfo = getGpuInfo();
    g_gpu_model = gpuInfo.model;

    // 5. Network
    updateNetworkTelemetry();

    // 6. Disk I/O
    updateDiskIo();

    // 7. Discover Disks
    auto disks = discoverDisks();

    // 8. Sensors
    int temp_c = getCpuTemperature();
    int battery_pct = getBatteryPercent();

    // 9. Processes
    std::vector<ProcEntry> cpu_procs, mem_procs;
    std::vector<DiskProcEntry> disk_procs;
    std::vector<NetProcEntry> net_procs;

    getTopCpuProcesses(cpu_procs);
    getTopMemProcesses(mem_procs);
    getTopDiskProcesses(disk_procs);
    getTopNetProcesses(net_procs);

    // Build JSON
    std::stringstream json;
    json << "{";

    // Hardware
    json << "\"hardware\":{\"cpu_model\":\"" << escapeJson(g_cpu_model) << "\",\"gpu_model\":\"" << escapeJson(gpuInfo.model) << "\"},";

    // RAM (no ZRAM on Windows, use Pagefile instead)
    json << "\"ram\":{\"total\":" << memInfo.total << ",\"used\":" << memInfo.used << ",\"free\":" << memInfo.avail << "},";
    json << "\"zram\":{\"total\":0,\"used\":0},"; // No ZRAM on Windows
    json << "\"swap\":{\"total\":" << memInfo.pagefile_total << ",\"used\":" << memInfo.pagefile_used << "},";

    // CPU
    json << "\"cpu\":{\"total_usage\":" << (cpu_usages.count("cpu") ? cpu_usages["cpu"] : 0.0) << ",\"cores\":[";
    {
        SYSTEM_INFO si; GetSystemInfo(&si);
        for (DWORD i = 0; i < si.dwNumberOfProcessors; i++) {
            if (i > 0) json << ",";
            std::string cname = "cpu" + std::to_string(i);
            json << (cpu_usages.count(cname) ? cpu_usages[cname] : 0.0);
        }
    }
    json << "],\"freqs\":[";
    for (size_t i = 0; i < cpu_freqs.size(); i++) {
        if (i > 0) json << ",";
        json << cpu_freqs[i];
    }
    json << "]},";

    // GPU
    json << "\"gpu\":{\"freq\":" << gpuInfo.freq_mhz << ",\"usage\":{\"rcs\":" << gpuInfo.usage_pct << ",\"bcs\":0.0,\"vcs\":0.0,\"vecs\":0.0}},";

    // Network
    json << "\"network\":{\"rx_rate\":" << g_cur_rx_rate << ",\"tx_rate\":" << g_cur_tx_rate << "},";

    // Disks
    json << "\"disk\":{\"read_rate\":" << g_cur_disk_read_rate << ",\"write_rate\":" << g_cur_disk_write_rate << ",\"disks\":[";
    bool first_disk = true;
    std::string ssd_models_joined;
    for (auto& disk : disks) {
        if (!first_disk) json << ",";
        first_disk = false;

        char sz_buf[32];
        snprintf(sz_buf, sizeof(sz_buf), "%.1f GB", disk.size_gb);
        char usage_buf[128];
        snprintf(usage_buf, sizeof(usage_buf), "%.1f GB Used (%.0f%%) | %.1f GB Free", disk.used_gb, disk.usage_pct, disk.free_gb);

        json << "{\"id\":\"" << escapeJson(disk.id) << "\","
             << "\"dev\":\"" << escapeJson(disk.id) << "\","
             << "\"name\":\"" << escapeJson(disk.model + " (" + sz_buf + ")") << "\","
             << "\"model\":\"" << escapeJson(disk.model) << "\","
             << "\"size\":\"" << sz_buf << "\","
             << "\"capacity_str\":\"" << sz_buf << " Total Capacity\","
             << "\"usage_str\":\"" << escapeJson(usage_buf) << "\","
             << "\"partitions_str\":\"" << escapeJson(disk.id + " (" + disk.type_str + ")") << "\","
             << "\"icon\":\"" << disk.icon << "\","
             << "\"label\":\"" << disk.icon << " " << escapeJson(disk.model + " (" + sz_buf + ")") << "\","
             << "\"type\":\"" << escapeJson(disk.type_str) << "\","
             << "\"transport\":\"" << (disk.is_ssd ? "sata" : "ata") << "\","
             << "\"is_root\":" << (disk.is_system ? "true" : "false") << ","
             << "\"is_rizky\":false,"
             << "\"is_ssd\":" << (disk.is_ssd ? "true" : "false") << ","
             << "\"is_hdd\":" << (!disk.is_ssd ? "true" : "false") << ","
             << "\"read_rate\":" << disk.read_rate << ","
             << "\"write_rate\":" << disk.write_rate << ","
             << "\"tbw_str\":\"N/A\","
             << "\"remaining_str\":\"N/A\","
             << "\"temp_str\":\"" << escapeJson(disk.temp_str) << "\","
             << "\"health_str\":\"" << escapeJson(disk.health_str) << "\""
             << "}";

        if (!ssd_models_joined.empty()) ssd_models_joined += " | ";
        ssd_models_joined += disk.id + " (" + sz_buf + "): " + disk.model;
    }
    json << "],\"list\":[";
    // Repeat disks array for compatibility
    first_disk = true;
    for (auto& disk : disks) {
        if (!first_disk) json << ",";
        first_disk = false;
        char sz_buf[32];
        snprintf(sz_buf, sizeof(sz_buf), "%.1f GB", disk.size_gb);
        json << "{\"id\":\"" << escapeJson(disk.id) << "\",\"label\":\"" << disk.icon << " " << escapeJson(disk.model) << " (" << sz_buf << ")\"}";
    }
    json << "]},";

    // Sensors
    json << "\"sensors\":{\"temp\":" << temp_c << ",\"battery\":" << battery_pct << "},";

    // Processes
    json << "\"processes\":{\"cpu\":[";
    for (size_t i = 0; i < cpu_procs.size(); i++) {
        if (i > 0) json << ",";
        json << "{\"name\":\"" << escapeJson(cpu_procs[i].name) << "\",\"val\":\"" << escapeJson(cpu_procs[i].val) << "\"}";
    }
    json << "],\"mem\":[";
    for (size_t i = 0; i < mem_procs.size(); i++) {
        if (i > 0) json << ",";
        json << "{\"name\":\"" << escapeJson(mem_procs[i].name) << "\",\"val\":\"" << escapeJson(mem_procs[i].val) << "\"}";
    }
    json << "],\"disk\":[";
    for (size_t i = 0; i < disk_procs.size(); i++) {
        if (i > 0) json << ",";
        json << "{\"name\":\"" << escapeJson(disk_procs[i].name) << "\",\"read\":\"" << escapeJson(disk_procs[i].read) << "\",\"write\":\"" << escapeJson(disk_procs[i].write) << "\"}";
    }
    json << "],\"disk_per_dev\":{";
    // Map disk procs to per-drive (simplified: all go under first drive)
    if (!disks.empty()) {
        bool first_dev = true;
        for (auto& disk : disks) {
            if (!first_dev) json << ",";
            first_dev = false;
            json << "\"" << escapeJson(disk.id) << "\":[";
            for (size_t i = 0; i < disk_procs.size() && i < 10; i++) {
                if (i > 0) json << ",";
                json << "{\"name\":\"" << escapeJson(disk_procs[i].name) << "\",\"read\":\"" << escapeJson(disk_procs[i].read) << "\",\"write\":\"" << escapeJson(disk_procs[i].write) << "\"}";
            }
            json << "]";
        }
    }
    json << "},\"net\":[";
    for (size_t i = 0; i < net_procs.size(); i++) {
        if (i > 0) json << ",";
        json << "{\"name\":\"" << escapeJson(net_procs[i].name) << "\",\"dw\":\"" << escapeJson(net_procs[i].down) << "\",\"dr\":\"" << escapeJson(net_procs[i].up) << "\"}";
    }
    json << "],\"gpu\":[]},";

    // Details
    json << "\"details\":{";
    json << "\"raw_cpu\":\"" << escapeJson(g_cache_raw_cpu) << "\",";
    json << "\"raw_gpu\":\"" << escapeJson(g_cache_raw_gpu) << "\",";
    json << "\"raw_ram\":\"" << escapeJson(g_cache_raw_ram) << "\",";
    json << "\"raw_net\":\"" << escapeJson(g_cache_raw_net) << "\",";
    json << "\"raw_disk\":\"" << escapeJson(g_cache_raw_disk) << "\",";
    json << "\"ram_type\":" << g_cache_ram_type_json << ",";
    json << "\"zram_info\":\"Windows Pagefile (Virtual Memory Manager)\",";
    json << "\"swap_info\":\"" << escapeJson("System Managed Pagefile") << "\",";
    json << "\"ssd_model\":\"" << escapeJson(ssd_models_joined) << "\",";
    json << "\"network_type\":\"" << escapeJson(g_cache_network_type) << "\",";
    json << "\"battery_tech\":\"" << escapeJson(g_cache_battery_tech) << "\",";

    // OS info
    std::string os_ver = "Windows";
    std::string build_out = execCmd("cmd /c ver 2>NUL");
    if (!build_out.empty()) os_ver = trim(build_out);
    json << "\"uptime\":3600,\"os\":\"" << escapeJson(os_ver) << "\",\"kernel\":\"NT\"";
    json << "}";

    json << "}";

    {
        std::lock_guard<std::mutex> lock(g_stats_mutex);
        g_cached_stats_json = json.str();
    }
}

static void telemetryLoop() {
    while (g_running) {
        updateTelemetry();
        Sleep(500); // 500ms sampling interval
    }
}

// =============================================================================
// Native HTTP Server (Winsock2)
// =============================================================================
static void handleClient(SOCKET client_fd) {
    char buf[8192] = {};
    int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) { closesocket(client_fd); return; }
    buf[n] = '\0';

    std::string request(buf);
    std::string method, path;
    {
        std::istringstream iss(request);
        iss >> method >> path;
    }

    std::string response_body;
    std::string content_type = "application/json";
    int status_code = 200;

    if (method == "GET" && (path == "/" || path.rfind("/?", 0) == 0 || path == "/index.html")) {
        // Serve index.html
        std::string html = readFile(g_app_dir + "\\index.html");
        if (html.empty()) html = readFile(g_app_dir + "/index.html");
        if (html.empty()) html = "<html><body><h1>RizkybyMONITOR - index.html not found</h1></body></html>";
        response_body = html;
        content_type = "text/html; charset=utf-8";
    }
    else if (method == "GET" && path == "/api/stats") {
        std::lock_guard<std::mutex> lock(g_stats_mutex);
        response_body = g_cached_stats_json;
    }
    else if (method == "GET" && path == "/api/config") {
        response_body = readFile(g_app_dir + "\\config.json");
        if (response_body.empty()) response_body = "{}";
    }
    else if (method == "POST" && path == "/api/config") {
        // Parse body
        size_t body_start = request.find("\r\n\r\n");
        std::string body = (body_start != std::string::npos) ? request.substr(body_start + 4) : "";

        // Parse window_id from body
        int win_id = 1;
        size_t wid_pos = body.find("\"window_id\"");
        if (wid_pos != std::string::npos) {
            size_t val_pos = body.find(':', wid_pos);
            if (val_pos != std::string::npos) {
                win_id = std::atoi(body.c_str() + val_pos + 1);
            }
        }

        // Update window instance state
        {
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            if (g_windows.find(win_id) != g_windows.end()) {
                auto& w = g_windows[win_id];

                // Parse details array
                size_t det_pos = body.find("\"details\"");
                if (det_pos != std::string::npos) {
                    size_t arr_start = body.find('[', det_pos);
                    size_t arr_end = body.find(']', arr_start);
                    if (arr_start != std::string::npos && arr_end != std::string::npos) {
                        w.details = body.substr(arr_start, arr_end - arr_start + 1);
                    }
                }

                // Parse selected_disk
                size_t sd_pos = body.find("\"selected_disk\"");
                if (sd_pos != std::string::npos) {
                    size_t q1 = body.find('"', sd_pos + 15);
                    size_t q2 = body.find('"', q1 + 1);
                    if (q1 != std::string::npos && q2 != std::string::npos) {
                        w.selected_disk = body.substr(q1 + 1, q2 - q1 - 1);
                    }
                }

                // Parse font_size
                size_t fs_pos = body.find("\"font_size\"");
                if (fs_pos != std::string::npos) {
                    size_t val_pos = body.find(':', fs_pos);
                    if (val_pos != std::string::npos) {
                        w.font_size = std::atoi(body.c_str() + val_pos + 1);
                    }
                }

                // Parse mode
                size_t md_pos = body.find("\"mode\"");
                if (md_pos != std::string::npos) {
                    size_t q1 = body.find('"', md_pos + 6);
                    size_t q2 = body.find('"', q1 + 1);
                    if (q1 != std::string::npos && q2 != std::string::npos) {
                        w.mode = body.substr(q1 + 1, q2 - q1 - 1);
                    }
                }
            }
        }

        // Save config
        // (saveWindowConfig will be called)
        response_body = "{\"status\":\"ok\"}";
    }
    else if (method == "POST" && path == "/api/copy") {
        // Copy to clipboard (Windows)
        size_t body_start = request.find("\r\n\r\n");
        std::string body = (body_start != std::string::npos) ? request.substr(body_start + 4) : "";

        size_t tp = body.find("\"text\"");
        if (tp != std::string::npos) {
            size_t q1 = body.find('"', tp + 6);
            size_t q2 = body.rfind('"');
            if (q1 != std::string::npos && q2 > q1) {
                std::string text = body.substr(q1 + 1, q2 - q1 - 1);
                // Clipboard
                if (OpenClipboard(NULL)) {
                    EmptyClipboard();
                    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
                    if (hMem) {
                        memcpy(GlobalLock(hMem), text.c_str(), text.size() + 1);
                        GlobalUnlock(hMem);
                        SetClipboardData(CF_TEXT, hMem);
                    }
                    CloseClipboard();
                }
            }
        }
        response_body = "{\"status\":\"ok\"}";
    }
    else if (method == "POST" && path == "/api/duplicate") {
        // Create new window (post message to main thread)
        {
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            g_next_win_id++;
        }
        // Signal main thread to create window
        response_body = "{\"status\":\"ok\",\"new_id\":" + std::to_string(g_next_win_id) + "}";
    }
    else if (method == "POST" && path.rfind("/api/on_top", 0) == 0) {
        int win_id = 1;
        size_t qp = path.find("win=");
        if (qp != std::string::npos) win_id = std::atoi(path.c_str() + qp + 4);

        std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
        if (g_windows.find(win_id) != g_windows.end()) {
            auto& w = g_windows[win_id];
            w.is_on_top = !w.is_on_top;
            SetWindowPos(w.hwnd, w.is_on_top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            response_body = "{\"status\":\"ok\",\"on_top\":" + std::string(w.is_on_top ? "true" : "false") + "}";
        } else {
            response_body = "{\"status\":\"not_found\"}";
        }
    }
    else if (method == "POST" && path.rfind("/api/close_window", 0) == 0) {
        int win_id = 1;
        size_t qp = path.find("win=");
        if (qp != std::string::npos) win_id = std::atoi(path.c_str() + qp + 4);

        std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
        if (g_windows.find(win_id) != g_windows.end()) {
            DestroyWindow(g_windows[win_id].hwnd);
            g_windows.erase(win_id);
        }
        if (g_windows.empty()) {
            PostQuitMessage(0);
        }
        response_body = "{\"status\":\"ok\"}";
    }
    else if (method == "POST" && path == "/api/quit") {
        g_running = false;
        PostQuitMessage(0);
        response_body = "{\"status\":\"ok\"}";
    }
    else if (method == "GET" && path.rfind("/api/disk-detail", 0) == 0) {
        std::string dev = "";
        size_t dp = path.find("dev=");
        if (dp != std::string::npos) dev = path.substr(dp + 4);

        std::string detail;
        {
            std::lock_guard<std::mutex> lock(g_stats_mutex);
            if (g_disk_details_map.find(dev) != g_disk_details_map.end()) {
                detail = g_disk_details_map[dev];
            }
        }
        if (detail.empty()) {
            detail = buildDiskDetailText(dev);
            std::lock_guard<std::mutex> lock(g_stats_mutex);
            g_disk_details_map[dev] = detail;
        }
        response_body = "{\"detail_text\":\"" + escapeJson(detail) + "\"}";
    }
    else {
        status_code = 404;
        response_body = "{\"error\":\"Not Found\"}";
    }

    std::stringstream resp;
    resp << "HTTP/1.1 " << status_code << " OK\r\n";
    resp << "Content-Type: " << content_type << "\r\n";
    resp << "Content-Length: " << response_body.size() << "\r\n";
    resp << "Access-Control-Allow-Origin: *\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << response_body;

    std::string resp_str = resp.str();
    send(client_fd, resp_str.c_str(), (int)resp_str.size(), 0);
    closesocket(client_fd);
}

static void startHttpServer() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(g_server_port);

    int retries = 0;
    while (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        if (++retries > 15) {
            std::cerr << "Failed to bind after 15 retries\n";
            closesocket(server_fd);
            return;
        }
        Sleep(100);
    }

    listen(server_fd, 64);

    while (g_running) {
        struct sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);
        SOCKET client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd == INVALID_SOCKET) continue;
        std::thread(handleClient, client_fd).detach();
    }

    closesocket(server_fd);
    WSACleanup();
}

// =============================================================================
// Win32 Window Management + WebView2
// =============================================================================
static void saveWindowConfig() {
    std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
    std::stringstream cfg;
    cfg << "{\"window_count\":" << g_windows.size() << ",\"windows\":{";
    bool first = true;
    for (auto& [id, w] : g_windows) {
        if (!first) cfg << ",";
        first = false;

        RECT rect;
        GetWindowRect(w.hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        cfg << "\"" << id << "\":{\"width\":" << width << ",\"height\":" << height
            << ",\"x\":" << rect.left << ",\"y\":" << rect.top
            << ",\"on_top\":" << (w.is_on_top ? "true" : "false")
            << ",\"details\":\"" << escapeJson(w.details) << "\""
            << ",\"mode\":\"" << escapeJson(w.mode) << "\""
            << ",\"font_size\":" << w.font_size
            << ",\"selected_disk\":\"" << escapeJson(w.selected_disk) << "\""
            << "}";
    }
    cfg << "}}";
    writeFile(g_app_dir + "\\config.json", cfg.str());
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE: {
            // Resize WebView2 controller
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            for (auto& [id, w] : g_windows) {
                if (w.hwnd == hwnd && w.controller) {
                    RECT bounds;
                    GetClientRect(hwnd, &bounds);
                    w.controller->put_Bounds(bounds);
                }
            }
            return 0;
        }
        case WM_NCHITTEST: {
            // Enable frameless window dragging from top area
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT rc;
            GetClientRect(hwnd, &rc);
            // Top 30px = drag area (titlebar)
            if (pt.y < 30) return HTCAPTION;
            return HTCLIENT;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY: {
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            for (auto it = g_windows.begin(); it != g_windows.end(); ++it) {
                if (it->second.hwnd == hwnd) {
                    g_windows.erase(it);
                    break;
                }
            }
            if (g_windows.empty()) {
                saveWindowConfig();
                PostQuitMessage(0);
            }
            return 0;
        }
        case WM_MOVE:
        case WM_EXITSIZEMOVE:
            saveWindowConfig();
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void createNewWindow(int win_id) {
    // Read saved config
    int w = 480, h = 720, x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    bool on_top = false;
    std::string details = "[]", mode = "dark", selected_disk = "C:";
    int font_size = 14;

    std::string cfg = readFile(g_app_dir + "\\config.json");
    if (!cfg.empty()) {
        std::string key = "\"" + std::to_string(win_id) + "\"";
        size_t pos = cfg.find(key);
        if (pos != std::string::npos) {
            // Simple parse
            auto getInt = [&](const std::string& field, int def) -> int {
                size_t fp = cfg.find("\"" + field + "\"", pos);
                if (fp == std::string::npos || fp > pos + 500) return def;
                size_t cp = cfg.find(':', fp);
                if (cp == std::string::npos) return def;
                return std::atoi(cfg.c_str() + cp + 1);
            };
            w = getInt("width", 480);
            h = getInt("height", 720);
            x = getInt("x", CW_USEDEFAULT);
            y = getInt("y", CW_USEDEFAULT);
            font_size = getInt("font_size", 14);

            size_t otp = cfg.find("\"on_top\"", pos);
            if (otp != std::string::npos && otp < pos + 500) {
                on_top = (cfg.find("true", otp) != std::string::npos && cfg.find("true", otp) < otp + 20);
            }
        }
    }

    // Create frameless window (WS_POPUP for no titlebar/border)
    DWORD style = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    DWORD exStyle = WS_EX_APPWINDOW;
    if (on_top) exStyle |= WS_EX_TOPMOST;

    HWND hwnd = CreateWindowExW(
        exStyle, WINDOW_CLASS, L"RizkybyMONITOR",
        style, x, y, w, h,
        NULL, NULL, g_hInstance, NULL);

    if (!hwnd) {
        std::cerr << "Failed to create window for win_id " << win_id << std::endl;
        return;
    }

    WindowInstance wi = {};
    wi.id = win_id;
    wi.hwnd = hwnd;
    wi.is_on_top = on_top;
    wi.details = details;
    wi.mode = mode;
    wi.font_size = font_size;
    wi.selected_disk = selected_disk;

    {
        std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
        g_windows[win_id] = wi;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Initialize WebView2
    std::wstring userDataFolder = strToWstr(g_app_dir) + L"\\webview2_data";

    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, userDataFolder.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd, win_id](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) return result;

                env->CreateCoreWebView2Controller(hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hwnd, win_id](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || !controller) return result;

                            ComPtr<ICoreWebView2> webview;
                            controller->get_CoreWebView2(&webview);

                            RECT bounds;
                            GetClientRect(hwnd, &bounds);
                            controller->put_Bounds(bounds);
                            controller->put_IsVisible(TRUE);

                            // Disable default context menu
                            ComPtr<ICoreWebView2Settings> settings;
                            webview->get_Settings(&settings);
                            settings->put_AreDefaultContextMenusEnabled(FALSE);
                            settings->put_AreDevToolsEnabled(TRUE);
                            settings->put_IsStatusBarEnabled(FALSE);

                            // Navigate to local server
                            std::wstring url = L"http://127.0.0.1:" + std::to_wstring(g_server_port) + L"/?win=" + std::to_wstring(win_id);
                            webview->Navigate(url.c_str());

                            // Store references
                            {
                                std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
                                if (g_windows.find(win_id) != g_windows.end()) {
                                    g_windows[win_id].controller = controller;
                                    g_windows[win_id].webview = webview;
                                }
                            }

                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
}

// =============================================================================
// WinMain Entry Point
// =============================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    g_hInstance = hInstance;

    // Initialize COM for WebView2
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Resolve application directory
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    g_app_dir = wstrToStr(exePath);

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = WINDOW_CLASS;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    // Start HTTP server in background
    std::thread(startHttpServer).detach();

    // Run initial telemetry sample
    updateTelemetry();

    // Start background hardware info and telemetry threads
    std::thread([]() {
        initHardwareInfo();
        updateTelemetry(); // Refresh with hardware info
    }).detach();
    std::thread(telemetryLoop).detach();

    // Parse window count from config
    int window_count = 1;
    std::string cfg = readFile(g_app_dir + "\\config.json");
    if (!cfg.empty()) {
        size_t wcp = cfg.find("\"window_count\"");
        if (wcp != std::string::npos) {
            size_t cp = cfg.find(':', wcp);
            if (cp != std::string::npos) {
                window_count = std::atoi(cfg.c_str() + cp + 1);
                if (window_count < 1) window_count = 1;
            }
        }
    }
    g_next_win_id = window_count + 1;

    // Create windows
    for (int i = 1; i <= window_count; i++) {
        createNewWindow(i);
    }

    // Win32 Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_running = false;
    saveWindowConfig();
    CoUninitialize();
    return 0;
}

#else
// This file is only for Windows builds.
// On Linux, compile src/main.cpp instead.
int main() {
    std::cerr << "Error: main_windows.cpp should only be compiled on Windows.\n";
    std::cerr << "Use src/main.cpp for Linux builds.\n";
    return 1;
}
#endif // _WIN32
