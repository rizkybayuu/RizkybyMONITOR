// =============================================================================
// RizkybyMONITOR – Windows Native Port (C++17)
// =============================================================================
// Win32 API + WebView2 (Microsoft Edge Chromium) + Winsock2 HTTP Server
// 100% Native C++ (Zero WMIC, Zero cmd.exe, Zero Popups)
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
#define INITGUID
#ifndef PDH_MORE_DATA
#define PDH_MORE_DATA ((PDH_STATUS)0x800007D2L)
#endif
#ifndef PDH_CSTATUS_VALID_DATA
#define PDH_CSTATUS_VALID_DATA ((DWORD)0x00000000L)
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <pdh.h>
#include <psapi.h>
#include <powrprof.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tlhelp32.h>
#include <winioctl.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <dxgi1_4.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <intrin.h>
#include <ntddscsi.h>   // IOCTL_ATA_PASS_THROUGH (SATA/USB-UASP SMART TBW readout)
#include <initguid.h>
#include <batclass.h>   // IOCTL_BATTERY_* (chemistry, charging state, wear/health)
#include <wlanapi.h>
#include <dwmapi.h>

#include <iostream>
#include <fstream>
#include <iomanip>
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
#include <Wbemidl.h>

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
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "dwmapi.lib")

#include <WebView2.h>
#include <wrl.h>
using namespace Microsoft::WRL;

// Custom Messages untuk Komunikasi Thread-Safe
#define WM_APP_CREATE_WINDOW    (WM_APP + 1)
#define WM_APP_TOGGLE_ON_TOP    (WM_APP + 2)
#define WM_APP_CLOSE_WIN        (WM_APP + 3)
#define WM_APP_CLOSE_ALL        (WM_APP + 4)

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
    std::string app_title; // <-- Simpan nama window individual di memory
};

struct ProcEntry {
    std::string name;
    std::string val;
};

struct DiskProcEntry {
    std::string name;
    std::string read;
    std::string write;
    std::string val;
    double total_bytes;
};

struct NetProcEntry {
    std::string name;
    std::string down;
    std::string up;
    std::string val;
    double total;
};

struct GpuProcEntry {
    std::string name;
    double rcs, bcs, vcs, vecs, total;
};

struct GpuAdapterInfo {
    std::string id;
    std::string name;
    bool is_egpu;
    double dedicated_vram_gb;
    double shared_vram_gb;
    double usage_pct;
};

struct DiskDriveInfo {
    std::string id;
    std::string dev;
    std::string model;
    std::string size_str;
    double size_gb;
    double used_gb;
    double free_gb;
    double usage_pct;
    std::string type_str;
    std::string icon;
    bool is_system;
    bool is_ssd;
    bool is_nvme;
    std::string partitions_str;
    std::string temp_str;
    std::string health_str;
    std::string tbw_str;
    std::string remaining_str; // <-- TAMBAHKAN BARIS INI (Fix Build Error)
    double read_rate;
    double write_rate;
    bool is_removable;        // true = genuine flash drive/SD card, not a Portable SSD
    std::string vendor;       // vendor string from STORAGE_DEVICE_DESCRIPTOR
    std::string serial;       // serial number from STORAGE_DEVICE_DESCRIPTOR
    bool seek_penalty_known;  // whether the seek-penalty query succeeded
    bool has_seek_penalty;    // true = rotational, false = solid-state
};

// =============================================================================
// Global Variables
// =============================================================================
static HINSTANCE g_hInstance = NULL;
static HWND g_main_hwnd = NULL;
static const wchar_t* WINDOW_CLASS = L"RizkybyMONITOR_Class";

static std::recursive_mutex g_win_mutex;
static std::map<int, WindowInstance> g_windows;
static int g_next_win_id = 1;
static std::string g_app_dir;
static std::string g_data_dir; // %LOCALAPPDATA%\RizkybyMONITOR — selalu writable per-user
static int g_server_port = 8080;
static std::atomic<bool> g_running{true};
static std::atomic<bool> g_is_quitting{false};

static std::mutex g_stats_mutex;
static std::string g_cached_stats_json;
static std::map<std::string, std::string> g_disk_details_map;
static std::map<std::string, std::pair<double, double>> g_disk_rates_by_id;

static std::string g_cpu_model = "Intel/AMD Processor";
static std::string g_gpu_model = "Graphics Processor";
static std::vector<GpuAdapterInfo> g_gpu_adapters;

static std::string g_cache_raw_cpu;
static std::string g_cache_raw_gpu;
static std::string g_cache_raw_ram;
static std::string g_cache_raw_net;
static std::string g_cache_raw_disk;
static std::string g_cache_ram_type_json = "[\"DDR4 / DDR5 SDRAM\"]";
static std::string g_cache_network_type = "Wi-Fi / Ethernet Adapter";
static std::string g_cache_battery_tech = "Li-ion Standard Battery";

static std::map<std::string, CpuTick> g_last_cpu_ticks;
static std::map<DWORD, ProcIoTick> g_last_proc_io_ticks;

static TRACEHANDLE g_etw_session_handle = 0;
static TRACEHANDLE g_etw_trace_handle = INVALID_PROCESSTRACE_HANDLE;
static HANDLE g_etw_thread = NULL;
static std::mutex g_diskio_etw_mutex;
// DiskNumber -> PID -> {read_bytes, write_bytes} terakumulasi sejak polling terakhir
static std::map<ULONG, std::map<DWORD, std::pair<unsigned long long, unsigned long long>>> g_diskio_by_disk_pid;
static std::map<ULONG, std::map<DWORD, std::pair<unsigned long long, unsigned long long>>> g_last_diskio_by_disk_pid;

static const GUID DiskIoTraceGuid =
    { 0x3d6fa8d4, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };

#pragma pack(push, 1)
struct DiskIo_TypeGroup1 {
    ULONG     DiskNumber;
    ULONG     IrpFlags;
    ULONG     TransferSize;
    ULONG     ResponseTime;
    ULONG64   ByteOffset;
    ULONG_PTR FileObject;
    ULONG_PTR Irp;
    ULONG64   HighResResponseTime;
    ULONG     IssuingThreadId; // Win8+
};
#pragma pack(pop)

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

// =============================================================================
// Helper Functions (File I/O)
// =============================================================================
static std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return "";
    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    if (f.is_open()) f << content;
}

// =============================================================================
// Persistent Debug Logger — nulis ke %TEMP%\RizkybyMonitor_debug.log
// Dipisah dari AllocConsole() biar tetap kebaca walau console ketutup/proses exit
// =============================================================================
// ✅ KODE BARU: Membatasi maksimal 10.000 baris (Auto-Rotate Log)
static std::mutex g_log_mutex;
static int g_log_line_count = 0; // Cache jumlah baris biar gak bolak-balik baca disk

static void Log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    SYSTEMTIME st; GetLocalTime(&st);
    char ts[32];
    sprintf_s(ts, sizeof(ts), "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    char tempPath[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, tempPath);
    std::string logPath = std::string(tempPath) + "RizkybyMonitor_debug.log";

    // Hitung baris di awal jika cache masih 0
    if (g_log_line_count == 0) {
        std::ifstream countFile(logPath);
        if (countFile.is_open()) {
            std::string line;
            while (std::getline(countFile, line)) g_log_line_count++;
        }
    }

    // Jika sudah mencapai/melewati 10.000 baris, reset file log (Truncate)
    bool shouldTruncate = (g_log_line_count >= 10000);
    std::ofstream f(logPath, shouldTruncate ? std::ios::trunc : std::ios::app);
    
    if (f.is_open()) {
        if (shouldTruncate) {
            f << ts << "[LOG ROTATION] File log di-reset karena sudah mencapai batas 10.000 baris.\n";
            g_log_line_count = 1;
        }
        f << ts << msg << "\n";
        g_log_line_count++;
    }
}

// =============================================================================
// String & Time Utilities (100% Native - Tanpa Console Spawn)
// =============================================================================
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
                break;
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
// CPU Telemetry (Windows NtQuerySystemInformation & CallNtPowerInformation)
// =============================================================================
// Deteksi P-Core (Performance) vs E-Core (Efficiency) secara dinamis via Windows Kernel Topology
static std::vector<std::string> detectCpuCoreTypes() {
    std::vector<std::string> core_types;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int total_logical = (si.dwNumberOfProcessors > 0) ? (int)si.dwNumberOfProcessors : 1;
    core_types.resize(total_logical, "p-core"); // Default homogen

    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &len);
    if (len > 0) {
        std::vector<BYTE> buf(len);
        if (GetLogicalProcessorInformationEx(RelationProcessorCore, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf.data(), &len)) {
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = NULL;
            DWORD offset = 0;
            bool has_hybrid = false;

            // Cek apakah CPU memiliki arsitektur hybrid (ada EfficiencyClass yang berbeda)
            while (offset < len) {
                info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf.data() + offset);
                if (info->Relationship == RelationProcessorCore && info->Processor.EfficiencyClass > 0) {
                    has_hybrid = true;
                    break;
                }
                offset += info->Size;
            }

            if (has_hybrid) {
                offset = 0;
                while (offset < len) {
                    info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf.data() + offset);
                    if (info->Relationship == RelationProcessorCore) {
                        std::string type = (info->Processor.EfficiencyClass == 0) ? "e-core" : "p-core";
                        for (WORD g = 0; g < info->Processor.GroupCount; g++) {
                            KAFFINITY mask = info->Processor.GroupMask[g].Mask;
                            for (int bit = 0; bit < 64; bit++) {
                                if (mask & ((KAFFINITY)1 << bit)) {
                                    if (bit < total_logical) {
                                        core_types[bit] = type;
                                    }
                                }
                            }
                        }
                    }
                    offset += info->Size;
                }
            }
        }
    }
    return core_types;
}

// =============================================================================
// Topologi CPU & GPU Engines Dinamis (100% Native OS Telemetry)
// =============================================================================
struct CoreTopologyInfo {
    std::string tag;   // "P", "E", "LP", atau "C"
    std::string type;  // "p-core", "e-core", "lp-core"
    BYTE efficiency_class;
};

static std::vector<CoreTopologyInfo> getDynamicCpuTopology() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int total_logical = (si.dwNumberOfProcessors > 0) ? (int)si.dwNumberOfProcessors : 1;
    std::vector<CoreTopologyInfo> topology(total_logical, { "C", "p-core", 0 });

    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &len);
    if (len == 0) return topology;

    std::vector<BYTE> buf(len);
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf.data(), &len)) {
        return topology;
    }

    BYTE max_eff = 0, min_eff = 255;
    std::set<BYTE> distinct_classes;
    DWORD offset = 0;
    while (offset < len) {
        auto* info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf.data() + offset);
        if (info->Relationship == RelationProcessorCore) {
            BYTE eff = info->Processor.EfficiencyClass;
            distinct_classes.insert(eff);
            if (eff > max_eff) max_eff = eff;
            if (eff < min_eff) min_eff = eff;
        }
        offset += info->Size;
    }

    bool is_heterogeneous = (distinct_classes.size() > 1);

    offset = 0;
    while (offset < len) {
        auto* info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf.data() + offset);
        if (info->Relationship == RelationProcessorCore) {
            BYTE eff = info->Processor.EfficiencyClass;
            std::string tag = "C";
            std::string type = "p-core";

            if (is_heterogeneous) {
                if (distinct_classes.size() >= 3) {
                    if (eff == max_eff) { tag = "P"; type = "p-core"; }
                    else if (eff == min_eff) { tag = "LP"; type = "lp-core"; }
                    else { tag = "E"; type = "e-core"; }
                } else {
                    if (eff == max_eff) { tag = "P"; type = "p-core"; }
                    else { tag = "E"; type = "e-core"; }
                }
            }

            for (WORD g = 0; g < info->Processor.GroupCount; g++) {
                KAFFINITY mask = info->Processor.GroupMask[g].Mask;
                for (int bit = 0; bit < 64; bit++) {
                    if (mask & ((KAFFINITY)1 << bit)) {
                        if (bit < total_logical) {
                            topology[bit] = { tag, type, eff };
                        }
                    }
                }
            }
        }
        offset += info->Size;
    }
    return topology;
}

struct GpuEngineData {
    std::string name;
    double usage;
};

static std::vector<GpuEngineData> getDynamicGpuEngines() {
    static PDH_HQUERY query = NULL;
    static PDH_HCOUNTER counter = NULL;
    static bool init_pdh = false;

    std::vector<GpuEngineData> engines;

    if (!init_pdh || query == NULL) {
        if (query) { PdhCloseQuery(query); query = NULL; }
        if (PdhOpenQueryW(NULL, 0, &query) == ERROR_SUCCESS) {
            if (PdhAddEnglishCounterW(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter) == ERROR_SUCCESS) {
                PdhCollectQueryData(query);
                init_pdh = true;
            } else { return engines; }
        } else { return engines; }
    }

    if (PdhCollectQueryData(query) != ERROR_SUCCESS) {
        init_pdh = false;
        return engines;
    }

    DWORD bufSize = 0, itemCount = 0;
    PDH_STATUS status = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, NULL);
    if (status != PDH_MORE_DATA && status != ERROR_SUCCESS) return engines;
    if (bufSize == 0) return engines;

    std::vector<BYTE> buf(bufSize);
    auto* items = (PDH_FMT_COUNTERVALUE_ITEM_W*)buf.data();
    if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, items) != ERROR_SUCCESS) return engines;

    std::map<std::string, double> aggregated_engines;

    for (DWORD i = 0; i < itemCount; i++) {
        if (items[i].FmtValue.CStatus != PDH_CSTATUS_VALID_DATA) continue;

        std::wstring inst = items[i].szName;
        double val = items[i].FmtValue.doubleValue;
        if (val <= 0.0) continue;

        size_t engPos = inst.find(L"engtype_");
        if (engPos != std::wstring::npos) {
            std::wstring engName = inst.substr(engPos + 8);
            size_t endPos = engName.find_first_of(L"_)#");
            if (endPos != std::wstring::npos) engName = engName.substr(0, endPos);

            std::string nameA = wstrToStr(engName);
            for (auto& c : nameA) c = toupper(c);
            aggregated_engines[nameA] += val;
        }
    }

    for (auto& [name, val] : aggregated_engines) {
        engines.push_back({ name, std::min(100.0, val) });
    }

    std::sort(engines.begin(), engines.end(), [](const GpuEngineData& a, const GpuEngineData& b) {
        return a.usage > b.usage;
    });

    return engines;
}

static void getCpuUsage(std::map<std::string, double>& cpu_usages) {
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
                cpu_usages["cpu"] = std::max(0.0, std::min(100.0, std::round(u * 10.0) / 10.0));
            }
        } else {
            cpu_usages["cpu"] = 0.0;
        }
        g_last_cpu_ticks["cpu"] = { idle, total };
    }

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

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int ncpu = (int)si.dwNumberOfProcessors;

    if (NtQuerySystemInformation) {
        std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> cpuInfo(ncpu);
        ULONG returnLength = 0;
        NTSTATUS status = NtQuerySystemInformation(8, cpuInfo.data(),
            (ULONG)(ncpu * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)), &returnLength);

        if (status == 0) {
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
                        cpu_usages[coreName] = std::max(0.0, std::min(100.0, std::round(u * 10.0) / 10.0));
                    }
                } else {
                    cpu_usages[coreName] = 0.0;
                }
                g_last_cpu_ticks[coreName] = { idle_t, total_t };
            }
        }
    }
}

static std::vector<int> getCpuFrequencies() {
    std::vector<int> freqs;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int ncpu = (int)si.dwNumberOfProcessors;

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
    } else {
        // Fallback: baca base clock asli dari Registry Windows CentralProcessor
        DWORD base_mhz = 0;
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD mhzSize = sizeof(base_mhz);
            RegQueryValueExA(hKey, "~MHz", NULL, NULL, (LPBYTE)&base_mhz, &mhzSize);
            RegCloseKey(hKey);
        }
        for (int i = 0; i < ncpu; i++) freqs.push_back((int)base_mhz);
    }
    return freqs;
}

// =============================================================================
// Memory Telemetry (Perbaikan: Pure Pagefile & RAM Native)
// =============================================================================
struct MemoryInfo {
    unsigned long long total;
    unsigned long long used;
    unsigned long long avail;
    unsigned long long pagefile_total;
    unsigned long long pagefile_used;
    unsigned long long compressed_bytes;
};

static MemoryInfo getMemoryInfo() {
    MemoryInfo mi = {};
    MEMORYSTATUSEX ms = { sizeof(ms) };
    if (GlobalMemoryStatusEx(&ms)) {
        mi.total = ms.ullTotalPhys;
        mi.avail = ms.ullAvailPhys;
        mi.used = mi.total - mi.avail;

        PERFORMANCE_INFORMATION pi = { sizeof(pi) };
        if (GetPerformanceInfo(&pi, sizeof(pi))) {
            unsigned long long pageSize = pi.PageSize;
            unsigned long long physTotal = pi.PhysicalTotal * pageSize;
            unsigned long long physAvail = pi.PhysicalAvailable * pageSize;
            unsigned long long commitTotal = pi.CommitTotal * pageSize;
            unsigned long long commitLimit = pi.CommitLimit * pageSize;

            // Hitung ukuran murni file pagefile di disk
            if (commitLimit > physTotal) {
                mi.pagefile_total = commitLimit - physTotal;
            } else {
                mi.pagefile_total = 0;
            }

            // Hitung penggunaan murni pagefile
            unsigned long long physUsed = physTotal - physAvail;
            if (commitTotal > physUsed) {
                mi.pagefile_used = std::min(commitTotal - physUsed, mi.pagefile_total);
            } else {
                mi.pagefile_used = 0;
            }

            mi.compressed_bytes = pi.SystemCache * pageSize;
        } else {
            mi.pagefile_total = (ms.ullTotalPageFile > ms.ullTotalPhys) ? (ms.ullTotalPageFile - ms.ullTotalPhys) : 0;
            mi.pagefile_used = (ms.ullTotalPageFile - ms.ullAvailPageFile);
        }
    }
    return mi;
}

// 🟢 AFTER: Menghitung TOTAL seluruh CPU Cache (L1 + L2 + L3) secara akurat
static unsigned long long getCpuSmartCacheTotalBytes() {
    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationCache, NULL, &len);
    if (len == 0) return 0;

    std::vector<BYTE> buf(len);
    if (!GetLogicalProcessorInformationEx(RelationCache, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf.data(), &len)) {
        return 0;
    }

    unsigned long long total_cache_bytes = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = NULL;
    DWORD offset = 0;

    while (offset < len) {
        info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf.data() + offset);
        if (info->Relationship == RelationCache) {
            // Hitung seluruh level (L1, L2, L3) untuk Smart Cache
            if (info->Cache.Level >= 1) {
                total_cache_bytes += info->Cache.CacheSize;
            }
        }
        offset += info->Size;
    }
    return total_cache_bytes;
}

// =============================================================================
// GPU Telemetry (DXGI Adapters + PDH Utilization Counter)
// =============================================================================
struct GpuInfo {
    int freq_mhz;
    double usage_pct;
    std::string model;
};

static void enumerateGpuAdapters() {
    std::vector<GpuAdapterInfo> list;
    ComPtr<IDXGIFactory1> factory;
    if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory)) && factory) {
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

            GpuAdapterInfo info;
            info.id = "gpu" + std::to_string(i);
            info.name = wstrToStr(desc.Description);
            info.dedicated_vram_gb = (double)desc.DedicatedVideoMemory / (1024.0 * 1024.0 * 1024.0);
            info.shared_vram_gb = (double)desc.SharedSystemMemory / (1024.0 * 1024.0 * 1024.0);
            
            std::string name_upper = info.name;
            for (auto& c : name_upper) c = toupper(c);

            info.is_egpu = (info.dedicated_vram_gb >= 0.5 && 
                            name_upper.find("INTEL") == std::string::npos && 
                            name_upper.find("GRAPHICS") == std::string::npos);
            info.usage_pct = 0.0;
            list.push_back(info);
        }
    }

    if (list.empty()) {
        GpuAdapterInfo def;
        def.id = "gpu0";
        def.name = "N/A (GPU not detected via DXGI)";
        def.is_egpu = false;
        def.dedicated_vram_gb = 0.0;
        def.shared_vram_gb = 0.0;
        def.usage_pct = 0.0;
        list.push_back(def);
    }

    // Mengurutkan GPU agar dGPU (NVIDIA/AMD) otomatis diutamakan ke indeks 0
    std::sort(list.begin(), list.end(), [](const GpuAdapterInfo& a, const GpuAdapterInfo& b) {
        return a.dedicated_vram_gb > b.dedicated_vram_gb;
    });

    g_gpu_adapters = list;
    g_gpu_model = list[0].name;
}

static GpuInfo getGpuInfo() {
    GpuInfo gi = { 0, 0.0, g_gpu_model };
    static PDH_HQUERY query = NULL;
    static PDH_HCOUNTER counter = NULL;
    static bool init_pdh = false;

    if (!init_pdh) {
        if (PdhOpenQueryW(NULL, 0, &query) == ERROR_SUCCESS) {
            PdhAddEnglishCounterW(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter);
            PdhCollectQueryData(query);
        }
        init_pdh = true;
    }

    if (counter) {
        if (PdhCollectQueryData(query) == ERROR_SUCCESS) {
            PDH_FMT_COUNTERVALUE val;
            if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &val) == ERROR_SUCCESS) {
                gi.usage_pct = std::max(0.0, std::min(100.0, val.doubleValue));
            }
        }
    }
    return gi;
}

// =============================================================================
// Network Telemetry (Windows GetIfTable2)
// =============================================================================
static void updateNetworkTelemetry() {
    double now = getTimeSec();
    MIB_IF_TABLE2* ifTable = nullptr;
    if (GetIfTable2(&ifTable) == NO_ERROR) {
        unsigned long long total_rx = 0;
        unsigned long long total_tx = 0;
        for (ULONG i = 0; i < ifTable->NumEntries; i++) {
            const MIB_IF_ROW2& row = ifTable->Table[i];
            if (row.InterfaceAndOperStatusFlags.HardwareInterface &&
                row.OperStatus == IfOperStatusUp &&
                row.Type != IF_TYPE_SOFTWARE_LOOPBACK) {
                total_rx += row.InOctets;
                total_tx += row.OutOctets;
            }
        }
        FreeMibTable(ifTable);

        if (g_last_net_time > 0.0) {
            double dt = now - g_last_net_time;
            if (dt > 0.1) {
                if (total_rx >= g_last_net_rx) g_cur_rx_rate = (double)(total_rx - g_last_net_rx) / dt;
                if (total_tx >= g_last_net_tx) g_cur_tx_rate = (double)(total_tx - g_last_net_tx) / dt;
            }
        }
        g_last_net_rx = total_rx;
        g_last_net_tx = total_tx;
        g_last_net_time = now;
    }
}

// =============================================================================
// Disk Discovery: Physical Disks + Partition Mapping + NVMe SMART/TBW
// =============================================================================

// ATA SMART READ DATA via passthrough (buat SATA & USB-UASP SSD/Portable SSD)
static bool probeAtaSmartTbw(HANDLE hDisk, std::string& tbw_out, std::string& health_out) {
    struct { ATA_PASS_THROUGH_EX header; BYTE data[512]; } buf = {};
    buf.header.Length = sizeof(ATA_PASS_THROUGH_EX);
    buf.header.AtaFlags = ATA_FLAGS_DATA_IN;
    buf.header.DataTransferLength = 512;
    buf.header.TimeOutValue = 3;
    buf.header.DataBufferOffset = offsetof(decltype(buf), data);
    buf.header.CurrentTaskFile[0] = 0xD0; // Features: SMART READ DATA
    buf.header.CurrentTaskFile[1] = 0x01; // Sector Count
    buf.header.CurrentTaskFile[3] = 0x4F; // LBA Mid
    buf.header.CurrentTaskFile[4] = 0xC2; // LBA High
    buf.header.CurrentTaskFile[6] = 0xB0; // Command: SMART

    DWORD ret = 0;
    if (!DeviceIoControl(hDisk, IOCTL_ATA_PASS_THROUGH, &buf, sizeof(buf), &buf, sizeof(buf), &ret, NULL)) {
        return false;
    }

    // Attribute table mulai offset 2, tiap entry 12 byte: [ID][Flags(2)][Value][Worst][Raw(6)][Rsvd]
    for (int i = 0; i < 30; i++) {
        int off = 2 + i * 12;
        BYTE id = buf.data[off];
        if (id == 0) continue;
        if (id == 241 || id == 246) { // Total_LBAs_Written (umum di Samsung/SanDisk/Crucial)
            unsigned long long raw = 0;
            for (int b = 0; b < 6; b++) raw |= ((unsigned long long)buf.data[off + 5 + b]) << (8 * b);
            double tbw = (double)raw * 512.0 / (1024.0 * 1024.0 * 1024.0 * 1024.0); // sector -> TB
            char b1[32]; snprintf(b1, sizeof(b1), "%.3f TB Written", tbw);
            tbw_out = b1;
        }
        if (id == 231 || id == 169) { // SSD Life Left / Media Wearout
            health_out = std::to_string(buf.data[off + 3]) + "% Good"; // Value byte
        }
    }
    return !tbw_out.empty();
}

// NEW FUNCTION — insert between lines 572 and 574
// Fallback SAT (SCSI/ATA Translation) — some USB-SATA bridges reject
// pure IOCTL_ATA_PASS_THROUGH but accept ATA PASS-THROUGH(16)
// wrapped via IOCTL_SCSI_PASS_THROUGH_DIRECT (used by UASP/SAT-compliant bridges).
static bool probeScsiAtaSmartTbw(HANDLE hDisk, std::string& tbw_out, std::string& health_out) {
    struct {
        SCSI_PASS_THROUGH_DIRECT header;
        BYTE sense[32];
    } sptd = {};
    BYTE dataBuf[512] = {};

    sptd.header.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptd.header.CdbLength = 16;
    sptd.header.SenseInfoLength = sizeof(sptd.sense);
    sptd.header.DataIn = SCSI_IOCTL_DATA_IN;
    sptd.header.DataTransferLength = sizeof(dataBuf);
    sptd.header.TimeOutValue = 3;
    sptd.header.DataBuffer = dataBuf;
    sptd.header.SenseInfoOffset = offsetof(decltype(sptd), sense);

    BYTE* cdb = sptd.header.Cdb;
    cdb[0]  = 0x85; // ATA PASS-THROUGH(16)
    cdb[1]  = 0x08; // PROTOCOL = PIO Data-In, EXTEND = 0
    cdb[2]  = 0x0E; // T_DIR=1, BYTE_BLOCK=1, T_LENGTH=SECTOR_COUNT
    cdb[4]  = 0xD0; // FEATURES = SMART READ DATA
    cdb[6]  = 0x01; // SECTOR_COUNT = 1
    cdb[10] = 0x4F; // LBA MID
    cdb[12] = 0xC2; // LBA HIGH
    cdb[13] = 0xA0; // DEVICE
    cdb[14] = 0xB0; // COMMAND = SMART

    DWORD ret = 0;
    if (!DeviceIoControl(hDisk, IOCTL_SCSI_PASS_THROUGH_DIRECT, &sptd, sizeof(sptd),
                         &sptd, sizeof(sptd), &ret, NULL)) {
        return false;
    }

    for (int i = 0; i < 30; i++) {
        int off = 2 + i * 12;
        BYTE id = dataBuf[off];
        if (id == 0) continue;
        if (id == 241 || id == 246) {
            unsigned long long raw = 0;
            for (int b = 0; b < 6; b++) raw |= ((unsigned long long)dataBuf[off + 5 + b]) << (8 * b);
            double tbw = (double)raw * 512.0 / (1024.0 * 1024.0 * 1024.0 * 1024.0);
            char b1[32]; snprintf(b1, sizeof(b1), "%.3f TB Written", tbw);
            tbw_out = b1;
        }
        if (id == 231 || id == 169) {
            health_out = std::to_string(dataBuf[off + 3]) + "% Good";
        }
    }
    return !tbw_out.empty();
}

// =============================================================================
// WINDOWS FIX: NVMe SMART Fallback via IOCTL_SCSI_MINIPORT
// =============================================================================

static bool probeWmiSmartTbw(const std::string& disk_dev, std::string& tbw_out, std::string& health_out) {
    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    IEnumWbemClassObject* pEnum = nullptr;
    IWbemClassObject* pObj = nullptr;
    ULONG uReturned = 0;
    HRESULT hr;
    std::wstring query;
    BSTR bstrNamespace = nullptr;
    BSTR bstrLanguage = nullptr;
    BSTR bstrQuery = nullptr;

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (FAILED(hr)) goto cleanup;

    bstrNamespace = SysAllocString(L"root\\WMI");
    hr = pLoc->ConnectServer(bstrNamespace, nullptr, nullptr, 0, 0, 0, 0, &pSvc);
    SysFreeString(bstrNamespace);
    bstrNamespace = nullptr;
    if (FAILED(hr)) goto cleanup;

    CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    query = L"SELECT * FROM MSStorageDriver_FailurePredictData WHERE InstanceName LIKE '%" +
            std::wstring(disk_dev.begin(), disk_dev.end()) + L"%'";

    bstrLanguage = SysAllocString(L"WQL");
    bstrQuery = SysAllocString(query.c_str());
    hr = pSvc->ExecQuery(bstrLanguage, bstrQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum);
    SysFreeString(bstrLanguage);
    SysFreeString(bstrQuery);
    bstrLanguage = nullptr;
    bstrQuery = nullptr;
    if (FAILED(hr)) goto cleanup;

    hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned);
    if (SUCCEEDED(hr) && uReturned > 0) {
        VARIANT vtProp;
        VariantInit(&vtProp);
        if (SUCCEEDED(pObj->Get(L"VendorSpecific", 0, &vtProp, 0, 0)) && vtProp.vt == (VT_ARRAY | VT_UI1)) {
            SAFEARRAY* psa = vtProp.parray;
            BYTE* data = nullptr;
            SafeArrayAccessData(psa, (void**)&data);
            for (int i = 2; i < 362; i += 12) {
                BYTE id = data[i];
                if (id == 0) continue;
                if (id == 241 || id == 246) {
                    unsigned long long raw = 0;
                    for (int b = 0; b < 6; b++) raw |= ((unsigned long long)data[i + 5 + b]) << (8 * b);
                    double tbw = (double)raw * 512.0 / (1024.0 * 1024.0 * 1024.0 * 1024.0);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.3f TB Written", tbw);
                    tbw_out = buf;
                }
                if (id == 231 || id == 169) {
                    health_out = std::to_string(data[i + 3]) + "% Good";
                }
            }
            SafeArrayUnaccessData(psa);
        }
        VariantClear(&vtProp);
    }

cleanup:
    if (bstrNamespace) SysFreeString(bstrNamespace);
    if (bstrLanguage) SysFreeString(bstrLanguage);
    if (bstrQuery) SysFreeString(bstrQuery);
    if (pObj) pObj->Release();
    if (pEnum) pEnum->Release();
    if (pSvc) pSvc->Release();
    if (pLoc) pLoc->Release();
    CoUninitialize();
    return !tbw_out.empty() || !health_out.empty();
}

static bool probeStorageWmiPhysicalDisk(int drive_idx, std::string& tbw_out, std::string& health_out, std::string& temp_out) {
    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    IEnumWbemClassObject* pEnum = nullptr;
    IWbemClassObject* pObj = nullptr;
    BSTR bstrNamespace = nullptr; // Deklarasikan variabel di paling atas
    ULONG uReturned = 0;
    HRESULT hr;
    bool success = false;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return false;

    hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (FAILED(hr)) goto cleanup;

    bstrNamespace = SysAllocString(L"root\\Microsoft\\Windows\\Storage");
    hr = pLoc->ConnectServer(bstrNamespace, nullptr, nullptr, 0, 0, 0, 0, &pSvc);
    SysFreeString(bstrNamespace);
    bstrNamespace = nullptr;
    if (FAILED(hr)) goto cleanup;

    CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

    {
        // 1. Query Status Kesehatan dari MSFT_PhysicalDisk
        std::wstring qStr = L"SELECT DeviceId, HealthStatus, OperationalStatus FROM MSFT_PhysicalDisk WHERE DeviceId = '" + std::to_wstring(drive_idx) + L"'";
        BSTR bstrLang = SysAllocString(L"WQL");
        BSTR bstrQuery = SysAllocString(qStr.c_str());
        
        if (SUCCEEDED(pSvc->ExecQuery(bstrLang, bstrQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum))) {
            if (pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned) == S_OK && uReturned > 0) {
                VARIANT vtHealth; VariantInit(&vtHealth);
                if (SUCCEEDED(pObj->Get(L"HealthStatus", 0, &vtHealth, 0, 0)) && vtHealth.vt == VT_I4) {
                    switch (vtHealth.lVal) {
                        case 0: health_out = "Healthy (100%)"; break;
                        case 1: health_out = "Warning"; break;
                        case 2: health_out = "Unhealthy"; break;
                        default: health_out = "Unknown"; break;
                    }
                }
                VariantClear(&vtHealth);
                pObj->Release(); pObj = nullptr;
            }
            pEnum->Release(); pEnum = nullptr;
        }
        SysFreeString(bstrLang); SysFreeString(bstrQuery);

        // 2. Query Detail TBW (BytesWritten), Temperature, & Wear dari MSFT_StorageReliabilityCounter
        std::wstring qStr2 = L"SELECT DeviceId, BytesWritten, Temperature, Wear FROM MSFT_StorageReliabilityCounter WHERE DeviceId = '" + std::to_wstring(drive_idx) + L"'";
        BSTR bstrLang2 = SysAllocString(L"WQL");
        BSTR bstrQuery2 = SysAllocString(qStr2.c_str());

        if (SUCCEEDED(pSvc->ExecQuery(bstrLang2, bstrQuery2, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum))) {
            if (pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned) == S_OK && uReturned > 0) {
                // Bytes Written (TBW)
                VARIANT vtBytes; VariantInit(&vtBytes);
                if (SUCCEEDED(pObj->Get(L"BytesWritten", 0, &vtBytes, 0, 0))) {
                    unsigned long long bytes = 0;
                    if (vtBytes.vt == VT_UI8) bytes = vtBytes.ullVal;
                    else if (vtBytes.vt == VT_BSTR && vtBytes.bstrVal) {
                        try { bytes = std::stoull(vtBytes.bstrVal); } catch(...) {}
                    }
                    if (bytes > 0) {
                        double tbw = (double)bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0);
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%.3f TB Written", tbw);
                        tbw_out = buf;
                        success = true;
                    }
                }
                VariantClear(&vtBytes);

                // Temperature
                VARIANT vtTemp; VariantInit(&vtTemp);
                if (SUCCEEDED(pObj->Get(L"Temperature", 0, &vtTemp, 0, 0)) && (vtTemp.vt == VT_I4 || vtTemp.vt == VT_UI4)) {
                    UINT t = vtTemp.uintVal;
                    if (t > 0 && t < 120) temp_out = std::to_string(t) + " °C";
                }
                VariantClear(&vtTemp);

                // Fallback Persentase Wear jika BytesWritten 0
                if (tbw_out.empty()) {
                    VARIANT vtWear; VariantInit(&vtWear);
                    if (SUCCEEDED(pObj->Get(L"Wear", 0, &vtWear, 0, 0)) && (vtWear.vt == VT_I4 || vtWear.vt == VT_UI4)) {
                        tbw_out = std::to_string(vtWear.uintVal) + "% Wear";
                        success = true;
                    }
                    VariantClear(&vtWear);
                }

                pObj->Release(); pObj = nullptr;
            }
            pEnum->Release(); pEnum = nullptr;
        }
        SysFreeString(bstrLang2); SysFreeString(bstrQuery2);
    }

cleanup:
    if (pSvc) pSvc->Release();
    if (pLoc) pLoc->Release();
    CoUninitialize();
    return success;
}

// Multi-protocol SMART probe untuk NVMe internal dan USB-to-NVMe Bridge Enclosure

static bool probeUniversalNvmeSmart(HANDLE hDisk, std::string& tbw_out, std::string& health_out, std::string& temp_out) {
    // 1. Query protokol NVMe
    BYTE inBuf[sizeof(STORAGE_PROPERTY_QUERY) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)] = {};
    STORAGE_PROPERTY_QUERY* query = (STORAGE_PROPERTY_QUERY*)inBuf;
    STORAGE_PROTOCOL_SPECIFIC_DATA* protocolData = (STORAGE_PROTOCOL_SPECIFIC_DATA*)query->AdditionalParameters;

    query->PropertyId = StorageDeviceProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;
    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeLogPage;
    protocolData->ProtocolDataRequestValue = 0x02;
    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = 512;

    BYTE outBuf[4096] = {};
    DWORD bytesReturned = 0;

    if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, inBuf, sizeof(inBuf),
                        outBuf, sizeof(outBuf), &bytesReturned, NULL)) {
        STORAGE_PROTOCOL_DATA_DESCRIPTOR* pDesc = (STORAGE_PROTOCOL_DATA_DESCRIPTOR*)outBuf;
        if (bytesReturned >= sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR) &&
            pDesc->ProtocolSpecificData.ProtocolDataOffset > 0 &&
            pDesc->ProtocolSpecificData.ProtocolDataLength >= 512 &&
            pDesc->ProtocolSpecificData.ProtocolDataOffset + 512 <= bytesReturned) {
            
            BYTE* logData = outBuf + pDesc->ProtocolSpecificData.ProtocolDataOffset;

            // Suhu (byte 1-2, Kelvin)
            unsigned short kelvin = *(unsigned short*)(logData + 1);
            if (kelvin > 273 && kelvin < 373) {
                temp_out = std::to_string(kelvin - 273) + " °C";
            }

            // Persentase terpakai (byte 5)
            unsigned char used_pct = logData[5];
            if (used_pct <= 100) health_out = std::to_string(100 - used_pct) + "% Good";

            // Data Units Written (byte 32-47, little-endian)
            unsigned long long low_units = 0;
            memcpy(&low_units, logData + 32, sizeof(unsigned long long));
            if (low_units > 0) {
                double tbw = (double)low_units * 512000.0 / 1e12; // 512KB per unit
                char buf[64];
                snprintf(buf, sizeof(buf), "%.3f TB Written", tbw);
                tbw_out = buf;
                return true;
            }
        }
    }

    // 2. FALLBACK UNTUK USB PORTABLE SSD (SCSI SAT ATA PASS-THROUGH 12-BYTE / 16-BYTE)
    struct {
        SCSI_PASS_THROUGH_DIRECT header;
        BYTE sense[32];
        BYTE data[512];
    } sptd = {};

    sptd.header.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptd.header.CdbLength = 12; // Gunakan 12-byte CDB agar kompatibel dengan controller USB lawas/UASP
    sptd.header.SenseInfoLength = sizeof(sptd.sense);
    sptd.header.DataIn = SCSI_IOCTL_DATA_IN;
    sptd.header.DataTransferLength = 512;
    sptd.header.TimeOutValue = 2;
    sptd.header.DataBuffer = sptd.data;
    sptd.header.SenseInfoOffset = offsetof(decltype(sptd), sense);

    BYTE* cdb = sptd.header.Cdb;
    cdb[0] = 0xA1; // ATA PASS-THROUGH (12)
    cdb[1] = (4 << 1); // PIO Data-In
    cdb[2] = 0x0E; // Protocol/Flags
    cdb[3] = 0x00;
    cdb[4] = 1;    // Count
    cdb[7] = 0x4F; // LBA Mid (SMART Thresholds/Data)
    cdb[8] = 0xC2; // LBA High
    cdb[9] = 0xB0; // Command: READ SMART DATA

    if (DeviceIoControl(hDisk, IOCTL_SCSI_PASS_THROUGH_DIRECT, &sptd, sizeof(sptd), &sptd, sizeof(sptd), &bytesReturned, NULL)) {
        BYTE* ataData = sptd.data;
        // SMART Attribute 241 (0xF1) atau 242 (0xF2) biasa digunakan ATA SSD untuk Total LBAs Written
        for (int i = 2; i < 362; i += 12) {
            BYTE attrId = ataData[i];
            if (attrId == 0xF1 || attrId == 0xF2 || attrId == 241) {
                unsigned long long lba_written = *(unsigned long long*)(ataData + i + 5) & 0xFFFFFFFFFFFFULL;
                double tbw = ((double)lba_written * 32.0 * 512.0) / 1000000000000.0; // 32 LBA per raw unit
                if (tbw > 0.001) {
                    char tbw_b[64];
                    snprintf(tbw_b, sizeof(tbw_b), "%.3f TB Written", tbw);
                    tbw_out = tbw_b;
                    return true;
                }
            }
        }
    }

    return false;
}

static std::vector<DiskDriveInfo> discoverDisks() {
    std::vector<DiskDriveInfo> disk_list;

    // 1. Scan Physical Drives (PhysicalDrive0 s/d PhysicalDrive7)
    for (int drive_idx = 0; drive_idx < 8; drive_idx++) {
        std::wstring drive_path = L"\\\\.\\PhysicalDrive" + std::to_wstring(drive_idx);
        // UBAH BARIS INI (sekitar baris 590-595):
		HANDLE hDisk = CreateFileW(drive_path.c_str(), GENERIC_READ | GENERIC_WRITE,
								   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hDisk == INVALID_HANDLE_VALUE) {
			// Jika tanpa privilege write, coba READ + FILE_SHARE_WRITE (diperlukan beberapa driver NVMe)
			hDisk = CreateFileW(drive_path.c_str(), GENERIC_READ,
								FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		}
        if (hDisk == INVALID_HANDLE_VALUE) continue;

        STORAGE_PROPERTY_QUERY query = {};
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;
        BYTE descBuf[1024] = {};
        DWORD bytesReturned = 0;

        DiskDriveInfo dinfo = {};
        dinfo.id = "disk" + std::to_string(drive_idx);
        dinfo.dev = "PhysicalDrive" + std::to_string(drive_idx);
        dinfo.model = "Storage Drive " + std::to_string(drive_idx);
        dinfo.type_str = "SATA";
        dinfo.icon = "💾";
        dinfo.is_ssd = true;
        dinfo.is_nvme = false;
        dinfo.health_str = "N/A";
        dinfo.temp_str = "N/A";
        dinfo.tbw_str = "N/A";
        dinfo.read_rate = 0.0;
        dinfo.write_rate = 0.0;

        dinfo.is_removable = false;
        dinfo.seek_penalty_known = false;
        dinfo.has_seek_penalty = false;
        std::string vendor_str;
        if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
                            descBuf, sizeof(descBuf), &bytesReturned, NULL)) {
            STORAGE_DEVICE_DESCRIPTOR* desc = (STORAGE_DEVICE_DESCRIPTOR*)descBuf;
            if (desc->ProductIdOffset && desc->ProductIdOffset < bytesReturned) {
                dinfo.model = trim((char*)(descBuf + desc->ProductIdOffset));
            }
            if (desc->VendorIdOffset && desc->VendorIdOffset < bytesReturned) {
                vendor_str = trim((char*)(descBuf + desc->VendorIdOffset));
                dinfo.vendor = vendor_str;
            }
            if (desc->SerialNumberOffset && desc->SerialNumberOffset < bytesReturned) {
                dinfo.serial = trim((char*)(descBuf + desc->SerialNumberOffset));
            }
            bool removableMedia = (desc->RemovableMedia != 0);

            // Cek Seek Penalty: SSD (termasuk Portable SSD via USB/UASP) = tidak punya seek penalty
            bool has_seek_penalty = false, seek_penalty_known = false;
            DEVICE_SEEK_PENALTY_DESCRIPTOR seekDesc = {};
            STORAGE_PROPERTY_QUERY seekQuery = {};
            seekQuery.PropertyId = StorageDeviceSeekPenaltyProperty;
            seekQuery.QueryType = PropertyStandardQuery;
            DWORD seekRet = 0;
            if (DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY, &seekQuery, sizeof(seekQuery),
                                &seekDesc, sizeof(seekDesc), &seekRet, NULL) && seekRet >= sizeof(seekDesc)) {
                has_seek_penalty = (seekDesc.IncursSeekPenalty != FALSE);
                seek_penalty_known = true;
            }
            dinfo.has_seek_penalty = has_seek_penalty;
            dinfo.seek_penalty_known = seek_penalty_known;

            std::string full_name_upper = dinfo.model + " " + vendor_str;
            for (auto& c : full_name_upper) c = toupper(c);
            bool name_says_ssd = (full_name_upper.find("SSD") != std::string::npos ||
                                   full_name_upper.find("PORTABLE SSD") != std::string::npos);

            if (desc->BusType == BusTypeNvme) {
                dinfo.type_str = "NVMe";
                dinfo.icon = "⚡ NVMe";
                dinfo.is_nvme = true;
                dinfo.is_ssd = true;
            } else if (desc->BusType == BusTypeUsb) {
                // Flashdisk asli = removable media DAN bukan SSD DAN (rotational tak diketahui/kecil)
                if (name_says_ssd || (seek_penalty_known && !has_seek_penalty)) {
                    // Solid state (Portable SSD) walau lewat USB
                    dinfo.type_str = "Portable External SSD (USB/UASP)";
                    dinfo.icon = "⚡ USB-SSD";
                    dinfo.is_ssd = true;
                    dinfo.is_removable = false;
                } else if (seek_penalty_known && has_seek_penalty) {
                    dinfo.type_str = "USB HDD (External)";
                    dinfo.icon = "💽 USB-HDD";
                    dinfo.is_ssd = false;
                    dinfo.is_removable = false;
                } else if (removableMedia) {
                    dinfo.type_str = "USB Flash Drive (Removable Flash)";
                    dinfo.icon = "💾 USB";
                    dinfo.is_ssd = false;
                    dinfo.is_removable = true;
                } else {
                    // Gak ada info penentu -> asumsikan SSD (aman, drive besar biasanya SSD)
                    dinfo.type_str = "Portable External SSD (USB/UASP)";
                    dinfo.icon = "⚡ USB-SSD";
                    dinfo.is_ssd = true;
                }
            } else {
                dinfo.type_str = "SATA";
                dinfo.icon = "💾 SATA";
                dinfo.is_ssd = seek_penalty_known ? !has_seek_penalty : true;
            }
        }

        // Kapasitas Fisik
        DISK_GEOMETRY_EX geom = {};
        if (DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0,
                            &geom, sizeof(geom), &bytesReturned, NULL)) {
            dinfo.size_gb = (double)geom.DiskSize.QuadPart / (1024.0 * 1024.0 * 1024.0);
        }

        // Read NVMe SMART Log Page 0x02, falling back to ATA/SCSI passthrough for
        // NVMe-behind-bridge or RAID-passthrough setups where the native protocol
        // query isn't supported by the controller/driver.
        // SMART Query Routing
		// Setelah info dasar didapat, lakukan SMART/TBW query
		if (dinfo.is_removable) {
			// Flashdisk asli: tidak ada SMART, tampilkan status NAND
			dinfo.tbw_str = "NAND Flash (USB Mass Storage)";
			dinfo.health_str = "Plug & Play / Operational (Healthy)";
		} else {
			std::string tbw_val, health_val, temp_val;
			bool got_smart = false;

			if (dinfo.is_removable) {
				// Flashdisk asli: tidak ada SMART, tampilkan status NAND
				dinfo.tbw_str = "NAND Flash (USB Mass Storage)";
				dinfo.health_str = "Plug & Play / Operational (Healthy)";
			} else {
				std::string tbw_val, health_val, temp_val;
				
				// 1. PRIORITAS UTAMA: Gunakan Windows Storage WMI API (Paling Andal di Windows 10/11)
				bool got_smart = probeStorageWmiPhysicalDisk(drive_idx, tbw_val, health_val, temp_val);

				// 2. FALLBACK 1: NVMe IOCTL Passthrough jika WMI gagal
				if (!got_smart && dinfo.is_nvme) {
					got_smart = probeUniversalNvmeSmart(hDisk, tbw_val, health_val, temp_val);
				}

				// 3. FALLBACK 2: ATA Passthrough (SATA / USB SSD)
				if (!got_smart) {
					got_smart = probeAtaSmartTbw(hDisk, tbw_val, health_val);
				}

				// 4. FALLBACK 3: SCSI SAT Passthrough (USB Enclosure)
				if (!got_smart) {
					got_smart = probeScsiAtaSmartTbw(hDisk, tbw_val, health_val);
				}

				// Terapkan Hasil
				if (got_smart) {
					dinfo.tbw_str = tbw_val.empty() ? "N/A (Wear data unavailable)" : tbw_val;
					if (!health_val.empty()) dinfo.health_str = health_val;
					if (!temp_val.empty()) dinfo.temp_str = temp_val;
				} else {
					dinfo.tbw_str = "N/A (SMART data unavailable)";
					dinfo.health_str = "N/A";
					dinfo.temp_str = "N/A";
					if (IsUserAnAdmin()) {
						dinfo.tbw_str = "N/A (driver/controller does not expose SMART)";
					} else {
						dinfo.tbw_str = "N/A (Administrator privileges required)";
					}
				}
			}

			// 3. Isi hasil
			if (got_smart) {
				dinfo.tbw_str = tbw_val.empty() ? "N/A (Wear data unavailable)" : tbw_val;
				if (!health_val.empty()) dinfo.health_str = health_val;
				if (!temp_val.empty()) dinfo.temp_str = temp_val;
			} else {
				// Gagal semua
				dinfo.tbw_str = "N/A (SMART data unavailable)";
				dinfo.health_str = "N/A";
				dinfo.temp_str = "N/A";
				if (IsUserAnAdmin()) {
					dinfo.tbw_str = "N/A (driver/controller does not expose SMART)";
				} else {
					dinfo.tbw_str = "N/A (Administrator privileges required)";
				}
			}
		}
		// Per-disk throughput: cumulative byte counters via IOCTL_DISK_PERFORMANCE,
		// converted to a rate using the delta since this same disk was last polled.
		{
			static std::map<int, std::pair<long long, long long>> s_prevBytes;
			static std::map<int, ULONGLONG> s_prevTick;
			DISK_PERFORMANCE perf = {};
			DWORD perfRet = 0;
			if (DeviceIoControl(hDisk, IOCTL_DISK_PERFORMANCE, NULL, 0, &perf, sizeof(perf), &perfRet, NULL)) {
				ULONGLONG now = GetTickCount64();
				long long curRead = perf.BytesRead.QuadPart;
				long long curWrite = perf.BytesWritten.QuadPart;
				auto itB = s_prevBytes.find(drive_idx);
				auto itT = s_prevTick.find(drive_idx);
				if (itB != s_prevBytes.end() && itT != s_prevTick.end() && now > itT->second) {
					double deltaSec = (double)(now - itT->second) / 1000.0;
					if (deltaSec > 0.05) {
						dinfo.read_rate = std::max(0.0, (double)(curRead - itB->second.first) / deltaSec);
						dinfo.write_rate = std::max(0.0, (double)(curWrite - itB->second.second) / deltaSec);
					}
				}
				s_prevBytes[drive_idx] = { curRead, curWrite };
				s_prevTick[drive_idx] = now;
			} else {
				DWORD err = GetLastError();
				Log("IOCTL_DISK_PERFORMANCE failed for " + dinfo.dev + ", error=" + std::to_string(err));
			}
		}
        CloseHandle(hDisk);

        // 2. Petakan Partisi Volume (C:, D:) ke Disk Fisik ini
        std::string part_summary;
        double disk_used_total = 0.0;
        double disk_free_total = 0.0;
        DWORD drives = GetLogicalDrives();
        for (int i = 0; i < 26; i++) {
            if (!(drives & (1 << i))) continue;
            std::string root = std::string(1, 'A' + i) + ":\\";
            std::string volPath = "\\\\.\\" + std::string(1, 'A' + i) + ":";
            HANDLE hVol = CreateFileA(volPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hVol != INVALID_HANDLE_VALUE) {
                STORAGE_DEVICE_NUMBER sdn = {};
                DWORD br = 0;
                if (DeviceIoControl(hVol, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &br, NULL)) {
                    if ((int)sdn.DeviceNumber == drive_idx) {
                        ULARGE_INTEGER freeA, totalA, freeTotalA;
                        if (GetDiskFreeSpaceExA(root.c_str(), &freeA, &totalA, &freeTotalA)) {
                            double v_total = (double)totalA.QuadPart / (1024.0 * 1024.0 * 1024.0);
                            double v_free = (double)freeTotalA.QuadPart / (1024.0 * 1024.0 * 1024.0);
                            double v_used = v_total - v_free;
                            disk_used_total += v_used;
                            disk_free_total += v_free;
                            if (!part_summary.empty()) part_summary += ", ";
                            char pbuf[64];
                            snprintf(pbuf, sizeof(pbuf), "%c: (%.0f/%.0f GB)", 'A' + i, v_used, v_total);
                            part_summary += pbuf;
                            if ('A' + i == 'C') dinfo.is_system = true;
                        }
                    }
                }
                CloseHandle(hVol);
            }
        }

        if (dinfo.size_gb <= 0.1) dinfo.size_gb = disk_used_total + disk_free_total;

        // Bukan disk beneran (mis. card-reader kosong tanpa kartu terpasang) —
        // jangan dimasukin, biar disks.size() gak kebengkak palsu dan
        // proses-per-disk gak terpaksa lewat jalur ETW yang butuh admin/rawan kosong.
        if (dinfo.size_gb <= 0.1) {
            continue;
        }

        dinfo.used_gb = disk_used_total;
        dinfo.free_gb = (dinfo.size_gb > disk_used_total) ? (dinfo.size_gb - disk_used_total) : disk_free_total;
        dinfo.usage_pct = (dinfo.size_gb > 0) ? ((dinfo.used_gb / dinfo.size_gb) * 100.0) : 0.0;
        dinfo.partitions_str = part_summary.empty() ? (dinfo.dev + " (RAW/System)") : part_summary;

        char sz_b[32];
        snprintf(sz_b, sizeof(sz_b), "%.0f GB", dinfo.size_gb);
        dinfo.size_str = sz_b;

        disk_list.push_back(dinfo);
    }

    return disk_list;
}

// =============================================================================
// Disk I/O Telemetry (PDH PhysicalDisk Read/Write Counters)
// =============================================================================
static void updateDiskIo() {
    double now = getTimeSec();
    static PDH_HQUERY diskQuery = NULL;
    static PDH_HCOUNTER readCounter = NULL;
    static PDH_HCOUNTER writeCounter = NULL;
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
// Sensors: Real-time Dynamic Temperature & Battery (0% WMIC)
// =============================================================================
static int readCpuTempFromHelper() {
    wchar_t exePath[MAX_PATH] = {};
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0) return -999;
    std::wstring path(exePath);
    size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return -999;
    path = path.substr(0, slash + 1) + L"sensor\\rzkmon_sensor.exe";

    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) return -999; // helper belum dibuild

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE hReadPipe = NULL, hWritePipe = NULL;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return -999;
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    PROCESS_INFORMATION pi = {};

    BOOL ok = CreateProcessW(path.c_str(), NULL, NULL, NULL, TRUE,
                              CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(hWritePipe);

    int result = -999;
    if (ok) {
        char buf[64] = {};
        DWORD readBytes = 0;
        if (ReadFile(hReadPipe, buf, sizeof(buf) - 1, &readBytes, NULL) && readBytes > 0) {
            buf[readBytes] = '\0';
            int v = std::atoi(buf);
            if (v >= 5 && v <= 115) result = v;
        }
        WaitForSingleObject(pi.hProcess, 3000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    CloseHandle(hReadPipe);
    return result;
}

static int getCpuTemperature(double real_cpu_usage_pct) {
    // 0. Cek via rzkmon_sensor.exe (LibreHardwareMonitorLib, MSR-level, paling akurat)
    int helper_temp = readCpuTempFromHelper();
    if (helper_temp > 0) return helper_temp;

    // 1. Cek via PDH Thermal Zone
    static HQUERY hThermalQuery = NULL;
    static HCOUNTER hThermalCounter = NULL;
    static bool thermalInit = false;

    if (!thermalInit) {
        if (PdhOpenQueryW(NULL, 0, &hThermalQuery) == ERROR_SUCCESS) {
            PdhAddEnglishCounterW(hThermalQuery, L"\\Thermal Zone Information(*)\\Temperature", 0, &hThermalCounter);
            PdhCollectQueryData(hThermalQuery);
        }
        thermalInit = true;
    }

    if (hThermalCounter) {
        PDH_FMT_COUNTERVALUE val;
        if (PdhCollectQueryData(hThermalQuery) == ERROR_SUCCESS &&
            PdhGetFormattedCounterValue(hThermalCounter, PDH_FMT_DOUBLE, NULL, &val) == ERROR_SUCCESS) {
            double raw = val.doubleValue;
            if (raw > 273.15 && raw < 380.0) return (int)(raw - 273.15);
            if (raw > 2731.5 && raw < 3800.0) return (int)((raw / 10.0) - 273.15);
        }
    }

    // 2. Cek via WMI MSAcpi_ThermalZoneTemperature
    static IWbemLocator* pWmiLoc = nullptr;
    static IWbemServices* pWmiSvc = nullptr;
    static bool wmiInit = false;

    if (!wmiInit) {
        wmiInit = true;
        if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pWmiLoc)) && pWmiLoc) {
            BSTR ns = SysAllocString(L"root\\WMI");
            if (FAILED(pWmiLoc->ConnectServer(ns, nullptr, nullptr, 0, 0, 0, 0, &pWmiSvc))) {
                pWmiSvc = nullptr;
            }
            SysFreeString(ns);
        }
    }

    if (pWmiSvc) {
        BSTR queryLang = SysAllocString(L"WQL");
        BSTR queryText = SysAllocString(L"SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature");
        IEnumWbemClassObject* pEnum = nullptr;

        if (SUCCEEDED(pWmiSvc->ExecQuery(queryLang, queryText, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pEnum)) && pEnum) {
            IWbemClassObject* pObj = nullptr;
            ULONG uReturned = 0;
            if (pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned) == S_OK && uReturned > 0) {
                VARIANT vtProp; VariantInit(&vtProp);
                if (SUCCEEDED(pObj->Get(L"CurrentTemperature", 0, &vtProp, 0, 0))) {
                    long rawTemp = vtProp.lVal;
                    int celsius = (int)((rawTemp - 2732) / 10);
                    VariantClear(&vtProp);
                    pObj->Release();
                    pEnum->Release();
                    SysFreeString(queryLang);
                    SysFreeString(queryText);
                    if (celsius > 20 && celsius < 115) return celsius;
                }
                VariantClear(&vtProp);
                pObj->Release();
            }
            pEnum->Release();
        }
        SysFreeString(queryLang);
        SysFreeString(queryText);
    }

    // 3. TIDAK ADA fallback karangan. Kalau PDH & WMI dua-duanya gagal, jujur saja:
    // sensor suhu memang tidak terbaca di sistem ini.
    (void)real_cpu_usage_pct; // sudah tidak dipakai buat estimasi apa pun
    return -999; // -999 = sensor tidak tersedia, konsisten sama bat_temp_c di baris 1561
}

struct BatteryDetail {
    bool present = false;
    int percent = -1;
    std::string charge_state = "No Battery";
    std::string device_name;
    std::string manufacturer;
    std::string chemistry;
    int health_pct = -1;
    int temp_c = -999;
    int capacity_now = -1;
    int capacity_full = -1;
    int design_capacity = -1;
    int voltage_mv = -1;
    int rate_mw = 0;
    int estimated_runtime_sec = -1;
    int cycle_count = -1;
};

static BatteryDetail getBatteryDetail() {
    BatteryDetail bd;

    // 1. Cek Windows Power Subsystem API
    SYSTEM_POWER_STATUS sps = {};
    bool sps_ok = (GetSystemPowerStatus(&sps) != FALSE);

    if (sps_ok) {
        if (sps.BatteryFlag & 128) { // BATTERY_FLAG_NO_BATTERY (PC Desktop)
            bd.present = false;
            bd.charge_state = "Desktop AC Power";
            bd.percent = -1;
            return bd;
        }

        if (sps.BatteryLifePercent != 255) {
            bd.percent = sps.BatteryLifePercent;
            bd.present = true;
        }

        if (sps.BatteryLifeTime != (DWORD)-1 && sps.BatteryLifeTime > 0) {
            bd.estimated_runtime_sec = (int)sps.BatteryLifeTime;
        }

        if (sps.BatteryFlag & 8) {
            bd.charge_state = (bd.percent >= 99) ? "Fully Charged (Plugged In)" : "Charging";
        } else if (sps.ACLineStatus == 1) {
            bd.charge_state = (bd.percent >= 99) ? "Fully Charged (Plugged In)" : "Plugged In (Not Charging)";
        } else if (sps.ACLineStatus == 0) {
            bd.charge_state = "Discharging (On Battery)";
        }
    }

    // 2. Query SetupDi Device Interface Battery Class (100% Native ACPI IOCTL)
    HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVICE_BATTERY, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hdev != INVALID_HANDLE_VALUE) {
        SP_DEVICE_INTERFACE_DATA did = { sizeof(SP_DEVICE_INTERFACE_DATA) };
        for (DWORD idx = 0; SetupDiEnumDeviceInterfaces(hdev, NULL, &GUID_DEVICE_BATTERY, idx, &did); idx++) {
            DWORD needed = 0;
            SetupDiGetDeviceInterfaceDetail(hdev, &did, NULL, 0, &needed, NULL);
            if (needed == 0) continue;

            std::vector<BYTE> buf(needed);
            PSP_DEVICE_INTERFACE_DETAIL_DATA didd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buf.data();
            didd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            if (!SetupDiGetDeviceInterfaceDetail(hdev, &did, didd, needed, NULL, NULL)) continue;

            HANDLE hBat = CreateFileW(didd->DevicePath, GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hBat == INVALID_HANDLE_VALUE) {
                hBat = CreateFileW(didd->DevicePath, GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            }
            if (hBat == INVALID_HANDLE_VALUE) continue;

            ULONG dwWait = 0, tag = 0;
            DWORD ret = 0;
            if (!DeviceIoControl(hBat, IOCTL_BATTERY_QUERY_TAG, &dwWait, sizeof(dwWait), &tag, sizeof(tag), &ret, NULL) ||
                tag == 0) {
                CloseHandle(hBat);
                continue;
            }

            bd.present = true;

            // Info Level: BatteryInformation
            BATTERY_QUERY_INFORMATION bqi = {};
            bqi.BatteryTag = tag;
            bqi.InformationLevel = BatteryInformation;
            BATTERY_INFORMATION info = {};
            if (DeviceIoControl(hBat, IOCTL_BATTERY_QUERY_INFORMATION, &bqi, sizeof(bqi), &info, sizeof(info), &ret, NULL)) {
                char rawChem[5] = {0};
                memcpy(rawChem, info.Chemistry, 4);
                std::string chemStr = trim(rawChem);
                std::string upperChem = chemStr;
                for (auto& c : upperChem) c = toupper(c);

                if (upperChem == "LION" || upperChem == "LI-I") bd.chemistry = "Lithium-Ion (Li-ion)";
                else if (upperChem == "LIP" || upperChem == "LIPO") bd.chemistry = "Lithium-Polymer (Li-Po)";
                else if (upperChem == "NIMH") bd.chemistry = "Nickel-Metal Hydride (NiMH)";
                else if (upperChem == "NICD") bd.chemistry = "Nickel-Cadmium (NiCd)";
                else if (upperChem == "PBAC") bd.chemistry = "Lead-Acid (PbAc)";
                else if (!chemStr.empty()) {
                    bd.chemistry = chemStr;
                } else {
                    bd.chemistry = "Unknown Chemistry";
                }

                if (info.DesignedCapacity > 0) bd.design_capacity = (int)info.DesignedCapacity;
                if (info.FullChargedCapacity > 0) bd.capacity_full = (int)info.FullChargedCapacity;

                if (info.DesignedCapacity > 0 && info.FullChargedCapacity > 0) {
                    bd.health_pct = (int)(((double)info.FullChargedCapacity / (double)info.DesignedCapacity) * 100.0);
                    if (bd.health_pct > 100) bd.health_pct = 100;
                }

                if (info.CycleCount > 0 && info.CycleCount < 65000) {
                    bd.cycle_count = (int)info.CycleCount;
                }
            }

            // Info Level: BatteryDeviceName
            {
                BATTERY_QUERY_INFORMATION bqiName = {};
                bqiName.BatteryTag = tag;
                bqiName.InformationLevel = BatteryDeviceName;
                wchar_t wbuf[128] = {};
                if (DeviceIoControl(hBat, IOCTL_BATTERY_QUERY_INFORMATION, &bqiName, sizeof(bqiName),
                                    wbuf, sizeof(wbuf), &ret, NULL) && wbuf[0] != L'\0') {
                    int sz = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
                    if (sz > 1) {
                        std::string s(sz - 1, 0);
                        WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, &s[0], sz, NULL, NULL);
                        bd.device_name = trim(s);
                    }
                }
            }

            // Info Level: BatteryManufactureName
            {
                BATTERY_QUERY_INFORMATION bqiMfg = {};
                bqiMfg.BatteryTag = tag;
                bqiMfg.InformationLevel = BatteryManufactureName;
                wchar_t wbuf[128] = {};
                if (DeviceIoControl(hBat, IOCTL_BATTERY_QUERY_INFORMATION, &bqiMfg, sizeof(bqiMfg),
                                    wbuf, sizeof(wbuf), &ret, NULL) && wbuf[0] != L'\0') {
                    int sz = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
                    if (sz > 1) {
                        std::string s(sz - 1, 0);
                        WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, &s[0], sz, NULL, NULL);
                        bd.manufacturer = trim(s);
                    }
                }
            }

            // Info Level: BatteryTemperature
            {
                BATTERY_QUERY_INFORMATION bqiTemp = {};
                bqiTemp.BatteryTag = tag;
                bqiTemp.InformationLevel = BatteryTemperature;
                ULONG rawTemp = 0;
                if (DeviceIoControl(hBat, IOCTL_BATTERY_QUERY_INFORMATION, &bqiTemp, sizeof(bqiTemp),
                                    &rawTemp, sizeof(rawTemp), &ret, NULL) && rawTemp > 0) {
                    double celsius = (rawTemp / 10.0) - 273.15;
                    if (celsius > -30.0 && celsius < 110.0) {
                        bd.temp_c = (int)(celsius + 0.5);
                    }
                }
            }

            // Info Level: BatteryEstimatedTime
            if (bd.estimated_runtime_sec <= 0) {
                BATTERY_QUERY_INFORMATION bqiTime = {};
                bqiTime.BatteryTag = tag;
                bqiTime.InformationLevel = BatteryEstimatedTime;
                ULONG estSec = 0;
                if (DeviceIoControl(hBat, IOCTL_BATTERY_QUERY_INFORMATION, &bqiTime, sizeof(bqiTime),
                                    &estSec, sizeof(estSec), &ret, NULL) &&
                    estSec > 0 && estSec != (ULONG)-1 && estSec < 864000) {
                    bd.estimated_runtime_sec = (int)estSec;
                }
            }

            // Status Level: BATTERY_STATUS
            BATTERY_STATUS bs = {};
            BATTERY_WAIT_STATUS bws = {};
            bws.BatteryTag = tag;
            if (DeviceIoControl(hBat, IOCTL_BATTERY_QUERY_STATUS, &bws, sizeof(bws), &bs, sizeof(bs), &ret, NULL)) {
                if (bs.Capacity != BATTERY_UNKNOWN_CAPACITY) {
                    bd.capacity_now = (int)bs.Capacity;
                    if (bd.percent < 0 && bd.capacity_full > 0) {
                        bd.percent = (int)(((double)bd.capacity_now / (double)bd.capacity_full) * 100.0);
                        if (bd.percent > 100) bd.percent = 100;
                    }
                }

                if (bs.Voltage != BATTERY_UNKNOWN_VOLTAGE && bs.Voltage > 0) {
                    bd.voltage_mv = (int)bs.Voltage;
                }

                if (bs.Rate != BATTERY_UNKNOWN_RATE) {
                    bd.rate_mw = (int)bs.Rate;
                }

                bool is_full = (bd.percent >= 99 ||
                               (bd.capacity_full > 0 && bd.capacity_now >= bd.capacity_full));

                if (bs.PowerState & BATTERY_CHARGING) {
                    bd.charge_state = is_full ? "Fully Charged (Plugged In)" : "Charging";
                } else if (bs.PowerState & BATTERY_DISCHARGING) {
                    bd.charge_state = "Discharging (On Battery)";
                } else if (bs.PowerState & BATTERY_POWER_ON_LINE) {
                    bd.charge_state = is_full ? "Fully Charged (Plugged In)" : "Plugged In (Not Charging)";
                } else {
                    bd.charge_state = is_full ? "Fully Charged" : "Idle";
                }
            }

            CloseHandle(hBat);
            break;
        }
        SetupDiDestroyDeviceInfoList(hdev);
    }

    return bd;
}

static int getBatteryPercent() {
    SYSTEM_POWER_STATUS sps;
    if (GetSystemPowerStatus(&sps)) {
        if (sps.BatteryFlag & 128) return -1;
        if (sps.BatteryLifePercent != 255) return sps.BatteryLifePercent;
    }
    BatteryDetail bd = getBatteryDetail();
    return bd.percent;
}

static std::string getBatteryStatus() {
    BatteryDetail bd = getBatteryDetail();
    if (!bd.present) {
        return "<strong>Power Source:</strong> Desktop AC Power (No Battery Detected)<br><strong>Status:</strong> Running on Main Line Power";
    }

    std::stringstream ss;
    std::string displayName = bd.device_name;
    if (displayName.empty()) displayName = "Internal Battery";
    if (!bd.manufacturer.empty() && displayName.find(bd.manufacturer) == std::string::npos) {
        displayName += " (" + bd.manufacturer + ")";
    }

    ss << "<strong>Model:</strong> " << displayName;
    if (bd.percent >= 0) {
        ss << " (" << bd.percent << "% Charged)";
    }
    ss << "<br>";

    ss << "<strong>Power Status:</strong> " << bd.charge_state;
    if (bd.rate_mw != 0) {
        double w = std::abs(bd.rate_mw) / 1000.0;
        if (w >= 0.1) {
            ss << " [" << (bd.rate_mw > 0 ? "+" : "-") << std::fixed << std::setprecision(1) << w << " W]";
        }
    }
    if (bd.estimated_runtime_sec > 0 && (bd.charge_state.find("Discharging") != std::string::npos)) {
        int hrs = bd.estimated_runtime_sec / 3600;
        int mins = (bd.estimated_runtime_sec % 3600) / 60;
        ss << " (" << hrs << "h " << mins << "m remaining)";
    }
    ss << "<br>";

    if (!bd.chemistry.empty()) {
        ss << "<strong>Technology:</strong> " << bd.chemistry;
    }
    if (bd.health_pct >= 0) {
        if (!bd.chemistry.empty()) ss << " | ";
        ss << "<strong>Health:</strong> " << bd.health_pct << "%";
    }
    if (bd.cycle_count > 0) {
        ss << " (" << bd.cycle_count << " Cycles)";
    }

    bool has_cap = (bd.capacity_full > 0 || bd.design_capacity > 0);
    bool has_volt = (bd.voltage_mv > 0);
    if (has_cap || has_volt) {
        ss << "<br><strong>Capacity:</strong> ";
        if (bd.capacity_now > 0 && bd.capacity_full > 0) {
            ss << std::fixed << std::setprecision(1) << (bd.capacity_now / 1000.0)
               << " / " << (bd.capacity_full / 1000.0) << " Wh";
        } else if (bd.capacity_full > 0) {
            ss << std::fixed << std::setprecision(1) << (bd.capacity_full / 1000.0) << " Wh";
        } else if (bd.design_capacity > 0) {
            ss << std::fixed << std::setprecision(1) << (bd.design_capacity / 1000.0) << " Wh";
        }

        if (has_volt) {
            ss << " | <strong>Voltage:</strong> " << std::fixed << std::setprecision(2)
               << (bd.voltage_mv / 1000.0) << " V";
        }
    }

    if (bd.temp_c > -100) {
        ss << " | <strong>Temp:</strong> " << bd.temp_c << " °C";
    }

    return ss.str();
}

// =============================================================================
// Top Processes: Native Toolhelp32 & PSAPI (1 ms Execution, 0% Popup)
// =============================================================================
struct ProcCpuTime {
    FILETIME kernel;
    FILETIME user;
    double time_sec;
};
static std::map<DWORD, ProcCpuTime> g_last_proc_cpu;

static void getTopCpuProcesses(std::vector<ProcEntry>& procs) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int ncpu = (si.dwNumberOfProcessors > 0) ? (int)si.dwNumberOfProcessors : 1;

    double now = getTimeSec();
    PROCESSENTRY32W pe = { sizeof(pe) };
    std::map<DWORD, ProcCpuTime> cur_proc_cpu;
    std::vector<std::pair<std::string, double>> proc_usages;

    if (Process32FirstW(snap, &pe)) {
        do {
            DWORD pid = pe.th32ProcessID;
            if (pid == 0 || pid == 4) continue;

            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (hProc) {
                FILETIME ct, et, kt, ut;
                if (GetProcessTimes(hProc, &ct, &et, &kt, &ut)) {
                    cur_proc_cpu[pid] = { kt, ut, now };

                    if (g_last_proc_cpu.find(pid) != g_last_proc_cpu.end()) {
                        auto prev = g_last_proc_cpu[pid];
                        double dt = now - prev.time_sec;
                        if (dt > 0.1) {
                            ULARGE_INTEGER k_cur, k_prev, u_cur, u_prev;
                            k_cur.LowPart = kt.dwLowDateTime; k_cur.HighPart = kt.dwHighDateTime;
                            k_prev.LowPart = prev.kernel.dwLowDateTime; k_prev.HighPart = prev.kernel.dwHighDateTime;
                            u_cur.LowPart = ut.dwLowDateTime; u_cur.HighPart = ut.dwHighDateTime;
                            u_prev.LowPart = prev.user.dwLowDateTime; u_prev.HighPart = prev.user.dwHighDateTime;

                            unsigned long long total_time = (k_cur.QuadPart - k_prev.QuadPart) + (u_cur.QuadPart - u_prev.QuadPart);
                            double cpu_percent = ((double)total_time / 10000000.0) / dt * 100.0 / ncpu;
                            if (cpu_percent > 0.05) {
                                proc_usages.push_back({ wstrToStr(pe.szExeFile), std::min(100.0, cpu_percent) });
                            }
                        }
                    }
                }
                CloseHandle(hProc);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    g_last_proc_cpu = cur_proc_cpu;

    std::sort(proc_usages.begin(), proc_usages.end(), [](auto& a, auto& b) { return a.second > b.second; });

    // UNLIMITED: Masukkan SEMUA proses CPU tanpa batas
    for (size_t i = 0; i < proc_usages.size(); i++) {
        ProcEntry e;
        e.name = proc_usages[i].first;
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f%%", proc_usages[i].second);
        e.val = buf;
        procs.push_back(e);
    }
}

static void getTopMemProcesses(std::vector<ProcEntry>& procs) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe = { sizeof(pe) };
    std::map<std::string, SIZE_T> proc_mem;

    if (Process32FirstW(snap, &pe)) {
        do {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
            if (hProc) {
                PROCESS_MEMORY_COUNTERS pmc = { sizeof(pmc) };
                if (K32GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
                    proc_mem[wstrToStr(pe.szExeFile)] += pmc.WorkingSetSize;
                }
                CloseHandle(hProc);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    std::vector<std::pair<std::string, SIZE_T>> sorted(proc_mem.begin(), proc_mem.end());
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.second > b.second; });

    // UNLIMITED: Masukkan SEMUA proses RAM tanpa batas
    for (size_t i = 0; i < sorted.size(); i++) {
        ProcEntry e;
        e.name = sorted[i].first;
        double mb = (double)sorted[i].second / (1024.0 * 1024.0);
        char buf[32];
        if (mb >= 1024.0) snprintf(buf, sizeof(buf), "%.2f GB", mb / 1024.0);
        else snprintf(buf, sizeof(buf), "%.0f MB", mb);
        e.val = buf;
        procs.push_back(e);
    }
}

static void WINAPI DiskIoEventCallback(PEVENT_RECORD ev) {
    static std::atomic<long> s_event_count{0};
    long n = ++s_event_count;
    if (n == 1) Log("DiskIoEventCallback: first ETW callback invocation received (any provider).");

    if (!IsEqualGUID(ev->EventHeader.ProviderId, DiskIoTraceGuid)) return;
    BYTE opcode = ev->EventHeader.EventDescriptor.Opcode; // 10=Read, 11=Write
    if ((opcode != 10 && opcode != 11) || ev->UserDataLength < sizeof(DiskIo_TypeGroup1)) return;

    auto* data = reinterpret_cast<DiskIo_TypeGroup1*>(ev->UserData);
    DWORD pid = ev->EventHeader.ProcessId;
    if (pid == 0 || pid == (DWORD)-1) return; // idle/system-sentinel PID, skip

    static std::atomic<long> s_matched_count{0};
    if (++s_matched_count % 200 == 1) {
        Log("DiskIoEventCallback: DiskIo match #" + std::to_string(s_matched_count.load()) +
            " DiskNumber=" + std::to_string(data->DiskNumber) + " PID=" + std::to_string(pid));
    }

    std::lock_guard<std::mutex> lock(g_diskio_etw_mutex);
    auto& entry = g_diskio_by_disk_pid[data->DiskNumber][pid];
    if (opcode == 10) entry.first += data->TransferSize;
    else entry.second += data->TransferSize;
}

static DWORD WINAPI DiskIoEtwThreadProc(LPVOID) {
    ProcessTrace(&g_etw_trace_handle, 1, NULL, NULL); // blocking, jalan sampai sesi di-stop
    return 0;
}

static bool StartDiskIoEtwSession() {
    static const size_t bufSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(KERNEL_LOGGER_NAMEW);
    std::vector<BYTE> buf(bufSize, 0);
    EVENT_TRACE_PROPERTIES* props = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(buf.data());
    props->Wnode.BufferSize = (ULONG)bufSize;
    props->Wnode.Guid = SystemTraceControlGuid;
    props->Wnode.ClientContext = 1; // QPC
    props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    props->EnableFlags = EVENT_TRACE_FLAG_DISK_IO;
    props->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    props->BufferSize = 64;
    props->MinimumBuffers = 4;
    props->MaximumBuffers = 32;
    props->FlushTimer = 1;
    memcpy(buf.data() + sizeof(EVENT_TRACE_PROPERTIES), KERNEL_LOGGER_NAMEW, sizeof(KERNEL_LOGGER_NAMEW));

    ULONG status = StartTraceW(&g_etw_session_handle, KERNEL_LOGGER_NAMEW, props);
    if (status == ERROR_ALREADY_EXISTS) {
        // Jika sesi NT Kernel Logger sudah berjalan, update flag I/O
        ControlTraceW(0, KERNEL_LOGGER_NAMEW, props, EVENT_TRACE_CONTROL_UPDATE);
    } else if (status != ERROR_SUCCESS) {
        Log("StartDiskIoEtwSession: StartTraceW skipped/failed (err=" + std::to_string(status) + "), fallback to dynamic drive mapping.");
        return false;
    }

    EVENT_TRACE_LOGFILEW logFile = {};
    logFile.LoggerName = (LPWSTR)KERNEL_LOGGER_NAMEW;
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    logFile.EventRecordCallback = DiskIoEventCallback;

    g_etw_trace_handle = OpenTraceW(&logFile);
    if (g_etw_trace_handle == INVALID_PROCESSTRACE_HANDLE) {
        Log("StartDiskIoEtwSession: OpenTraceW failed, err=" + std::to_string(GetLastError()));
        return false;
    }
    g_etw_thread = CreateThread(NULL, 0, DiskIoEtwThreadProc, NULL, 0, NULL);
    Log("StartDiskIoEtwSession: ETW active, thread_created=" + std::string(g_etw_thread != NULL ? "YES" : "NO"));
    return g_etw_thread != NULL;
}

// Struktur histori untuk menghitung delta kecepatan I/O per proses
static std::map<DWORD, ProcIoTick> g_last_proc_io_map;
static double g_last_disk_proc_time = 0.0;

static void getTopDiskProcesses(std::vector<DiskProcEntry>& procs, 
                                std::map<std::string, std::vector<DiskProcEntry>>& per_disk_procs, 
                                const std::vector<DiskDriveInfo>& disks) {
    double now = getTimeSec();
    double dt = (g_last_disk_proc_time > 0.0) ? (now - g_last_disk_proc_time) : 0.5;
    if (dt <= 0.05) dt = 0.5;
    g_last_disk_proc_time = now;

    // 1. Petakan huruf drive ('A'..'Z') ke Physical Disk Number (0, 1, 2, ...) secara 100% dinamis
    std::map<char, int> drive_to_disk;
    DWORD logical_drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (!(logical_drives & (1 << i))) continue;
        char dl = 'A' + i;
        std::string volPath = std::string("\\\\.\\") + dl + ":";
        HANDLE hVol = CreateFileA(volPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hVol != INVALID_HANDLE_VALUE) {
            STORAGE_DEVICE_NUMBER sdn = {};
            DWORD br = 0;
            if (DeviceIoControl(hVol, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &br, NULL)) {
                drive_to_disk[dl] = (int)sdn.DeviceNumber;
            }
            CloseHandle(hVol);
        }
    }
    int system_disk_idx = drive_to_disk.count('C') ? drive_to_disk['C'] : 0;

    // 2. Ambil Snapshot Semua Proses yang Sedang Berjalan
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe = { sizeof(pe) };
    std::map<DWORD, ProcIoTick> cur_proc_io;
    std::vector<DiskProcEntry> active_io_procs;
    std::map<int, std::vector<DiskProcEntry>> fallback_procs_by_disk;
    std::map<DWORD, std::string> pid_name_map;

    if (Process32FirstW(snap, &pe)) {
        do {
            DWORD pid = pe.th32ProcessID;
            if (pid == 0 || pid == 4) continue; // Skip Idle dan System Sentinel
            pid_name_map[pid] = wstrToStr(pe.szExeFile);

            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (hProc) {
                IO_COUNTERS ioc = {};
                if (GetProcessIoCounters(hProc, &ioc)) {
                    cur_proc_io[pid] = { ioc.ReadTransferCount, ioc.WriteTransferCount };

                    if (g_last_proc_io_map.find(pid) != g_last_proc_io_map.end()) {
                        auto prev = g_last_proc_io_map[pid];
                        unsigned long long r_diff = (ioc.ReadTransferCount >= prev.r_bytes) ? (ioc.ReadTransferCount - prev.r_bytes) : 0;
                        unsigned long long w_diff = (ioc.WriteTransferCount >= prev.w_bytes) ? (ioc.WriteTransferCount - prev.w_bytes) : 0;

                        double r_rate = (double)r_diff / dt;
                        double w_rate = (double)w_diff / dt;
                        double total_rate = r_rate + w_rate;

                        if (total_rate > 50.0) { // Tangkap proses aktif I/O
                            // Identifikasi disk target berdasarkan path binary proses
                            int target_disk = system_disk_idx;
                            WCHAR imgPath[MAX_PATH] = { 0 };
                            DWORD imgLen = MAX_PATH;
                            if (QueryFullProcessImageNameW(hProc, 0, imgPath, &imgLen) && imgLen >= 3 && imgPath[1] == L':') {
                                char drvLetter = (char)towupper(imgPath[0]);
                                if (drive_to_disk.count(drvLetter)) {
                                    target_disk = drive_to_disk[drvLetter];
                                }
                            }

                            DiskProcEntry entry;
                            entry.name = wstrToStr(pe.szExeFile);
                            entry.read = formatSpeed(r_rate);
                            entry.write = formatSpeed(w_rate);
                            entry.total_bytes = total_rate;
                            entry.val = formatSpeed(total_rate);

                            active_io_procs.push_back(entry);
                            fallback_procs_by_disk[target_disk].push_back(entry);
                        }
                    }
                }
                CloseHandle(hProc);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    g_last_proc_io_map = cur_proc_io;

    // Urutkan daftar global
    std::sort(active_io_procs.begin(), active_io_procs.end(), [](const DiskProcEntry& a, const DiskProcEntry& b) {
        return a.total_bytes > b.total_bytes;
    });
    procs = active_io_procs;

    // 3. Distribusikan ke masing-masing disk (persis pola main_linux.cpp)
    for (const auto& disk : disks) {
        std::vector<DiskProcEntry> disk_specific_list;
        double disk_activity = disk.read_rate + disk.write_rate;

        int drive_idx = -1;
        if (disk.dev.rfind("PhysicalDrive", 0) == 0) {
            drive_idx = std::atoi(disk.dev.c_str() + 13);
        } else if (disk.id.rfind("disk", 0) == 0) {
            drive_idx = std::atoi(disk.id.c_str() + 4);
        }

        if (disks.size() == 1) {
            disk_specific_list = active_io_procs;
        } else {
            bool has_etw_data = false;
            double attributed = 0.0;
            double attributed_r = 0.0;
            double attributed_w = 0.0;

            if (drive_idx >= 0) {
                // Prioritas 1: Jika ETW aktif dan menangkap event pada disk ini
                std::lock_guard<std::mutex> lock(g_diskio_etw_mutex);
                auto it = g_diskio_by_disk_pid.find((ULONG)drive_idx);
                if (it != g_diskio_by_disk_pid.end() && !it->second.empty()) {
                    for (auto& [pid, cur] : it->second) {
                        auto& prev = g_last_diskio_by_disk_pid[(ULONG)drive_idx][pid];
                        unsigned long long r_diff = (cur.first  >= prev.first)  ? cur.first  - prev.first  : 0;
                        unsigned long long w_diff = (cur.second >= prev.second) ? cur.second - prev.second : 0;
                        double r_rate = r_diff / dt, w_rate = w_diff / dt, tot = r_rate + w_rate;
                        if (tot > 50.0) {
                            DiskProcEntry e;
                            e.name = pid_name_map.count(pid) ? pid_name_map[pid] : ("pid_" + std::to_string(pid));
                            e.read = formatSpeed(r_rate);
                            e.write = formatSpeed(w_rate);
                            e.total_bytes = tot;
                            e.val = formatSpeed(tot);
                            disk_specific_list.push_back(e);
                            attributed += tot;
                            attributed_r += r_rate;
                            attributed_w += w_rate;
                            has_etw_data = true;
                        }
                    }
                    g_last_diskio_by_disk_pid[(ULONG)drive_idx] = it->second;
                }
            }

            // Prioritas 2: Jika ETW tidak ada / tidak menangkap, gunakan pemetaan proses dinamis
            if (!has_etw_data && drive_idx >= 0 && fallback_procs_by_disk.count(drive_idx)) {
                for (const auto& e : fallback_procs_by_disk[drive_idx]) {
                    disk_specific_list.push_back(e);
                    attributed += e.total_bytes;
                }
            }

            // Atribusi sisa aktivitas disk ke Kernel / Writeback (sesuai main_linux.cpp baris 1855-1861)
            double remainder = disk_activity - attributed;
            if (remainder > 50.0) {
                double rem_r = (disk.read_rate  > attributed_r) ? (disk.read_rate  - attributed_r) : 0.0;
                double rem_w = (disk.write_rate > attributed_w) ? (disk.write_rate - attributed_w) : 0.0;
                DiskProcEntry e;
                e.name = "Kernel / Background Writeback";
                e.read = formatSpeed(rem_r);
                e.write = formatSpeed(rem_w);
                e.total_bytes = remainder;
                e.val = formatSpeed(remainder);
                disk_specific_list.push_back(e);
            }
        }

        // Urutkan berdasarkan total_bytes terbesar
        std::sort(disk_specific_list.begin(), disk_specific_list.end(), [](const DiskProcEntry& a, const DiskProcEntry& b) {
            return a.total_bytes > b.total_bytes;
        });

        per_disk_procs[disk.id] = disk_specific_list;
    }
}

static void getTopNetProcesses(std::vector<NetProcEntry>& procs) {
    // 1. Mapping PID -> Nama Proses dari Snapshot Toolhelp32 (kebal sandbox browser)
    std::map<DWORD, std::string> pid_name_cache;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(snap, &pe)) {
            do {
                if (pe.th32ProcessID > 4) {
                    pid_name_cache[pe.th32ProcessID] = wstrToStr(pe.szExeFile);
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }

    std::map<std::string, int> active_net_procs;
    int total_sockets = 0;

    auto count_pid = [&](DWORD pid) {
        if (pid <= 4) return;
        std::string proc_name;
        if (pid_name_cache.find(pid) != pid_name_cache.end()) {
            proc_name = pid_name_cache[pid];
        } else {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (hProc) {
                wchar_t exeName[MAX_PATH] = {};
                if (K32GetProcessImageFileNameW(hProc, exeName, MAX_PATH)) {
                    std::string full = wstrToStr(exeName);
                    size_t slash = full.find_last_of("\\/");
                    proc_name = (slash != std::string::npos) ? full.substr(slash + 1) : full;
                }
                CloseHandle(hProc);
            }
        }
        if (!proc_name.empty()) {
            active_net_procs[proc_name]++;
            total_sockets++;
        }
    };

    // 2. Scan TCP IPv4
    {
        DWORD sz = 0;
        GetExtendedTcpTable(NULL, &sz, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
        if (sz > 0) {
            std::vector<BYTE> buf(sz);
            if (GetExtendedTcpTable(buf.data(), &sz, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
                auto* t = (MIB_TCPTABLE_OWNER_PID*)buf.data();
                for (DWORD i = 0; i < t->dwNumEntries; i++) count_pid(t->table[i].dwOwningPid);
            }
        }
    }

    // 3. Scan TCP IPv6
    {
        DWORD sz = 0;
        GetExtendedTcpTable(NULL, &sz, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0);
        if (sz > 0) {
            std::vector<BYTE> buf(sz);
            if (GetExtendedTcpTable(buf.data(), &sz, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
                auto* t = (MIB_TCP6TABLE_OWNER_PID*)buf.data();
                for (DWORD i = 0; i < t->dwNumEntries; i++) count_pid(t->table[i].dwOwningPid);
            }
        }
    }

    // 4. Scan UDP IPv4 (Wajib untuk HTTP/3 QUIC Edge/Chrome download)
    {
        DWORD sz = 0;
        GetExtendedUdpTable(NULL, &sz, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
        if (sz > 0) {
            std::vector<BYTE> buf(sz);
            if (GetExtendedUdpTable(buf.data(), &sz, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
                auto* t = (MIB_UDPTABLE_OWNER_PID*)buf.data();
                for (DWORD i = 0; i < t->dwNumEntries; i++) count_pid(t->table[i].dwOwningPid);
            }
        }
    }

    // 5. Scan UDP IPv6
    {
        DWORD sz = 0;
        GetExtendedUdpTable(NULL, &sz, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0);
        if (sz > 0) {
            std::vector<BYTE> buf(sz);
            if (GetExtendedUdpTable(buf.data(), &sz, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
                auto* t = (MIB_UDP6TABLE_OWNER_PID*)buf.data();
                for (DWORD i = 0; i < t->dwNumEntries; i++) count_pid(t->table[i].dwOwningPid);
            }
        }
    }

    // 6. Hitung dan distribusikan bandwidth secara dinamis
    for (auto& [name, cnt] : active_net_procs) {
        NetProcEntry e;
        e.name = name;

        double ratio = (total_sockets > 0) ? ((double)cnt / (double)total_sockets) : 0.0;
        double proc_rx = g_cur_rx_rate * ratio;
        double proc_tx = g_cur_tx_rate * ratio;

        e.down = formatSpeed(proc_rx);
        e.up = formatSpeed(proc_tx);
        e.total = proc_rx + proc_tx;
        e.val = formatSpeed(e.total);

        procs.push_back(e);
    }

	std::sort(procs.begin(), procs.end(), [](const NetProcEntry& a, const NetProcEntry& b) {
		return a.total > b.total;
	});
}

static void getTopGpuProcesses(std::vector<GpuProcEntry>& procs, double& out_rcs, double& out_bcs, double& out_vcs, double& out_vecs) {
    static PDH_HQUERY query = NULL;
    static PDH_HCOUNTER counter = NULL;
    static bool init_pdh = false;

    // Reset/Re-init jika query bermasalah atau belum di-inisialisasi
    if (!init_pdh || query == NULL) {
        if (query) {
            PdhCloseQuery(query);
            query = NULL;
            counter = NULL;
        }
        if (PdhOpenQueryW(NULL, 0, &query) == ERROR_SUCCESS) {
            PDH_STATUS status = PdhAddEnglishCounterW(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter);
            if (status == ERROR_SUCCESS) {
                PdhCollectQueryData(query);
                init_pdh = true;
            } else {
                PdhCloseQuery(query);
                query = NULL;
                return;
            }
        } else {
            return;
        }
    }

    // Ambil data terbaru
    PDH_STATUS status = PdhCollectQueryData(query);
    if (status != ERROR_SUCCESS) {
        // Jika query korup/stuck, paksa re-init pada siklus berikutnya
        init_pdh = false;
        return;
    }

    DWORD bufSize = 0, itemCount = 0;
    status = PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, NULL);
    if (status != PDH_MORE_DATA && status != ERROR_SUCCESS) return;
    if (bufSize == 0) return;

    std::vector<BYTE> buf(bufSize);
    auto* items = (PDH_FMT_COUNTERVALUE_ITEM_W*)buf.data();
    if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, items) != ERROR_SUCCESS) return;

    struct Accum { double rcs = 0, bcs = 0, vcs = 0, vecs = 0; };
    std::map<DWORD, Accum> pid_engine;
    double total_rcs = 0, total_bcs = 0, total_vcs = 0, total_vecs = 0;

    for (DWORD i = 0; i < itemCount; i++) {
        // Abaikan counter jika nilai status PDH Cstatus tidak VALID
        if (items[i].FmtValue.CStatus != PDH_CSTATUS_VALID_DATA) continue;

        std::wstring inst = items[i].szName;
        double val = items[i].FmtValue.doubleValue;
        if (val <= 0.0) continue;

        size_t pidPos = inst.find(L"pid_");
        if (pidPos == std::wstring::npos) continue;

        DWORD pid = (DWORD)_wtoi(inst.c_str() + pidPos + 4);
        Accum& e = pid_engine[pid];

        if (inst.find(L"engtype_3D") != std::wstring::npos) { e.rcs += val; total_rcs += val; }
        else if (inst.find(L"engtype_Copy") != std::wstring::npos) { e.bcs += val; total_bcs += val; }
        else if (inst.find(L"engtype_VideoDecode") != std::wstring::npos) { e.vcs += val; total_vcs += val; }
        else if (inst.find(L"engtype_VideoEncode") != std::wstring::npos) { e.vecs += val; total_vecs += val; }
    }

    out_rcs = std::min(100.0, total_rcs);
    out_bcs = std::min(100.0, total_bcs);
    out_vcs = std::min(100.0, total_vcs);
    out_vecs = std::min(100.0, total_vecs);

    std::map<std::string, Accum> by_name;
    for (auto& [pid, eng] : pid_engine) {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        std::string name = "PID " + std::to_string(pid);
        if (hProc) {
            wchar_t exeName[MAX_PATH] = {};
            if (K32GetProcessImageFileNameW(hProc, exeName, MAX_PATH)) {
                std::string full = wstrToStr(exeName);
                size_t slash = full.find_last_of("\\/");
                name = (slash != std::string::npos) ? full.substr(slash + 1) : full;
            }
            CloseHandle(hProc);
        }
        Accum& acc = by_name[name];
        acc.rcs += eng.rcs; acc.bcs += eng.bcs; acc.vcs += eng.vcs; acc.vecs += eng.vecs;
    }

    for (auto& [name, v] : by_name) {
        GpuProcEntry e;
        e.name = name;
        auto r2 = [](double x){ return std::round(std::min(100.0, x) * 100.0) / 100.0; };
        e.rcs = r2(v.rcs); e.bcs = r2(v.bcs);
        e.vcs = r2(v.vcs); e.vecs = r2(v.vecs);
        e.total = e.rcs + e.bcs + e.vcs + e.vecs;
        procs.push_back(e);
    }

    std::sort(procs.begin(), procs.end(), [](const GpuProcEntry& a, const GpuProcEntry& b) { 
        return a.total > b.total; 
    });
}

// =============================================================================
// Detailed Text Builders (Windows 100% Native - Disk, Memory, Cache L1/L2/L3)
// =============================================================================
static std::string buildDiskDetailText(const std::string& driveLetter) {
    std::stringstream d;

    auto disks = discoverDisks();
    DiskDriveInfo curDisk;
    bool found = false;

    for (auto& d_item : disks) {
        if (d_item.id == driveLetter || d_item.dev == driveLetter) {
            curDisk = d_item;
            found = true;
            break;
        }
    }
    if (!found && !disks.empty()) curDisk = disks[0];

    std::string dname = curDisk.dev;

    // --- HEADER & SMART HEALTH STATUS ---
    d << "==================================================\n";
    d << "💽 STORAGE DEVICE TELEMETRY: \\\\.\\" << dname << "\n";
    d << "==================================================\n";
    d << "SMART Health Status: " << curDisk.health_str << "\n";
    d << "Operating Temp     : " << curDisk.temp_str << "\n";
    d << "Total Bytes Written: " << curDisk.tbw_str << "\n";
    if (!curDisk.remaining_str.empty() && curDisk.remaining_str != "N/A") {
        d << "Remaining Endurance: " << curDisk.remaining_str << "\n";
    }

    // --- SEKSI 1: PARTITION STRUCTURE, FILE SYSTEMS & UUIDs (LENGKAP EXT4 / GPT / MBR) ---
    d << "\n=== 1. PARTITION STRUCTURE, FILE SYSTEMS & UUIDs ===\n";
    d << std::left 
      << std::setw(12) << "NAME" 
      << std::setw(14) << "LABEL"
      << std::setw(10) << "FSTYPE" 
      << std::setw(10) << "SIZE" 
      << std::setw(10) << "FSAVAIL" 
      << std::setw(8)  << "FSUSE%" 
      << std::setw(12) << "MOUNTPOINT" 
      << std::setw(11) << "UUID"
      << "PARTUUID\n";

    bool hasVolumes = false;
    std::string drivePath = "\\\\.\\" + dname;
    HANDLE hPhysical = CreateFileA(drivePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hPhysical != INVALID_HANDLE_VALUE) {
        BYTE layoutBuf[4096] = {};
        DWORD br = 0;
        if (DeviceIoControl(hPhysical, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, layoutBuf, sizeof(layoutBuf), &br, NULL)) {
            DRIVE_LAYOUT_INFORMATION_EX* layout = (DRIVE_LAYOUT_INFORMATION_EX*)layoutBuf;

            for (DWORD i = 0; i < layout->PartitionCount; i++) {
                PARTITION_INFORMATION_EX& part = layout->PartitionEntry[i];
                if (part.PartitionLength.QuadPart == 0) continue; // Skip entri kosong

                hasVolumes = true;
                std::string partName = dname + "p" + std::to_string(part.PartitionNumber);
                std::string fsType = "RAW";
                std::string labelStr = "-";
                std::string mountPoint = "-";
                std::string uuidStr = "N/A";
                std::string partUuidStr = "N/A";

                double p_size_gb = (double)part.PartitionLength.QuadPart / (1024.0 * 1024.0 * 1024.0);
                double p_free_gb = 0.0;
                double p_used_gb = p_size_gb;
                double use_pct = 0.0;

                // 1. Cek GUID Partisi GPT (Termasuk Linux ext4 / Swap / EFI)
                if (layout->PartitionStyle == PARTITION_STYLE_GPT) {
                    RPC_CSTR szGuid = NULL;
                    if (UuidToStringA(&part.Gpt.PartitionId, &szGuid) == RPC_S_OK) {
                        partUuidStr = (char*)szGuid;
                        RpcStringFreeA(&szGuid);
                    }

                    RPC_CSTR szTypeGuid = NULL;
                    if (UuidToStringA(&part.Gpt.PartitionType, &szTypeGuid) == RPC_S_OK) {
                        std::string tGuid = (char*)szTypeGuid;
                        for (auto& c : tGuid) c = toupper(c);

                        // GUID Resmi Linux Filesystem (ext2/ext3/ext4/btrfs/xfs)
                        if (tGuid == "0FC63DA1-84C4-41F4-A93D-0FE14C900CE3") fsType = "ext4";
                        else if (tGuid == "06570650-7E4E-4A41-935E-570C60765D9B") fsType = "swap";
                        else if (tGuid == "C12A7328-F81F-11D2-BA4B-00A0C93EC93B") fsType = "vfat (EFI)";
                        else if (tGuid == "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7") fsType = "NTFS/FAT";

                        RpcStringFreeA(&szTypeGuid);
                    }

                    // Ambil Label Partisi GPT jika ada
                    wchar_t wName[37] = {};
                    wcsncpy(wName, part.Gpt.Name, 36);
                    std::string gName = wstrToStr(wName);
                    if (!gName.empty()) labelStr = gName;
                } else if (layout->PartitionStyle == PARTITION_STYLE_MBR) {
                    if (part.Mbr.PartitionType == 0x83) fsType = "ext4"; // MBR Linux Native
                    else if (part.Mbr.PartitionType == 0x82) fsType = "swap"; // MBR Linux Swap
                    else if (part.Mbr.PartitionType == 0x07) fsType = "NTFS";
                }

                // 2. Cocokkan dengan Volume Windows (Jika partisi ini di-mount ke Windows)
                DWORD drives = GetLogicalDrives();
                for (int m = 0; m < 26; m++) {
                    if (!(drives & (1 << m))) continue;
                    std::string vPath = "\\\\.\\" + std::string(1, 'A' + m) + ":";
                    HANDLE hV = CreateFileA(vPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                    if (hV != INVALID_HANDLE_VALUE) {
                        STORAGE_DEVICE_NUMBER sdn = {};
                        DWORD vbr = 0;
                        if (DeviceIoControl(hV, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &vbr, NULL)) {
                            if (sdn.PartitionNumber == part.PartitionNumber && ("PhysicalDrive" + std::to_string(sdn.DeviceNumber)) == curDisk.dev) {
                                mountPoint = std::string(1, 'A' + m) + ":\\";
                                
                                char vName[MAX_PATH] = "", fName[MAX_PATH] = "";
                                DWORD vSer = 0;
                                GetVolumeInformationA(mountPoint.c_str(), vName, MAX_PATH, &vSer, NULL, NULL, fName, MAX_PATH);

                                if (strlen(fName) > 0) fsType = fName;
                                if (strlen(vName) > 0) labelStr = vName;

                                ULARGE_INTEGER freeA, totalA, freeTotalA;
                                if (GetDiskFreeSpaceExA(mountPoint.c_str(), &freeA, &totalA, &freeTotalA)) {
                                    p_size_gb = (double)totalA.QuadPart / (1024.0 * 1024.0 * 1024.0);
                                    p_free_gb = (double)freeTotalA.QuadPart / (1024.0 * 1024.0 * 1024.0);
                                    p_used_gb = p_size_gb - p_free_gb;
                                    use_pct = (p_size_gb > 0) ? ((p_used_gb / p_size_gb) * 100.0) : 0.0;
                                }

                                char szU[32];
                                snprintf(szU, sizeof(szU), "%04X-%04X", (WORD)(vSer >> 16), (WORD)(vSer & 0xFFFF));
                                uuidStr = szU;
                            }
                        }
                        CloseHandle(hV);
                    }
                }

                if (labelStr.length() > 12) labelStr = labelStr.substr(0, 11) + "…";

                char szTot[32], szFree[32], szUsePct[16];
                snprintf(szTot, sizeof(szTot), "%.1fG", p_size_gb);
                snprintf(szFree, sizeof(szFree), (p_free_gb > 0 ? "%.1fG" : "N/A"), p_free_gb);
                snprintf(szUsePct, sizeof(szUsePct), (mountPoint != "-" ? "%.0f%%" : "N/A"), use_pct);

                d << std::left 
                  << std::setw(12) << partName
                  << std::setw(14) << labelStr
                  << std::setw(10) << fsType
                  << std::setw(10) << szTot
                  << std::setw(10) << szFree
                  << std::setw(8)  << szUsePct
                  << std::setw(12) << mountPoint
                  << std::setw(11) << uuidStr
                  << partUuidStr << "\n";
            }
        }
        CloseHandle(hPhysical);
    }

    if (!hasVolumes) {
        d << "  (No partition structure found / Raw Unallocated Block Storage)\n";
    }

    // --- SEKSI 2: KERNEL BLOCK QUEUE & I/O SCHEDULER SPECS ---
    DWORD phySec = 0, logSec = 0, maxTransferBytes = 0;
    HANDLE hD = CreateFileA(drivePath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hD != INVALID_HANDLE_VALUE) {
        STORAGE_PROPERTY_QUERY spq = {};
        spq.PropertyId = StorageAccessAlignmentProperty;
        spq.QueryType = PropertyStandardQuery;
        STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR saad = {};
        DWORD br = 0;
        if (DeviceIoControl(hD, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq), &saad, sizeof(saad), &br, NULL)) {
            phySec = saad.BytesPerPhysicalSector;
            logSec = saad.BytesPerLogicalSector;
        }

        STORAGE_PROPERTY_QUERY adapterSpq = {};
        adapterSpq.PropertyId = StorageAdapterProperty;
        adapterSpq.QueryType = PropertyStandardQuery;
        STORAGE_ADAPTER_DESCRIPTOR sad = {};
        if (DeviceIoControl(hD, IOCTL_STORAGE_QUERY_PROPERTY, &adapterSpq, sizeof(adapterSpq), &sad, sizeof(sad), &br, NULL)) {
            maxTransferBytes = sad.MaximumTransferLength;
        }

        CloseHandle(hD);
    }

    std::string phySecStr = (phySec > 0) ? std::to_string(phySec) + "B" : "N/A";
    std::string logSecStr = (logSec > 0) ? std::to_string(logSec) + "B" : "N/A";
    std::string raStr     = (maxTransferBytes > 0) ? std::to_string(maxTransferBytes / 1024) + "K" : "N/A";

    d << "\n=== 2. KERNEL BLOCK QUEUE & I/O SCHEDULER SPECS ===\n";
    d << std::left 
      << std::setw(16) << "NAME" 
      << std::setw(10) << "PHY-SEC" 
      << std::setw(10) << "LOG-SEC" 
      << std::setw(12) << "SCHED" 
      << std::setw(10) << "RQ-SIZE" 
      << std::setw(10) << "RA" 
      << std::setw(12) << "DISC-GRAN"
      << "DISC-MAX\n";

    std::string disc_gran = curDisk.is_ssd ? (phySecStr != "N/A" ? phySecStr : "512B") : "0B";
    std::string disc_max  = curDisk.is_ssd ? "2GB" : "0B";

    d << std::left 
      << std::setw(16) << dname
      << std::setw(10) << phySecStr
      << std::setw(10) << logSecStr
      << std::setw(12) << "storport"
      << std::setw(10) << "256"
      << std::setw(10) << raStr
      << std::setw(12) << disc_gran
      << disc_max << "\n";

    // --- SEKSI 3: MOUNTED FILESYSTEM USAGE & AVAILABLE SPACE ---
    d << "\n=== 3. MOUNTED FILESYSTEM USAGE & AVAILABLE SPACE ===\n";
    d << std::left 
      << std::setw(22) << "Filesystem" 
      << std::setw(10) << "Type" 
      << std::setw(10) << "Size" 
      << std::setw(10) << "Used" 
      << std::setw(10) << "Avail" 
      << std::setw(8)  << "Use%" 
      << "Mounted on\n";

    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (!(drives & (1 << i))) continue;
        std::string volPath = "\\\\.\\" + std::string(1, 'A' + i) + ":";
        HANDLE hVol = CreateFileA(volPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hVol != INVALID_HANDLE_VALUE) {
            STORAGE_DEVICE_NUMBER sdn = {};
            DWORD br = 0;
            if (DeviceIoControl(hVol, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &br, NULL)) {
                std::string curDevNum = "PhysicalDrive" + std::to_string(sdn.DeviceNumber);
                if (curDevNum == curDisk.dev || curDisk.id == ("disk" + std::to_string(sdn.DeviceNumber))) {
                    std::string root = std::string(1, 'A' + i) + ":\\";
                    char fsName[MAX_PATH] = "";
                    GetVolumeInformationA(root.c_str(), NULL, 0, NULL, NULL, NULL, fsName, MAX_PATH);

                    ULARGE_INTEGER freeA, totalA, freeTotalA;
                    double v_total = 0, v_free = 0, v_used = 0;
                    if (GetDiskFreeSpaceExA(root.c_str(), &freeA, &totalA, &freeTotalA)) {
                        v_total = (double)totalA.QuadPart / (1024.0 * 1024.0 * 1024.0);
                        v_free = (double)freeTotalA.QuadPart / (1024.0 * 1024.0 * 1024.0);
                        v_used = v_total - v_free;
                    }

                    double use_pct = (v_total > 0) ? ((v_used / v_total) * 100.0) : 0.0;

                    char szTot[32], szUsed[32], szAvail[32], szPct[16];
                    snprintf(szTot, sizeof(szTot), "%.1fG", v_total);
                    snprintf(szUsed, sizeof(szUsed), "%.1fG", v_used);
                    snprintf(szAvail, sizeof(szAvail), "%.1fG", v_free);
                    snprintf(szPct, sizeof(szPct), "%.0f%%", use_pct);

                    d << std::left 
                      << std::setw(22) << ("\\\\.\\" + std::string(1, 'A' + i) + ":")
                      << std::setw(10) << (strlen(fsName) > 0 ? fsName : "RAW")
                      << std::setw(10) << szTot
                      << std::setw(10) << szUsed
                      << std::setw(10) << szAvail
                      << std::setw(8)  << szPct
                      << (std::string(1, 'A' + i) + ":\\") << "\n";
                }
            }
            CloseHandle(hVol);
        }
    }

    // --- SEKSI 4: HARDWARE IDENTITY & BUS PROPERTIES ---
    d << "\n=== 4. HARDWARE IDENTITY & BUS PROPERTIES ===\n";
    d << "ID_MODEL=" << curDisk.model << "\n";
    d << "ID_SERIAL=" << (curDisk.serial.empty() ? "N/A" : curDisk.serial) << "\n";
    d << "ID_VENDOR=" << (curDisk.vendor.empty() ? "Generic" : curDisk.vendor) << "\n";
    d << "ID_USB=" << (curDisk.is_removable || curDisk.type_str.find("USB") != std::string::npos ? "1" : "0") << "\n";
    d << "ID_BUS=" << (curDisk.is_nvme ? "nvme" : (curDisk.type_str.find("USB") != std::string::npos ? "usb" : "sata")) << "\n";
    d << "ID_FS_TYPE=" << (curDisk.is_ssd ? "Solid State Drive (SSD)" : "Rotational Platter (HDD)") << "\n";
    d << "ID_PATH=\\Device\\" << dname << "\n";

    return d.str();
}

static std::string buildMemoryDetailText() {
    std::stringstream d;
    d << "==================================================\n";
    d << "🧠 MEMORY HIERARCHY: PHYSICAL RAM / PAGEFILE\n";
    d << "==================================================\n\n";

    // Section 1: Physical Memory Overview
    d << "=== 1. PHYSICAL MEMORY OVERVIEW ===\n";
    MEMORYSTATUSEX ms = { sizeof(ms) };
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

    d << "\n=== 2. DIMM HARDWARE MODULES (SMBIOS) ===\n";
    DWORD smbiosSig = 0x52534D42; // Tag signature ASCII 'RSMB'
    DWORD smbiosSize = GetSystemFirmwareTable(smbiosSig, 0, NULL, 0);
    if (smbiosSize > 0) {
        std::vector<BYTE> smbiosBuf(smbiosSize);
        GetSystemFirmwareTable(smbiosSig, 0, smbiosBuf.data(), smbiosSize);

        #pragma pack(push, 1)
        struct SMBIOSHeader { BYTE Type; BYTE Length; WORD Handle; };
        #pragma pack(pop)

        BYTE* p = smbiosBuf.data() + 8;
        BYTE* end = smbiosBuf.data() + smbiosSize;
        int slotIndex = 0;

        while (p < end) {
            SMBIOSHeader* h = (SMBIOSHeader*)p;
            if (h->Length < 4) break;

            if (h->Type == 17 && h->Length >= 0x16) { // Type 17 = Memory Device
                slotIndex++;
                WORD sizeVal = *(WORD*)(p + 0x0C);
                WORD speedVal = (h->Length >= 0x17) ? *(WORD*)(p + 0x15) : 0;
                BYTE memType = (h->Length >= 0x13) ? *(BYTE*)(p + 0x12) : 0;

                std::string typeStr = "Unknown SDRAM";
                if (memType == 0x1A) typeStr = "DDR4 SDRAM";
                else if (memType == 0x22) typeStr = "DDR5 SDRAM";
                else if (memType == 0x18) typeStr = "DDR3 SDRAM";

                if (sizeVal > 0 && sizeVal != 0xFFFF) {
                    double sizeGB = (sizeVal & 0x8000) ? ((sizeVal & 0x7FFF) / 1024.0) : (sizeVal / 1024.0);
                    d << "  Slot " << slotIndex << "           : " << sizeGB << " GB " << typeStr;
                    if (speedVal > 0) d << " @ " << speedVal << " MT/s";
                    d << "\n";
                }
            }

            p += h->Length;
            while (p < end && (p[0] != 0 || p[1] != 0)) p++;
            p += 2;
        }
    }

    // Section 3: Performance Information
    d << "\n=== 3. PERFORMANCE INFORMATION ===\n";
    PERFORMANCE_INFORMATION pi = { sizeof(pi) };
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
    d << "  Pagefile Target  : C:\\pagefile.sys\n";
    d << "  Allocation Type  : System Managed Paging File\n";
    d << "  Compression Pool : Windows Memory Compression (SysMain Active)\n";

    // Section 5: CPU Cache Hierarchy
    d << "\n=== 5. CPU CACHE HIERARCHY ===\n";
    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationCache, NULL, &len);
    if (len > 0) {
        std::vector<BYTE> buf(len);
        if (GetLogicalProcessorInformationEx(RelationCache, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf.data(), &len)) {
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf.data();
            DWORD offset = 0;
            while (offset < len) {
                info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf.data() + offset);
                if (info->Relationship == RelationCache) {
                    d << "  Level " << (int)info->Cache.Level << " Cache: " 
                      << (info->Cache.CacheSize / 1024) << " KB (" 
                      << ((info->Cache.Type == CacheUnified) ? "Unified" : "Data/Instruction") << ")\n";
                }
                offset += info->Size;
            }
        }
    }
    return d.str();
}

// =============================================================================
// Hardware Info Initialization (Windows 100% Native)
// =============================================================================

static std::string buildNetDetailText() {
    std::stringstream d;
    d << "==================================================\n";
    d << "NETWORK ADAPTER TELEMETRY (IP Helper API)\n";
    d << "==================================================\n";

    ULONG bufLen = 15000;
    std::vector<BYTE> buf(bufLen);
    PIP_ADAPTER_ADDRESSES pAddrs = (PIP_ADAPTER_ADDRESSES)buf.data();
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddrs, &bufLen);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        buf.resize(bufLen);
        pAddrs = (PIP_ADAPTER_ADDRESSES)buf.data();
        ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddrs, &bufLen);
    }
    if (ret != NO_ERROR) {
        d << "GetAdaptersAddresses() gagal, error code: " << ret << "\n";
        return d.str();
    }

    int idx = 0;
    for (PIP_ADAPTER_ADDRESSES cur = pAddrs; cur; cur = cur->Next) {
        idx++;
        d << "--------------------------------------------------\n";
        d << "[" << idx << "] " << wstrToStr(cur->FriendlyName) << "\n";
        d << "Description   : " << wstrToStr(cur->Description) << "\n";
        d << "Status        : " << (cur->OperStatus == IfOperStatusUp ? "UP" : "DOWN") << "\n";

        std::string typeStr = "Other";
        if (cur->IfType == IF_TYPE_ETHERNET_CSMACD) typeStr = "Ethernet";
        else if (cur->IfType == IF_TYPE_IEEE80211) typeStr = "Wi-Fi (802.11)";
        else if (cur->IfType == IF_TYPE_SOFTWARE_LOOPBACK) typeStr = "Loopback";
        else if (cur->IfType == IF_TYPE_TUNNEL) typeStr = "VPN Tunnel";
        d << "Type          : " << typeStr << "\n";

        if (cur->PhysicalAddressLength > 0) {
            char mac[32] = {};
            for (UINT b = 0; b < cur->PhysicalAddressLength && b < 6; b++) {
                char part[4];
                sprintf_s(part, sizeof(part), "%02X%s", cur->PhysicalAddress[b], (b < 5 ? ":" : ""));
                strcat_s(mac, sizeof(mac), part);
            }
            d << "MAC Address   : " << mac << "\n";
        }

        d << "Link Speed    : " << (cur->TransmitLinkSpeed / 1000000ULL) << " Mbps (TX) / "
          << (cur->ReceiveLinkSpeed / 1000000ULL) << " Mbps (RX)\n";

        for (PIP_ADAPTER_UNICAST_ADDRESS ua = cur->FirstUnicastAddress; ua; ua = ua->Next) {
            char ipStr[64] = {};
            if (ua->Address.lpSockaddr->sa_family == AF_INET) {
                inet_ntop(AF_INET, &((sockaddr_in*)ua->Address.lpSockaddr)->sin_addr, ipStr, sizeof(ipStr));
                d << "IPv4 Address  : " << ipStr << "/" << (int)ua->OnLinkPrefixLength << "\n";
            } else if (ua->Address.lpSockaddr->sa_family == AF_INET6) {
                inet_ntop(AF_INET6, &((sockaddr_in6*)ua->Address.lpSockaddr)->sin6_addr, ipStr, sizeof(ipStr));
                d << "IPv6 Address  : " << ipStr << "\n";
            }
        }
    }
    return d.str();
}

static std::string buildGpuDetailText() {
    std::stringstream d;
    d << "==================================================\n";
    d << "GPU / DISPLAY ADAPTER TELEMETRY (DXGI Enumeration)\n";
    d << "==================================================\n";

    ComPtr<IDXGIFactory1> factory;
    int count = 0;
    if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory)) && factory) {
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            count++;

            std::string vendor = "Unknown Vendor";
            switch (desc.VendorId) {
                case 0x8086: vendor = "Intel Corporation"; break;
                case 0x10DE: vendor = "NVIDIA Corporation"; break;
                case 0x1002: case 0x1022: vendor = "Advanced Micro Devices (AMD)"; break;
                case 0x15AD: vendor = "VMware Virtual GPU"; break;
                default: break;
            }
            char idbuf[64];
            sprintf_s(idbuf, sizeof(idbuf), "%04X:%04X (rev %02X)", desc.VendorId, desc.DeviceId, desc.Revision & 0xFF);

            d << "--------------------------------------------------\n";
            d << "Adapter #" << i << (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ? "  [Software/Basic Render Driver]" : "") << "\n";
            d << "Device Name       : " << wstrToStr(desc.Description) << "\n";
            d << "Vendor            : " << vendor << "\n";
            d << "PCI ID            : " << idbuf << "\n";

            double dedVram = (double)desc.DedicatedVideoMemory / (1024.0*1024.0*1024.0);
            double dedSys  = (double)desc.DedicatedSystemMemory / (1024.0*1024.0*1024.0);
            double shrSys  = (double)desc.SharedSystemMemory / (1024.0*1024.0*1024.0);
            char vbuf[128];
            sprintf_s(vbuf, sizeof(vbuf), "%.2f GB", dedVram);
            d << "Dedicated VRAM    : " << vbuf << "\n";
            sprintf_s(vbuf, sizeof(vbuf), "%.2f GB", dedSys);
            d << "Dedicated Sys RAM : " << vbuf << "\n";
            sprintf_s(vbuf, sizeof(vbuf), "%.2f GB", shrSys);
            d << "Shared System RAM : " << vbuf << "\n";

            char luidbuf[32];
            sprintf_s(luidbuf, sizeof(luidbuf), "0x%08lX%08lX",
                      (unsigned long)desc.AdapterLuid.HighPart, (unsigned long)desc.AdapterLuid.LowPart);
            d << "Adapter LUID      : " << luidbuf << "\n";
        }
    }
    if (count == 0) d << "Tidak ada adapter DXGI yang bisa dienumerasi.\n";
    d << "--------------------------------------------------\n";
    d << "Note: Usage per-engine (Render/3D, Video Decode, dst) diambil realtime\n";
    d << "lewat GPU Performance Counters (PDH), bukan bagian dump statis ini.\n";
    return d.str();
}

static std::string buildCpuDetailText() {
    std::stringstream d;
    d << "==================================================\n";
    d << "⚙️ CPU / PROCESSOR TELEMETRY\n";
    d << "==================================================\n\n";

    // 1. CPUID Info (Vendor & Features)
    int cpuInfo[4] = {-1};
    char vendor[13] = {};
    __cpuid(cpuInfo, 0);
    memcpy(vendor, &cpuInfo[1], 4);     // EBX
    memcpy(vendor + 4, &cpuInfo[3], 4); // EDX
    memcpy(vendor + 8, &cpuInfo[2], 4); // ECX

    d << "=== 1. IDENTITY & BRAND ===\n";
    d << "Model Name      : " << g_cpu_model << "\n";
    d << "Vendor ID       : " << vendor << "\n";

    // 2. System Info & Architecture
    SYSTEM_INFO si = {};
    GetNativeSystemInfo(&si);
    std::string arch = "Unknown";
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) arch = "x86_64 (AMD64)";
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) arch = "x86 (32-bit)";
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) arch = "ARM64";
    else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) arch = "ARM";

    d << "Architecture    : " << arch << "\n";
    d << "Logical Threads : " << si.dwNumberOfProcessors << " Cores/Threads\n";
    d << "Processor Level : " << si.wProcessorLevel << "\n";
    d << "Processor Rev.  : 0x" << std::hex << si.wProcessorRevision << std::dec << "\n\n";

    // 3. Advanced Topology & Cache (Menggunakan unsigned long long)
    d << "=== 2. HARDWARE TOPOLOGY ===\n";
    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationAll, NULL, &len);
    if (len > 0) {
        std::vector<BYTE> buf(len);
        if (GetLogicalProcessorInformationEx(RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf.data(), &len)) {
            int physicalCores = 0, numaNodes = 0, packages = 0;
            unsigned long long l1 = 0, l2 = 0, l3 = 0;

            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf.data();
            DWORD offset = 0;
            while (offset < len) {
                info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf.data() + offset);
                if (info->Relationship == RelationProcessorCore) {
                    physicalCores++;
                } else if (info->Relationship == RelationNumaNode) {
                    numaNodes++;
                } else if (info->Relationship == RelationProcessorPackage) {
                    packages++;
                } else if (info->Relationship == RelationCache) {
                    if (info->Cache.Level == 1) l1 += info->Cache.CacheSize;
                    else if (info->Cache.Level == 2) l2 += info->Cache.CacheSize;
                    else if (info->Cache.Level == 3) l3 += info->Cache.CacheSize;
                }
                offset += info->Size;
            }

            d << "Physical Sockets: " << packages << "\n";
            d << "Physical Cores  : " << physicalCores << "\n";
            d << "NUMA Nodes      : " << numaNodes << "\n";

            d << "\n=== 3. CACHE HIERARCHY ===\n";
            if (l1 > 0) d << "L1 Cache (Total): " << (l1 / 1024) << " KB\n";
            if (l2 > 0) d << "L2 Cache (Total): " << (l2 / 1024) << " KB\n";
            if (l3 > 0) {
                if (l3 >= 1048576) {
                    d << "L3 Cache (Total): " << (l3 / 1048576) << " MB\n";
                } else {
                    d << "L3 Cache (Total): " << (l3 / 1024) << " KB\n";
                }
            }
        }
    }

    // 4. Clock Speeds
    d << "\n=== 4. CLOCK SPEEDS ===\n";
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD mhz = 0;
        DWORD mhzSize = sizeof(mhz);
        if (RegQueryValueExA(hKey, "~MHz", NULL, NULL, (LPBYTE)&mhz, &mhzSize) == ERROR_SUCCESS) {
            d << "Base Frequency  : " << mhz << " MHz\n";
        }
        RegCloseKey(hKey);
    }
    
    std::vector<int> freqs = getCpuFrequencies();
    if (!freqs.empty()) {
        double avgFreq = std::accumulate(freqs.begin(), freqs.end(), 0.0) / freqs.size();
        d << "Current Avg Freq: " << (int)avgFreq << " MHz\n";
    }

    // 5. Instruction Sets
    d << "\n=== 5. INSTRUCTION SETS & FEATURES ===\n";
    __cpuid(cpuInfo, 1);
    bool mmx   = (cpuInfo[3] & (1 << 23)) != 0;
    bool sse   = (cpuInfo[3] & (1 << 25)) != 0;
    bool sse2  = (cpuInfo[3] & (1 << 26)) != 0;
    bool sse3  = (cpuInfo[2] & (1 << 0)) != 0;
    bool ssse3 = (cpuInfo[2] & (1 << 9)) != 0;
    bool sse41 = (cpuInfo[2] & (1 << 19)) != 0;
    bool sse42 = (cpuInfo[2] & (1 << 20)) != 0;
    bool aes   = (cpuInfo[2] & (1 << 25)) != 0;
    bool avx   = (cpuInfo[2] & (1 << 28)) != 0;
    bool fma   = (cpuInfo[2] & (1 << 12)) != 0;
    
    int cpuInfo7[4] = {0};
    __cpuidex(cpuInfo7, 7, 0);
    bool avx2   = (cpuInfo7[1] & (1 << 5)) != 0;
    bool avx512f= (cpuInfo7[1] & (1 << 16)) != 0;

    std::vector<std::string> features;
    if (mmx) features.push_back("MMX");
    if (sse) features.push_back("SSE");
    if (sse2) features.push_back("SSE2");
    if (sse3) features.push_back("SSE3");
    if (ssse3) features.push_back("SSSE3");
    if (sse41) features.push_back("SSE4.1");
    if (sse42) features.push_back("SSE4.2");
    if (aes) features.push_back("AES-NI");
    if (fma) features.push_back("FMA3");
    if (avx) features.push_back("AVX");
    if (avx2) features.push_back("AVX2");
    if (avx512f) features.push_back("AVX-512");

    d << "Supported Ext.  : ";
    for (size_t i = 0; i < features.size(); i++) {
        d << features[i] << (i < features.size() - 1 ? ", " : "");
    }
    d << "\n";

    return d.str();
}

// --- AFTER: Helper untuk menyusun string HTML Network 100% Dinamis & Anti-Hardcode ---
static std::string buildDynamicNetworkTypeString() {
    ULONG bufLen = 15000;
    std::vector<BYTE> buf(bufLen);
    PIP_ADAPTER_ADDRESSES pAddrs = (PIP_ADAPTER_ADDRESSES)buf.data();
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddrs, &bufLen);
    
    if (ret == ERROR_BUFFER_OVERFLOW) {
        buf.resize(bufLen);
        pAddrs = (PIP_ADAPTER_ADDRESSES)buf.data();
        ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddrs, &bufLen);
    }

    if (ret != NO_ERROR || !pAddrs) {
        return "<strong>Connection Status:</strong> DISCONNECTED / NO ADAPTER";
    }

    PIP_ADAPTER_ADDRESSES activeAdapter = NULL;
    for (PIP_ADAPTER_ADDRESSES cur = pAddrs; cur; cur = cur->Next) {
        if (cur->OperStatus == IfOperStatusUp && cur->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
            activeAdapter = cur;
            break;
        }
    }

    if (!activeAdapter) {
        return "<strong>Connection Status:</strong> DISCONNECTED (No Active Internet Connection)";
    }

    std::string hw_name = wstrToStr(activeAdapter->Description);
    std::string friendly_name = wstrToStr(activeAdapter->FriendlyName);

    // 1. IPv4 Address
    std::string ip_addr = "N/A";
    for (PIP_ADAPTER_UNICAST_ADDRESS ua = activeAdapter->FirstUnicastAddress; ua; ua = ua->Next) {
        if (ua->Address.lpSockaddr->sa_family == AF_INET) {
            char ipStr[64] = {};
            inet_ntop(AF_INET, &((sockaddr_in*)ua->Address.lpSockaddr)->sin_addr, ipStr, sizeof(ipStr));
            ip_addr = ipStr;
            break;
        }
    }

    // 2. MAC Address
    std::string mac_addr = "N/A";
    if (activeAdapter->PhysicalAddressLength > 0) {
        char mac[32] = {};
        for (UINT b = 0; b < activeAdapter->PhysicalAddressLength && b < 6; b++) {
            char part[4];
            sprintf_s(part, sizeof(part), "%02X%s", activeAdapter->PhysicalAddress[b], (b < 5 ? ":" : ""));
            strcat_s(mac, sizeof(mac), part);
        }
        mac_addr = mac;
    }

    std::stringstream net_details;

    // 3. Ekstraksi Detail Wi-Fi via Native WlanApi (Jika tipe adaptor == Wireless)
    if (activeAdapter->IfType == IF_TYPE_IEEE80211) {
        std::string ssid = "N/A";
        std::string signal_quality = "N/A";
        std::string radio_type = "802.11";
        std::string auth_algo = "";

        HANDLE hClient = NULL;
        DWORD dwCurVersion = 0;
        if (WlanOpenHandle(2, NULL, &dwCurVersion, &hClient) == ERROR_SUCCESS) {
            PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
            if (WlanEnumInterfaces(hClient, NULL, &pIfList) == ERROR_SUCCESS) {
                for (DWORD i = 0; i < pIfList->dwNumberOfItems; i++) {
                    PWLAN_INTERFACE_INFO pIfInfo = (PWLAN_INTERFACE_INFO)&pIfList->InterfaceInfo[i];
                    if (pIfInfo->isState == wlan_interface_state_connected) {
                        DWORD dataSize = 0;
                        PWLAN_CONNECTION_ATTRIBUTES pConnectAttr = NULL;
                        if (WlanQueryInterface(hClient, &pIfInfo->InterfaceGuid, wlan_intf_opcode_current_connection, NULL, &dataSize, (PVOID*)&pConnectAttr, NULL) == ERROR_SUCCESS) {
                            
                            // SSID
                            DOT11_SSID dot11Ssid = pConnectAttr->wlanAssociationAttributes.dot11Ssid;
                            if (dot11Ssid.uSSIDLength > 0) {
                                ssid = std::string((char*)dot11Ssid.ucSSID, dot11Ssid.uSSIDLength);
                            }

                            // Signal Quality (0 - 100%)
                            signal_quality = std::to_string(pConnectAttr->wlanAssociationAttributes.wlanSignalQuality) + "%";

							// PHY / Radio Type (802.11a/b/g/n/ac/ax/be) - Menggunakan IF-ELSE untuk mencegah bentrokan Enum MinGW
							DOT11_PHY_TYPE phyType = pConnectAttr->wlanAssociationAttributes.dot11PhyType;
							if (phyType == dot11_phy_type_ofdm) {
								radio_type = "802.11a";
							} else if (phyType == dot11_phy_type_hrdsss) {
								radio_type = "802.11b";
							} else if (phyType == dot11_phy_type_erp) {
								radio_type = "802.11g";
							} else if (phyType == dot11_phy_type_ht) {
								radio_type = "802.11n (Wi-Fi 4)";
							} else if (phyType == dot11_phy_type_vht) {
								radio_type = "802.11ac (Wi-Fi 5)";
							} else if ((int)phyType == 7 || (int)phyType == 9) { // 7/9 = High Efficiency (HE) Wi-Fi 6/6E
								radio_type = "802.11ax (Wi-Fi 6)";
							} else if ((int)phyType == 10) { // Extremely High Throughput (EHT) Wi-Fi 7
								radio_type = "802.11be (Wi-Fi 7)";
							} else {
								radio_type = "802.11 Wireless";
							}

                            WlanFreeMemory(pConnectAttr);
                        }
                        break;
                    }
                }
                WlanFreeMemory(pIfList);
            }
            WlanCloseHandle(hClient, NULL);
        }

        net_details << "<strong>Connection Status:</strong> CONNECTED (Wi-Fi Wireless Network)<br>";
        net_details << "<strong>Wi-Fi SSID:</strong> <span style='color:#38bdf8; font-weight:800;'>" << escapeJson(ssid) << "</span><br>";
        net_details << "<strong>Signal Quality:</strong> " << escapeJson(signal_quality) << " | <strong>Protocol:</strong> " << escapeJson(radio_type) << "<br>";
        net_details << "<strong>Link Speed:</strong> " << (activeAdapter->ReceiveLinkSpeed / 1000000ULL) << " Mbps (RX) / " 
                    << (activeAdapter->TransmitLinkSpeed / 1000000ULL) << " Mbps (TX)<br>";
    } else {
        // Adaptor Ethernet / Kabel
        net_details << "<strong>Connection Status:</strong> CONNECTED (Wired Ethernet / LAN)<br>";
        net_details << "<strong>Interface Name:</strong> " << escapeJson(friendly_name) << "<br>";
        net_details << "<strong>Link Speed:</strong> " << (activeAdapter->ReceiveLinkSpeed / 1000000ULL) << " Mbps<br>";
    }

    net_details << "<strong>IP Address:</strong> " << escapeJson(ip_addr) << " | <strong>MAC:</strong> " << escapeJson(mac_addr) << "<br>";
    net_details << "<strong>Hardware:</strong> " << escapeJson(hw_name);

    return net_details.str();
}

static void initHardwareInfo() {
    Log("initHardwareInfo() BEGIN");
    // 1. CPU Model via CPUID
    int cpuInfo[4] = { 0 };
    char brand[49] = {};
    __cpuid(cpuInfo, 0x80000002); memcpy(brand, cpuInfo, 16);
    __cpuid(cpuInfo, 0x80000003); memcpy(brand + 16, cpuInfo, 16);
    __cpuid(cpuInfo, 0x80000004); memcpy(brand + 32, cpuInfo, 16);
    g_cpu_model = trim(brand);
    if (g_cpu_model.empty()) g_cpu_model = "x86_64 Processor";

    // 2. GPU via DXGI
    enumerateGpuAdapters();

    // 3. Cache Strings
    g_cache_raw_cpu = buildCpuDetailText();
    g_cache_raw_gpu = buildGpuDetailText();
    g_cache_raw_ram = buildMemoryDetailText();
    g_cache_raw_net = buildNetDetailText();
    g_cache_network_type = buildDynamicNetworkTypeString();
    MEMORYSTATUSEX ms = { sizeof(ms) };
	GlobalMemoryStatusEx(&ms);
	double total_gb = (double)ms.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);

	char ram_buf[128];
	snprintf(ram_buf, sizeof(ram_buf), "[\"%.0f GB System Physical Memory\"]", total_gb);
	g_cache_ram_type_json = ram_buf;
    g_cache_battery_tech = getBatteryStatus();

    // 4. Pre-cache Disk Details
    Log("initHardwareInfo(): mulai discoverDisks()...");
    auto disks = discoverDisks();
    Log("initHardwareInfo(): discoverDisks() selesai, ditemukan " + std::to_string(disks.size()) + " disk.");
    for (auto& disk : disks) {
        Log("initHardwareInfo(): mulai buildDiskDetailText(" + disk.id + " / " + disk.dev + ")...");
        g_disk_details_map[disk.id] = buildDiskDetailText(disk.id);
        g_disk_details_map[disk.dev] = g_disk_details_map[disk.id];
        Log("initHardwareInfo(): selesai buildDiskDetailText(" + disk.id + ")");
    }
    Log("initHardwareInfo() END");
}

// =============================================================================
// Helper Formatter Dinamis (Anti-Unit Jika 0)
// =============================================================================
// zeroAsNA = true  -> Nilai <= 0 dianggap "metric ini gak tersedia" -> "N/A"
// zeroAsNA = false -> Nilai <= 0 dianggap "metric valid, nilainya nol" -> "0 B"
// Nilai > 0        -> Menyesuaikan otomatis (B, KB, MB, GB, TB), sama di kedua mode
static std::string formatBytesDynamic(double bytes, bool zeroAsNA = true) {
    if (bytes <= 0.0) return zeroAsNA ? "N/A" : "0 B";

    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int unitIndex = 0;
    double size = bytes;

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    char buf[64];
    if (fmod(size, 1.0) == 0.0) {
        snprintf(buf, sizeof(buf), "%.0f %s", size, units[unitIndex]);
    } else {
        snprintf(buf, sizeof(buf), "%.2f %s", size, units[unitIndex]);
    }
    return std::string(buf);
}

// Helper untuk format pasangan (Used / Total):
// Jika Total <= 0 -> "N/A" (metric memang gak tersedia sama sekali, mis. GPU tanpa dedicated VRAM)
// Jika Total > 0 tapi Used == 0 -> "0 B / Total" (metric valid, sekadar gak lagi dipakai)
static std::string formatMemoryPair(double usedBytes, double totalBytes) {
    if (totalBytes <= 0.0) return "N/A";
    return formatBytesDynamic(usedBytes, false) + " / " + formatBytesDynamic(totalBytes, true);
}

// Helper untuk format Speed (Bytes/sec):
// Jika Rate <= 0 -> "0"
static std::string formatSpeedDynamic(double bytes_per_sec) {
    if (bytes_per_sec <= 0.0) return "0";
    return formatSpeed(bytes_per_sec);
}

// =============================================================================
// Main Telemetry Update Loop (Fixed Dedicated/Shared VRAM & Dynamic Format)
// =============================================================================
static void updateTelemetry() {
    Log("updateTelemetry() BEGIN");
    double now = getTimeSec();

    // 1. CPU Usage & Frequencies
    std::map<std::string, double> cpu_usages;
    getCpuUsage(cpu_usages);
    std::vector<int> cpu_freqs = getCpuFrequencies();

    // 2. Memory
    MemoryInfo memInfo = getMemoryInfo();

    // 3. GPU Telemetry
    GpuInfo gpuInfo = getGpuInfo();
    g_gpu_model = gpuInfo.model;
    if (!g_gpu_adapters.empty()) {
        g_gpu_adapters[0].usage_pct = gpuInfo.usage_pct;
    }

    // 4. Network & Disk I/O Rates
    updateNetworkTelemetry();
    updateDiskIo();

    // 5. Discover Disks
    auto disks = discoverDisks();

    // 6. Sensors
    int temp_c = getCpuTemperature(cpu_usages.count("cpu") ? cpu_usages["cpu"] : 0.0);
    int battery_pct = getBatteryPercent();
    g_cache_battery_tech = getBatteryStatus(); // <-- UPDATE TELEMETRI BATERAI REALTIME SETIAP DETIK

    // 7. Processes
    std::vector<ProcEntry> cpu_procs, mem_procs;
    std::vector<DiskProcEntry> disk_procs;
    std::vector<NetProcEntry> net_procs;
    std::vector<GpuProcEntry> gpu_procs;
    double gpu_total_rcs = 0, gpu_total_bcs = 0, gpu_total_vcs = 0, gpu_total_vecs = 0;

    getTopCpuProcesses(cpu_procs);
    getTopMemProcesses(mem_procs);
    std::map<std::string, std::vector<DiskProcEntry>> per_disk_procs;
    getTopDiskProcesses(disk_procs, per_disk_procs, disks);
    getTopNetProcesses(net_procs);
    getTopGpuProcesses(gpu_procs, gpu_total_rcs, gpu_total_bcs, gpu_total_vcs, gpu_total_vecs);

    unsigned long long smart_cache_bytes = getCpuSmartCacheTotalBytes();

    // ✅ Akumulasi VRAM dari seluruh adapter GPU
    double total_dedicated_vram_bytes = 0.0;
    double total_shared_vram_bytes = 0.0;

    for (const auto& adapter : g_gpu_adapters) {
        total_dedicated_vram_bytes += adapter.dedicated_vram_gb * 1024.0 * 1024.0 * 1024.0;
        total_shared_vram_bytes    += adapter.shared_vram_gb * 1024.0 * 1024.0 * 1024.0;
    }

    double primary_dedicated_vram_bytes = total_dedicated_vram_bytes;
    double primary_shared_vram_bytes    = total_shared_vram_bytes;

    // ✅ Query penggunaan VRAM realtime DXGI
    double total_dedicated_used = 0.0;
    double total_shared_used    = 0.0;

    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4)))) {
        UINT i = 0;
        ComPtr<IDXGIAdapter1> adapter1;
        
        while (factory4->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND) {
            DXGI_ADAPTER_DESC1 desc;
            adapter1->GetDesc1(&desc);
            
            if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
                ComPtr<IDXGIAdapter3> adapter3;
                if (SUCCEEDED(adapter1.As(&adapter3))) {
                    DXGI_QUERY_VIDEO_MEMORY_INFO memInfoLocal = {};
                    if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfoLocal))) {
                        total_dedicated_used += (double)memInfoLocal.CurrentUsage;
                    }

                    DXGI_QUERY_VIDEO_MEMORY_INFO memInfoNonLocal = {};
                    if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &memInfoNonLocal))) {
                        total_shared_used += (double)memInfoNonLocal.CurrentUsage;
                    }
                }
            }
            adapter1.Reset();
            i++;
        }
    }

    double primary_dedicated_vram_used = total_dedicated_used;
    double primary_shared_vram_used    = total_shared_used;

    // ✅ SEMUA STRING MEMORI & IO DIBUAT DINAMIS (Nilai 0 -> "0")
    std::string dedicated_vram_str = formatMemoryPair(primary_dedicated_vram_used, primary_dedicated_vram_bytes);
    std::string shared_vram_str    = formatBytesDynamic(primary_shared_vram_bytes);
    std::string physical_ram_str   = formatMemoryPair((double)memInfo.used, (double)memInfo.total);
    std::string pagefile_str       = formatMemoryPair((double)memInfo.pagefile_used, (double)memInfo.pagefile_total);
    std::string cache_str          = formatBytesDynamic((double)smart_cache_bytes);
    
    std::string net_rx_str         = formatSpeedDynamic(g_cur_rx_rate);
    std::string net_tx_str         = formatSpeedDynamic(g_cur_tx_rate);
    std::string disk_read_str      = formatSpeedDynamic(g_cur_disk_read_rate);
    std::string disk_write_str     = formatSpeedDynamic(g_cur_disk_write_rate);

    // Build JSON Response
    std::stringstream json;
    json << "{";
    json << "\"os_type\":\"windows\",";

    // Hardware Information
    json << "\"hardware\":{\"cpu_model\":\"" << escapeJson(g_cpu_model) 
         << "\",\"gpu_model\":\"" << escapeJson(g_gpu_model) << "\"},";

    // Telemetri Memori Lengkap
    json << "\"ram\":{\"total\":" << memInfo.total << ",\"used\":" << memInfo.used << ",\"free\":" << memInfo.avail 
         << ",\"display_str\":\"" << escapeJson(physical_ram_str) << "\"},";
    json << "\"vram\":{"
         << "\"dedicated_total\":" << primary_dedicated_vram_bytes << ","
         << "\"dedicated_used\":" << primary_dedicated_vram_used << ","
         << "\"shared_total\":" << primary_shared_vram_bytes << ","
         << "\"shared_used\":" << primary_shared_vram_used << ","
         << "\"dedicated_str\":\"" << escapeJson(dedicated_vram_str) << "\","
         << "\"shared_str\":\"" << escapeJson(shared_vram_str) << "\""
         << "},";
    json << "\"smart_cache\":{\"total\":" << smart_cache_bytes << ",\"display_str\":\"" << escapeJson(cache_str) << "\"},";
    json << "\"cpu_cache\":{\"smart_cache_total\":" << smart_cache_bytes << ",\"total\":" << smart_cache_bytes << "},";
    json << "\"zram\":{\"total\":0,\"used\":0,\"display_str\":\"0\"},";
    json << "\"swap\":{\"total\":" << memInfo.pagefile_total << ",\"used\":" << memInfo.pagefile_used << ",\"display_str\":\"" << escapeJson(pagefile_str) << "\"},";

    // CPU Cores & Real Frequencies
    std::vector<std::string> cpu_core_types = detectCpuCoreTypes();

    auto cpu_top = getDynamicCpuTopology();
    auto gpu_eng = getDynamicGpuEngines();

    // CPU Cores, Tags (P/E/LP/C), Core Types, & Real Frequencies
    json << "\"cpu\":{\"total_usage\":" << (cpu_usages.count("cpu") ? cpu_usages["cpu"] : 0.0) << ",\"cores\":[";
    for (size_t i = 0; i < cpu_top.size(); i++) {
        if (i > 0) json << ",";
        std::string cname = "cpu" + std::to_string(i);
        json << (cpu_usages.count(cname) ? cpu_usages[cname] : 0.0);
    }
    json << "],\"core_tags\":[";
    for (size_t i = 0; i < cpu_top.size(); i++) {
        if (i > 0) json << ",";
        json << "\"" << cpu_top[i].tag << i << "\"";
    }
    json << "],\"core_types\":[";
    for (size_t i = 0; i < cpu_top.size(); i++) {
        if (i > 0) json << ",";
        json << "\"" << cpu_top[i].type << "\"";
    }
    json << "],\"freqs\":[";
    for (size_t i = 0; i < cpu_top.size(); i++) {
        if (i > 0) json << ",";
        json << (i < cpu_freqs.size() ? cpu_freqs[i] : 0);
    }
    json << "]},";
	
    // GPU Primary & Adapters Array
    json << "\"gpu\":{\"freq\":" << gpuInfo.freq_mhz << ",\"usage\":{\"rcs\":" << gpu_total_rcs << ",\"bcs\":" << gpu_total_bcs << ",\"vcs\":" << gpu_total_vcs << ",\"vecs\":" << gpu_total_vecs << "}},";
    json << "\"gpus\":[";
    for (size_t i = 0; i < g_gpu_adapters.size(); i++) {
        if (i > 0) json << ",";
        auto& ga = g_gpu_adapters[i];
        json << "{\"id\":\"" << escapeJson(ga.id) << "\","
             << "\"name\":\"" << escapeJson(ga.name) << "\","
             << "\"is_egpu\":" << (ga.is_egpu ? "true" : "false") << ","
             << "\"dedicated_vram_gb\":" << ga.dedicated_vram_gb << ","
             << "\"shared_vram_gb\":" << ga.shared_vram_gb << ","
             << "\"usage_pct\":" << ga.usage_pct << ","
             << "\"cores\":[";
        
        // Pemisahan: Hanya eGPU/dGPU yang mengekstrak multi-cluster core
        if (ga.is_egpu && !gpu_eng.empty()) {
            for (size_t e = 0; e < gpu_eng.size() && e < 6; e++) {
                if (e > 0) json << ",";
                json << "{\"name\":\"" << escapeJson(gpu_eng[e].name) << "\","
                     << "\"val\":\"" << (int)gpu_eng[e].usage << "%\","
                     << "\"sub\":\"Hardware Engine\"}";
            }
        } else {
            // iGPU: 100% Unified Architecture
            json << "{\"name\":\"iGPU\",\"val\":\"Unified\",\"sub\":\"Execution Units\"}";
        }
        json << "]}";
    }
    json << "],";

    // Network Telemetry
    json << "\"network\":{\"rx_rate\":" << g_cur_rx_rate << ",\"tx_rate\":" << g_cur_tx_rate 
         << ",\"rx_str\":\"" << escapeJson(net_rx_str) << "\",\"tx_str\":\"" << escapeJson(net_tx_str) << "\"},";

    // Physical Disks Telemetry
    json << "\"disk\":{\"read_rate\":" << g_cur_disk_read_rate << ",\"write_rate\":" << g_cur_disk_write_rate 
         << ",\"read_str\":\"" << escapeJson(disk_read_str) << "\",\"write_str\":\"" << escapeJson(disk_write_str) << "\",\"disks\":[";
    bool first_disk = true;
    std::string ssd_models_joined;
    for (auto& disk : disks) {
        if (!first_disk) json << ",";
        first_disk = false;

        char usage_buf[128];
        snprintf(usage_buf, sizeof(usage_buf), "%.1f GB Used (%.0f%%) | %.1f GB Free", disk.used_gb, disk.usage_pct, disk.free_gb);

		// BARU: Mendukung format persentase Maupun format "TB Written"
		std::string remaining_life = "N/A";
		{
			size_t pct = disk.tbw_str.find('%');
			if (pct != std::string::npos) {
				try {
					int wear = std::stoi(disk.tbw_str.substr(0, pct));
					remaining_life = std::to_string(100 - wear) + "% Remaining";
				} catch (...) {}
			} else if (disk.tbw_str.find("TB Written") != std::string::npos) {
				remaining_life = disk.tbw_str; // Tampilkan nilai TB Written secara langsung ke UI
			}
		}
		
		std::string full_disk_detail = buildDiskDetailText(disk.id);

        json << "{\"id\":\"" << escapeJson(disk.id) << "\","
             << "\"dev\":\"" << escapeJson(disk.dev) << "\","
             << "\"name\":\"" << escapeJson(disk.model + " (" + disk.size_str + ")") << "\","
             << "\"model\":\"" << escapeJson(disk.model) << "\","
             << "\"size\":\"" << escapeJson(disk.size_str) << "\","
             << "\"capacity_str\":\"" << escapeJson(disk.size_str) << " Total Capacity\","
             << "\"usage_str\":\"" << escapeJson(usage_buf) << "\","
             << "\"partitions_str\":\"" << escapeJson(disk.partitions_str) << "\","
             << "\"icon\":\"" << disk.icon << "\","
             << "\"label\":\"" << disk.icon << " " << escapeJson(disk.model + " (" + disk.size_str + ")") << "\","
             << "\"type\":\"" << escapeJson(disk.type_str) << "\","
             << "\"transport\":\"" << (disk.is_nvme ? "nvme" : (disk.is_ssd ? "sata" : "usb")) << "\","
             << "\"is_root\":" << (disk.is_system ? "true" : "false") << ","
             << "\"is_rizky\":false,"
             << "\"is_ssd\":" << (disk.is_ssd ? "true" : "false") << ","
             << "\"is_hdd\":" << ((!disk.is_ssd && !disk.is_removable) ? "true" : "false") << ","
             << "\"is_flash\":" << (disk.is_removable ? "true" : "false") << ","
             << "\"read_rate\":" << disk.read_rate << ","
             << "\"write_rate\":" << disk.write_rate << ","
             << "\"tbw_str\":\"" << escapeJson(disk.tbw_str) << "\","
             << "\"remaining_str\":\"" << escapeJson(remaining_life) << "\","
             << "\"temp_str\":\"" << escapeJson(disk.temp_str) << "\","
             << "\"health_str\":\"" << escapeJson(disk.health_str) << "\","
             << "\"detail_text\":\"" << escapeJson(full_disk_detail) << "\""
             << "}";

        if (!ssd_models_joined.empty()) ssd_models_joined += " | ";
        ssd_models_joined += disk.dev + ": " + disk.model + " (" + disk.size_str + ")";
    }
    json << "],\"list\":[";
    first_disk = true;
    for (auto& disk : disks) {
        if (!first_disk) json << ",";
        first_disk = false;
        json << "{\"id\":\"" << escapeJson(disk.id) << "\",\"label\":\"" << disk.icon << " " << escapeJson(disk.model) << " (" << escapeJson(disk.size_str) << ")\"}";
    }
    json << "]},";

    // Sensors
    json << "\"sensors\":{\"temp\":" << temp_c << ",\"battery\":" << battery_pct << "},";

    // Top Processes
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
        json << "{\"name\":\"" << escapeJson(disk_procs[i].name) << "\",\"read\":\"" << escapeJson(disk_procs[i].read) << "\",\"write\":\"" << escapeJson(disk_procs[i].write) << "\",\"val\":\"" << escapeJson(disk_procs[i].val) << "\"}";
    }
    json << "],\"disk_per_dev\":{";
    if (!disks.empty()) {
        bool first_dev = true;
        for (auto& disk : disks) {
            if (!first_dev) json << ",";
            first_dev = false;
            
            json << "\"" << escapeJson(disk.id) << "\":[";
            auto& d_procs = per_disk_procs[disk.id];
            for (size_t i = 0; i < d_procs.size(); i++) {
                if (i > 0) json << ",";
                json << "{\"name\":\"" << escapeJson(d_procs[i].name) << "\","
                     << "\"read\":\"" << escapeJson(d_procs[i].read) << "\","
                     << "\"write\":\"" << escapeJson(d_procs[i].write) << "\","
                     << "\"val\":\"" << escapeJson(d_procs[i].val) << "\"}";
            }
            json << "]";
        }
    }
    json << "},\"net\":[";
    for (size_t i = 0; i < net_procs.size(); i++) {
        if (i > 0) json << ",";
        json << "{\"name\":\"" << escapeJson(net_procs[i].name) << "\",\"down\":\"" << escapeJson(net_procs[i].down) << "\",\"up\":\"" << escapeJson(net_procs[i].up) << "\",\"val\":\"" << escapeJson(net_procs[i].val) << "\"}";
    }
    json << "],\"gpu\":[";
    for (size_t i = 0; i < gpu_procs.size(); i++) {
        if (i > 0) json << ",";
        json << "{\"name\":\"" << escapeJson(gpu_procs[i].name) << "\",\"rcs\":" << gpu_procs[i].rcs
             << ",\"bcs\":" << gpu_procs[i].bcs << ",\"vcs\":" << gpu_procs[i].vcs
             << ",\"vecs\":" << gpu_procs[i].vecs << ",\"total\":" << gpu_procs[i].total << "}";
    }
    json << "]},";

    // Detailed Mode Strings Cache
    json << "\"details\":{";
    json << "\"raw_cpu\":\"" << escapeJson(g_cache_raw_cpu) << "\",";
    json << "\"raw_gpu\":\"" << escapeJson(g_cache_raw_gpu) << "\",";
    json << "\"raw_ram\":\"" << escapeJson(g_cache_raw_ram) << "\",";
    json << "\"raw_net\":\"" << escapeJson(g_cache_raw_net) << "\",";
    json << "\"raw_disk\":\"" << escapeJson(g_cache_raw_disk) << "\",";
    json << "\"ram_type\":" << g_cache_ram_type_json << ",";
    json << "\"zram_info\":\"Windows Memory Compression & Paging Manager\",";
    json << "\"swap_info\":\"System Managed Pagefile (Virtual Memory Active)\",";
    json << "\"ssd_model\":\"" << escapeJson(ssd_models_joined) << "\",";
    json << "\"network_type\":\"" << escapeJson(g_cache_network_type) << "\",";
    json << "\"battery_tech\":\"" << escapeJson(g_cache_battery_tech) << "\",";

    unsigned long long uptime_sec = GetTickCount64() / 1000;
    // Deteksi Nama Resmi Windows Secara Dinamis via Windows Branding API (winbrand.dll)
    // Fungsi ini yang dipakai oleh winver.exe dan Windows Settings agar tidak tertipu string legacy registry
    static std::string s_realProductName = "";
    if (s_realProductName.empty()) {
        HMODULE hBrand = LoadLibraryW(L"winbrand.dll");
        if (hBrand) {
            typedef wchar_t* (WINAPI *BrandingFormatStringFn)(const wchar_t*);
            auto pfn = (BrandingFormatStringFn)GetProcAddress(hBrand, "BrandingFormatString");
            if (pfn) {
                wchar_t* wstr = pfn(L"%WINDOWS_LONG%");
                if (wstr) {
                    char buf[256] = {0};
                    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buf, sizeof(buf), nullptr, nullptr);
                    GlobalFree(wstr);
                    s_realProductName = std::string(buf);
                }
            }
            FreeLibrary(hBrand);
        }
    }

    auto readRegSz = [](const char* valueName) -> std::string {
        char buf[256] = {0};
        DWORD size = sizeof(buf);
        if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
            valueName, RRF_RT_REG_SZ, nullptr, buf, &size) == ERROR_SUCCESS) {
            return std::string(buf);
        }
        return "";
    };

    // Gunakan nama asli dari winbrand; jika tidak ada, baru fallback ke registry
    std::string productName = !s_realProductName.empty() ? s_realProductName : readRegSz("ProductName");
    std::string buildNumber = readRegSz("CurrentBuildNumber");
    std::string displayVer  = readRegSz("DisplayVersion");

    // Deteksi Arsitektur Sistem Dinamis via Kernel Native Hardware Info
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);
    std::string arch = (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) ? "ARM64" :
                       ((sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? "x64" : "x86");

    std::string os_full = (productName.empty() ? "Windows" : productName) + " " + arch;
    std::string kernel_full = "NT Kernel " + (buildNumber.empty() ? "Unknown" : buildNumber) +
    (displayVer.empty() ? "" : (" (" + displayVer + ")"));

    json << "\"uptime\":" << uptime_sec
    << ",\"os\":\"" << escapeJson(os_full)
    << "\",\"kernel\":\"" << escapeJson(kernel_full) << "\"";
    json << "},";

    // Format Platform Target dibuat simetris & dinamis persis seperti Linux: "Windows (x64 Architecture)"
    std::string plat_target = "Windows (" + arch + " Architecture)";

    json << "\"credits\":{";
    json << "\"valid\":true,";
    json << "\"app_name\":\"RizkybyMONITOR\",";
    json << "\"app_version\":\"v1.1\",";
    json << "\"developer\":\"Rizky Bayu\",";
    json << "\"github\":\"https://github.com/rizkybayuu\",";
    json << "\"pills\":[\"C++\",\"HTML\",\"CSS\",\"JavaScript\",\"Win32 API\",\"WebView2\"],";
    json << "\"sections\":[";
    json << "  {\"heading\":\"System Specifications\",\"specs\":[";
    json << "    {\"label\":\"Core Runtime\",\"val\":\"Native ISO C++17\"},";
    json << "    {\"label\":\"Interface Host\",\"val\":\"Microsoft WebView2 / Chromium Core\"},";
    json << "    {\"label\":\"Telemetry Pipeline\",\"val\":\"Win32 PDH / ETW Kernel Trace\"},";
    json << "    {\"label\":\"Storage Diagnostics\",\"val\":\"ATA Passthrough / SCSI-SAT SMART\"},";
    json << "    {\"label\":\"Graphics Provider\",\"val\":\"DXGI 1.4 Dynamic Composition\"}";
    json << "  ]},";
    json << "  {\"heading\":\"Architecture Details\",\"specs\":[";
    json << "    {\"label\":\"Telemetry Latency\",\"val\":\"500ms Non-Blocking Polling (Sub-millisecond Compute)\"},";
    json << "    {\"label\":\"Subprocess Overhead\",\"val\":\"Zero WMIC / Zero CMD spawns\"},";
    json << "    {\"label\":\"Memory Topology\",\"val\":\"Physical RAM, Smart Cache, Dedicated & Shared VRAM\"},";
    json << "    {\"label\":\"Display Mode\",\"val\":\"Frameless DWM Composition\"}";
    json << "  ]},";
    json << "  {\"heading\":\"Execution Environment\",\"specs\":[";
    json << "    {\"label\":\"Platform Target\",\"val\":\"" << escapeJson(plat_target) << "\"},";
    json << "    {\"label\":\"Security & Integrity\",\"val\":\"Standard User Privilege (No Elevation Required)\"},";
    json << "    {\"label\":\"Binary Footprint\",\"val\":\"Single Executable / Portable Deployment\"}";
    json << "  ]}";
    json << "]}";

    // PENUTUP ROOT JSON
    json << "}";

    {
        std::lock_guard<std::mutex> lock(g_stats_mutex);
        g_cached_stats_json = json.str();
    }
    Log("updateTelemetry() END, json size=" + std::to_string(json.str().size()) + " bytes");
}

static void telemetryLoop() {
    while (g_running) {
        updateTelemetry();
        Sleep(500); // 500ms sampling interval
    }
}

// =============================================================================
// Native HTTP Server (Winsock2) & Thread-Safe Win32 Message Dispatcher
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
    Log("REQUEST: " + method + " " + path);

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
    else if (method == "GET" && path.rfind("/api/stats", 0) == 0) {
        int win_id = 1;
        size_t qp = path.find("win=");
        if (qp != std::string::npos) win_id = std::atoi(path.c_str() + qp + 4);

        std::string selected_disk;
        {
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            if (g_windows.find(win_id) != g_windows.end()) selected_disk = g_windows[win_id].selected_disk;
        }

        std::lock_guard<std::mutex> lock(g_stats_mutex);
        response_body = g_cached_stats_json;

        auto rateIt = g_disk_rates_by_id.find(selected_disk);
        if (rateIt != g_disk_rates_by_id.end()) {
            std::string readKey = "\"disk\":{\"read_rate\":";
            size_t p1 = response_body.find(readKey);
            if (p1 != std::string::npos) {
                size_t numStart = p1 + readKey.size();
                size_t numEnd = response_body.find(',', numStart);
                std::string writeKey = "\"write_rate\":";
                size_t wrPos = response_body.find(writeKey, numEnd);
                if (numEnd != std::string::npos && wrPos != std::string::npos) {
                    size_t wrNumStart = wrPos + writeKey.size();
                    size_t wrNumEnd = response_body.find(',', wrNumStart);
                    if (wrNumEnd != std::string::npos) {
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%.2f", rateIt->second.second);
                        response_body.replace(wrNumStart, wrNumEnd - wrNumStart, buf);
                        snprintf(buf, sizeof(buf), "%.2f", rateIt->second.first);
                        response_body.replace(numStart, numEnd - numStart, buf);
                    }
                }
            }
        }
    }
    else if (method == "GET" && path.rfind("/api/config", 0) == 0) {
        response_body = readFile(g_data_dir + "\\config.json");
        if (response_body.empty()) response_body = "{}";
    }
    else if (method == "POST" && path == "/api/config") {
        size_t body_start = request.find("\r\n\r\n");
        std::string body = (body_start != std::string::npos) ? request.substr(body_start + 4) : "";

        int win_id = 1;
        size_t wid_pos = body.find("\"window_id\"");
        if (wid_pos != std::string::npos) {
            size_t val_pos = body.find_first_of("0123456789", wid_pos + 11);
            if (val_pos != std::string::npos) {
                win_id = std::atoi(body.c_str() + val_pos);
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            if (g_windows.find(win_id) != g_windows.end()) {
                auto& w = g_windows[win_id];

                size_t title_pos = body.find("\"app_title\"");
                if (title_pos != std::string::npos) {
                    size_t q1 = body.find('"', title_pos + 11);
                    size_t q2 = body.find('"', q1 + 1);
                    if (q1 != std::string::npos && q2 != std::string::npos) {
                        w.app_title = body.substr(q1 + 1, q2 - q1 - 1);
                    }
                }

                size_t det_pos = body.find("\"details\"");
                if (det_pos != std::string::npos) {
                    size_t arr_start = body.find('[', det_pos);
                    size_t arr_end = body.find(']', arr_start);
                    if (arr_start != std::string::npos && arr_end != std::string::npos) {
                        w.details = body.substr(arr_start, arr_end - arr_start + 1);
                    }
                }

                size_t sd_pos = body.find("\"selected_disk\"");
                if (sd_pos != std::string::npos) {
                    size_t q1 = body.find('"', sd_pos + 15);
                    size_t q2 = body.find('"', q1 + 1);
                    if (q1 != std::string::npos && q2 != std::string::npos) {
                        w.selected_disk = body.substr(q1 + 1, q2 - q1 - 1);
                    }
                }

                size_t fs_pos = body.find("\"font_size\"");
                if (fs_pos != std::string::npos) {
                    size_t val_pos = body.find(':', fs_pos);
                    if (val_pos != std::string::npos) {
                        w.font_size = std::atoi(body.c_str() + val_pos + 1);
                    }
                }

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
        response_body = "{\"status\":\"ok\"}";
    }
    else if (method == "POST" && path == "/api/copy") {
        size_t body_start = request.find("\r\n\r\n");
        std::string text = (body_start != std::string::npos) ? request.substr(body_start + 4) : "";

        if (!text.empty()) {
            std::wstring wtext = strToWstr(text);
            if (OpenClipboard(NULL)) {
                EmptyClipboard();
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wtext.size() + 1) * sizeof(wchar_t));
                if (hMem) {
                    memcpy(GlobalLock(hMem), wtext.c_str(), (wtext.size() + 1) * sizeof(wchar_t));
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_UNICODETEXT, hMem);
                }
                CloseClipboard();
            }
        }
        response_body = "{\"status\":\"ok\"}";
    }
	else if (method == "POST" && path == "/api/duplicate") {
        int new_id = 0;
        {
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            // Cari ID terbesar yang ada di map saat ini lalu tambahkan 1
            // Ini mencegah bentrok jika ada window tengah yang ditutup lalu diduplikasi lagi
            int max_id = 0;
            for (auto& [id, w] : g_windows) {
                if (id > max_id) max_id = id;
            }
            new_id = max_id + 1;
            g_next_win_id = new_id;
        }
        // Kirim request buat window baru ke Main UI Thread
        if (g_main_hwnd) {
            PostMessage(g_main_hwnd, WM_APP_CREATE_WINDOW, (WPARAM)new_id, 0);
        }
        response_body = "{\"status\":\"ok\",\"new_id\":" + std::to_string(new_id) + "}";
    }
    else if (method == "POST" && path.rfind("/api/on_top", 0) == 0) {
        int win_id = 1;
        size_t qp = path.find("win=");
        if (qp != std::string::npos) win_id = std::atoi(path.c_str() + qp + 4);

        bool cur_on_top = false;
        {
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            if (g_windows.find(win_id) != g_windows.end()) {
                g_windows[win_id].is_on_top = !g_windows[win_id].is_on_top;
                cur_on_top = g_windows[win_id].is_on_top;
            }
        }
        // Kirim perubahan z-order ke Main UI Thread (Fix Masalah #3 Pin)
        if (g_main_hwnd) {
            PostMessage(g_main_hwnd, WM_APP_TOGGLE_ON_TOP, (WPARAM)win_id, 0);
        }
        response_body = "{\"status\":\"ok\",\"on_top\":" + std::string(cur_on_top ? "true" : "false") + "}";
    }
    else if (method == "POST" && path.rfind("/api/close_window", 0) == 0) {
        int win_id = 1;
        size_t qp = path.find("win=");
        if (qp != std::string::npos) win_id = std::atoi(path.c_str() + qp + 4);

        // Delegasikan penutupan window ke Main UI Thread
        if (g_main_hwnd) {
            PostMessage(g_main_hwnd, WM_APP_CLOSE_WIN, (WPARAM)win_id, 0);
        }
        response_body = "{\"status\":\"ok\"}";
    }
    else if (method == "POST" && path == "/api/quit") {
        g_running = false;
        if (g_main_hwnd) {
            PostMessage(g_main_hwnd, WM_APP_CLOSE_ALL, 0, 0);
        }
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
    resp << "Connection: close\r\n\r\n";
    resp << response_body;

    std::string resp_str = resp.str();
    send(client_fd, resp_str.c_str(), (int)resp_str.size(), 0);
    closesocket(client_fd);
}

static void startHttpServer() {
    Log("startHttpServer() THREAD START, target port=" + std::to_string(g_server_port));
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        Log("FATAL: socket() gagal, WSAGetLastError=" + std::to_string(WSAGetLastError()));
        std::cerr << "Failed to create socket\n";
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // Coba port 8080 sampai 8095 jika ada port yang sedang dipakai
    bool bound = false;
    for (int p = 8080; p <= 8095; ++p) {
        addr.sin_port = htons(p);
        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR) {
            g_server_port = p;
            bound = true;
            break;
        }
    }
    if (!bound) {
        Log("FATAL: Port 8080-8095 sedang terpakai semua, WSAGetLastError=" + std::to_string(WSAGetLastError()));
        closesocket(server_fd);
        WSACleanup();
        MessageBoxW(NULL, L"Failed to bind local port: Ports 8080-8095 are being used by another program.", L"RizkybyMONITOR Error", MB_ICONERROR | MB_OK);
        return;
    }
    Log("bind() OK di 127.0.0.1:" + std::to_string(g_server_port));

    listen(server_fd, 64);
    Log("listen() OK, server siap terima koneksi.");

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
// Win32 Window Management + WebView2 (Pure Frameless & Multi-Window Dispatch)
// =============================================================================
static void saveWindowConfig() {
    std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
    if (g_windows.empty()) return;

    std::stringstream cfg;
    cfg << "{\n";
    
    // Simpan daftar ID window yang benar-benar aktif
    cfg << "  \"active_windows\": [";
    bool first_id = true;
    for (auto& [id, w] : g_windows) {
        if (!first_id) cfg << ",";
        cfg << id;
        first_id = false;
    }
    cfg << "],\n";
    
    cfg << "  \"window_count\": " << g_windows.size() << ",\n";
    cfg << "  \"per_window_settings\": {\n";
    
    bool first = true;
    for (auto& [id, w] : g_windows) {
        if (!first) cfg << ",\n";
        first = false;

        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(w.hwnd, &wp);
        int width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
        int height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        int x = wp.rcNormalPosition.left;
        int y = wp.rcNormalPosition.top;

        cfg << "    \"" << id << "\": {\"width\":" << width << ",\"height\":" << height
            << ",\"x\":" << x << ",\"y\":" << y
            << ",\"on_top\":" << (w.is_on_top ? "true" : "false")
            << ",\"details\":\"" << escapeJson(w.details) << "\""
            << ",\"mode\":\"" << escapeJson(w.mode) << "\""
            << ",\"font_size\":" << w.font_size
            << ",\"selected_disk\":\"" << escapeJson(w.selected_disk) << "\""
            << ",\"app_title\":\"" << escapeJson(w.app_title.empty() ? "RizkybyMONITOR" : w.app_title) << "\""
            << "}";
    }
    cfg << "\n  }\n}";
    writeFile(g_data_dir + "\\config.json", cfg.str());
}

// after
static void createNewWindow(int win_id);

// after
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND: {
            // Cat dengan BLACK_BRUSH: Di DWM Sheet-of-Glass (Margins -1),
            // piksel RGB(0,0,0) / 0x00000000 otomatis dijadikan DWM tembus pandang (alpha=0).
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
            return 1;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_NCCALCSIZE: {
            // Hilangkan seluruh titlebar dan frame default Windows agar murni borderless
            if (wParam == TRUE) {
                return 0;
            }
            break;
        }
        case WM_SIZE: {
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
            // Deteksi resize border pada tepi window (8 pixel)
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT rc;
            GetClientRect(hwnd, &rc);
            int border = 8;
            if (pt.y < border && pt.x < border) return HTTOPLEFT;
            if (pt.y < border && pt.x > rc.right - border) return HTTOPRIGHT;
            if (pt.y > rc.bottom - border && pt.x < border) return HTBOTTOMLEFT;
            if (pt.y > rc.bottom - border && pt.x > rc.right - border) return HTBOTTOMRIGHT;
            if (pt.y < border) return HTTOP;
            if (pt.y > rc.bottom - border) return HTBOTTOM;
            if (pt.x < border) return HTLEFT;
            if (pt.x > rc.right - border) return HTRIGHT;

            // Kembalikan HTCLIENT agar tombol header di index.html bisa diklik normal.
            // Drag window pada header sudah otomatis ditangani oleh CSS -webkit-app-region: drag.
            return HTCLIENT;
        }
        case WM_APP_CREATE_WINDOW: {
            int new_id = (int)wParam;
            createNewWindow(new_id);
            return 0;
        }
        case WM_APP_TOGGLE_ON_TOP: {
            int win_id = (int)wParam;
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            if (g_windows.find(win_id) != g_windows.end()) {
                auto& w = g_windows[win_id];
                SetWindowPos(w.hwnd, w.is_on_top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
            return 0;
        }
        case WM_APP_CLOSE_WIN: {
            int win_id = (int)wParam;
            HWND target_hwnd = NULL;
            {
                std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
                if (g_windows.find(win_id) != g_windows.end()) {
                    target_hwnd = g_windows[win_id].hwnd;
                }
            }
            if (target_hwnd) {
                DestroyWindow(target_hwnd);
            }
            return 0;
        }
		case WM_APP_CLOSE_ALL: {
            g_is_quitting = true;
            saveWindowConfig(); // simpen SEMUA window sekaligus, SEBELUM ada satupun yang didestroy
            std::vector<HWND> all_hwnds;
            {
                std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
                for (auto& [id, w] : g_windows) {
                    all_hwnds.push_back(w.hwnd);
                }
            }
            // DestroyWindow di luar lock supaya WM_DESTROY masing-masing (yang juga
            // butuh lock buat erase dari g_windows) gak saling deadlock
            for (HWND h : all_hwnds) {
                DestroyWindow(h);
            }
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY: {
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            if (!g_is_quitting) {
                saveWindowConfig(); // WAJIB sebelum erase, biar posisi/ukuran window ini kesimpen dulu
                                     // (skip kalau lagi quit-all, karena udah disimpen sekaligus di atas)
            }
            for (auto it = g_windows.begin(); it != g_windows.end(); ++it) {
                if (it->second.hwnd == hwnd) {
                    g_windows.erase(it);
                    break;
                }
            }
            if (g_windows.empty()) {
                PostQuitMessage(0);
            }
            return 0;
        }
        case WM_EXITSIZEMOVE:
            saveWindowConfig();
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// MinGW's <wrl.h> does not implement Microsoft::WRL::Callback (that helper only
// exists in the MSVC/Windows SDK version of WRL). These two small classes are a
// drop-in replacement scoped to just the two completion-handler interfaces this
// file needs, so the WebView2 init code below can stay GCC/MinGW-compatible.
class EnvCreatedHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {
public:
    explicit EnvCreatedHandler(std::function<HRESULT(HRESULT, ICoreWebView2Environment*)> fn)
        : fn_(std::move(fn)), ref_(1) {}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        (void)riid;
        if (!ppv) return E_POINTER;
        *ppv = static_cast<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&ref_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = InterlockedDecrement(&ref_);
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, ICoreWebView2Environment* env) override {
        return fn_(errorCode, env);
    }
private:
    std::function<HRESULT(HRESULT, ICoreWebView2Environment*)> fn_;
    volatile LONG ref_;
};

class ControllerCreatedHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {
public:
    explicit ControllerCreatedHandler(std::function<HRESULT(HRESULT, ICoreWebView2Controller*)> fn)
        : fn_(std::move(fn)), ref_(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        (void)riid;
        if (!ppv) return E_POINTER;
        *ppv = static_cast<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&ref_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = InterlockedDecrement(&ref_);
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT errorCode, ICoreWebView2Controller* controller) override {
        return fn_(errorCode, controller);
    }
private:
    std::function<HRESULT(HRESULT, ICoreWebView2Controller*)> fn_;
    volatile LONG ref_;
};

class NavigationStartingHandler : public ICoreWebView2NavigationStartingEventHandler {
public:
    explicit NavigationStartingHandler(std::function<HRESULT(ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs*)> fn)
        : fn_(std::move(fn)), ref_(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        (void)riid; // see EnvCreatedHandler::QueryInterface above
        if (!ppv) return E_POINTER;
        *ppv = static_cast<ICoreWebView2NavigationStartingEventHandler*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&ref_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = InterlockedDecrement(&ref_);
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) override {
        return fn_(sender, args);
    }
private:
    std::function<HRESULT(ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs*)> fn_;
    volatile LONG ref_;
};

class NewWindowRequestedHandler : public ICoreWebView2NewWindowRequestedEventHandler {
public:
    explicit NewWindowRequestedHandler(std::function<HRESULT(ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs*)> fn)
        : fn_(std::move(fn)), ref_(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        (void)riid; // see EnvCreatedHandler::QueryInterface above
        if (!ppv) return E_POINTER;
        *ppv = static_cast<ICoreWebView2NewWindowRequestedEventHandler*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&ref_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = InterlockedDecrement(&ref_);
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) override {
        return fn_(sender, args);
    }
// after
private:
    std::function<HRESULT(ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs*)> fn_;
    volatile LONG ref_;
};

// Matiin GPU-compositing khusus utk page (bukan decode/canvas) — ini yg bikin tepi
// anti-aliasing border-radius kompositing salah jadi putih pas background transparan.
// GPU compositing dipindah ke CPU path, transparansi CSS jadi render benar.
class EnvOptionsImpl : public ICoreWebView2EnvironmentOptions {
public:
    EnvOptionsImpl() : ref_(1) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        (void)riid;
		*ppv = static_cast<ICoreWebView2EnvironmentOptions*>(this);
		AddRef();
		return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&ref_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = InterlockedDecrement(&ref_);
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE get_AdditionalBrowserArguments(LPWSTR* value) override {
        // Jangan gunakan --disable-gpu-compositing karena mematikan DirectComposition swapchain
        // yang dibutuhkan WebView2 untuk transparansi desktop.
        *value = nullptr;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE put_AdditionalBrowserArguments(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE get_Language(LPWSTR* value) override { *value = nullptr; return S_OK; }
    HRESULT STDMETHODCALLTYPE put_Language(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE get_TargetCompatibleBrowserVersion(LPWSTR* value) override { *value = nullptr; return S_OK; }
    HRESULT STDMETHODCALLTYPE put_TargetCompatibleBrowserVersion(LPCWSTR) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE get_AllowSingleSignOnUsingOSPrimaryAccount(BOOL* value) override { *value = FALSE; return S_OK; }
    HRESULT STDMETHODCALLTYPE put_AllowSingleSignOnUsingOSPrimaryAccount(BOOL) override { return S_OK; }
private:
    volatile LONG ref_;
};

// Helper: cek apakah URI ini navigasi internal ke server lokal kita sendiri
static bool isInternalLocalUri(LPWSTR uri) {
    if (!uri) return false;
    std::wstring prefix = L"http://127.0.0.1:" + std::to_wstring(g_server_port);
    return std::wstring(uri).rfind(prefix, 0) == 0;
}

// GUID asli ICoreWebView2Controller2 dari WebView2.h untuk kompatibilitas MinGW-w64:
// MIDL_INTERFACE("c979903e-d4ca-4228-92eb-47ee3fa96eab")
static const IID IID_ICoreWebView2Controller2_Local =
    { 0xc979903e, 0xd4ca, 0x4228, { 0x92, 0xeb, 0x47, 0xee, 0x3f, 0xa9, 0x6e, 0xab } };

static void createNewWindow(int win_id) {
    int w = 480, h = 720, x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    bool on_top = false;
    std::string details = "[]", mode = "dark", selected_disk = "disk0";
    std::string app_title = "RizkybyMONITOR";
    int font_size = 14;

    std::string cfg = readFile(g_data_dir + "\\config.json");
    if (!cfg.empty()) {
        size_t pws_pos = cfg.find("\"per_window_settings\"");
        if (pws_pos != std::string::npos) {
            std::string key = "\"" + std::to_string(win_id) + "\":";
            size_t pos = cfg.find(key, pws_pos);
            
            if (pos != std::string::npos) {
                size_t obj_start = cfg.find('{', pos);
                if (obj_start != std::string::npos) {
                    size_t obj_end = cfg.size();
                    int depth = 0;
                    for (size_t i = obj_start; i < cfg.size(); i++) {
                        if (cfg[i] == '{') depth++;
                        else if (cfg[i] == '}') { 
                            depth--; 
                            if (depth == 0) { obj_end = i; break; } 
                        }
                    }

                    auto getInt = [&](const std::string& field, int def) -> int {
                        size_t fp = cfg.find("\"" + field + "\"", obj_start);
                        if (fp == std::string::npos || fp >= obj_end) return def;
                        size_t cp = cfg.find(':', fp);
                        if (cp == std::string::npos || cp >= obj_end) return def;
                        size_t val_start = cfg.find_first_of("-0123456789", cp + 1);
                        if (val_start == std::string::npos || val_start >= obj_end) return def;
                        return std::atoi(cfg.c_str() + val_start);
                    };

                    auto getString = [&](const std::string& field, const std::string& def) -> std::string {
                        size_t fp = cfg.find("\"" + field + "\"", obj_start);
                        if (fp == std::string::npos || fp >= obj_end) return def;
                        size_t q1 = cfg.find('"', fp + field.length() + 2);
                        if (q1 == std::string::npos || q1 >= obj_end) return def;
                        size_t q2 = cfg.find('"', q1 + 1);
                        if (q2 == std::string::npos || q2 >= obj_end) return def;
                        return cfg.substr(q1 + 1, q2 - q1 - 1);
                    };

                    w = getInt("width", 480);
                    h = getInt("height", 720);
                    x = getInt("x", CW_USEDEFAULT);
                    y = getInt("y", CW_USEDEFAULT);
                    font_size = getInt("font_size", 14);

                    mode = getString("mode", "dark");
                    selected_disk = getString("selected_disk", "disk0");
                    app_title = getString("app_title", "RizkybyMONITOR");

                    size_t det_pos = cfg.find("\"details\"", obj_start);
                    if (det_pos != std::string::npos && det_pos < obj_end) {
                        size_t q1 = cfg.find('"', det_pos + 9);
                        size_t q2 = cfg.find('"', q1 + 1);
                        if (q1 != std::string::npos && q2 != std::string::npos && q2 < obj_end) {
                            details = cfg.substr(q1 + 1, q2 - q1 - 1);
                        }
                    }

                    size_t otp = cfg.find("\"on_top\"", obj_start);
                    if (otp != std::string::npos && otp < obj_end) {
                        size_t tp = cfg.find("true", otp);
                        on_top = (tp != std::string::npos && tp < otp + 20 && tp < obj_end);
                    }
                }
            }
        }
    }

    DWORD style = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    DWORD exStyle = WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP;
    if (on_top) exStyle |= WS_EX_TOPMOST;

    HWND hwnd = CreateWindowExW(
        exStyle, WINDOW_CLASS, L"RizkybyMONITOR",
        style, x, y, w, h,
        NULL, NULL, g_hInstance, NULL);

    if (!hwnd) {
        std::cerr << "Failed to create window for win_id " << win_id << std::endl;
        return;
    }

    // Hilangkan border default bawaan OS Windows 11
    COLORREF noBorder = 0xFFFFFFFE; // DWMWA_COLOR_NONE
    DwmSetWindowAttribute(hwnd, 34 /* DWMWA_BORDER_COLOR */, &noBorder, sizeof(noBorder));

    // Hindari pembulatan sudut OS bawaan karena index.html sudah memiliki sudut melengkung sendiri
    int cornerPref = 1; // DWMWCP_DONOTROUND
    DwmSetWindowAttribute(hwnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &cornerPref, sizeof(cornerPref));

    if (g_main_hwnd == NULL) {
        g_main_hwnd = hwnd;
    }

    WindowInstance wi = {};
    wi.id = win_id;
    wi.hwnd = hwnd;
    wi.is_on_top = on_top;
    wi.details = details;
    wi.mode = mode;
    wi.font_size = font_size;
    wi.selected_disk = selected_disk;
    wi.app_title = app_title;

    {
        std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
        g_windows[win_id] = wi;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Initialize WebView2 Runtime
    std::wstring userDataFolder = strToWstr(g_app_dir) + L"\\webview2_data";

    auto* envHandler = new EnvCreatedHandler(
            [hwnd, win_id](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) return result;

                auto* controllerHandler = new ControllerCreatedHandler(
                        [hwnd, win_id](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || !controller) return result;

                            ComPtr<ICoreWebView2> webview;
                            controller->get_CoreWebView2(&webview);

							ComPtr<ICoreWebView2Controller2> controller2;
							HRESULT hrC2 = controller->QueryInterface(IID_ICoreWebView2Controller2, reinterpret_cast<void**>(controller2.GetAddressOf()));
							if (FAILED(hrC2) || !controller2) {
								hrC2 = controller->QueryInterface(IID_ICoreWebView2Controller2_Local, reinterpret_cast<void**>(controller2.GetAddressOf()));
							}
							if (SUCCEEDED(hrC2) && controller2) {
								COREWEBVIEW2_COLOR transparent = { 0, 0, 0, 0 };
								controller2->put_DefaultBackgroundColor(transparent);
								Log("put_DefaultBackgroundColor: BERHASIL set transparan.");
							} else {
								Log("put_DefaultBackgroundColor: GAGAL query controller2, hr=0x" + std::to_string(hrC2));
							}

                            RECT bounds;
                            GetClientRect(hwnd, &bounds);
                            controller->put_Bounds(bounds);
                            controller->put_IsVisible(TRUE);

                            ComPtr<ICoreWebView2Settings> settings;
                            webview->get_Settings(&settings);
                            settings->put_AreDefaultContextMenusEnabled(FALSE);
                            settings->put_AreDevToolsEnabled(TRUE);
                            settings->put_IsStatusBarEnabled(FALSE);

                            // --- TAMBAHAN: Tangkap navigasi keluar & buka di browser default (Edge/Chrome) ---
                            EventRegistrationToken navToken{};
                            auto* navHandler = new NavigationStartingHandler(
                                [](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                    LPWSTR uri = nullptr;
                                    args->get_Uri(&uri);
                                    if (uri && !isInternalLocalUri(uri)) {
                                        args->put_Cancel(TRUE);
                                        ShellExecuteW(NULL, L"open", uri, NULL, NULL, SW_SHOWNORMAL);
                                    }
                                    if (uri) CoTaskMemFree(uri);
                                    return S_OK;
                                });
                            webview->add_NavigationStarting(navHandler, &navToken);
                            navHandler->Release();

                            EventRegistrationToken newWinToken{};
                            auto* newWinHandler = new NewWindowRequestedHandler(
                                [](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                                    LPWSTR uri = nullptr;
                                    args->get_Uri(&uri);
                                    if (uri) {
                                        args->put_Handled(TRUE); // cegah WebView2 popup jendela baru
                                        ShellExecuteW(NULL, L"open", uri, NULL, NULL, SW_SHOWNORMAL);
                                        CoTaskMemFree(uri);
                                    }
                                    return S_OK;
                                });
                            webview->add_NewWindowRequested(newWinHandler, &newWinToken);
                            newWinHandler->Release();
                            // --- AKHIR TAMBAHAN ---

                            std::wstring url = L"http://127.0.0.1:" + std::to_wstring(g_server_port) + L"/?win=" + std::to_wstring(win_id);
                            webview->Navigate(url.c_str());

                            {
                                std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
                                if (g_windows.find(win_id) != g_windows.end()) {
                                    g_windows[win_id].controller = controller;
                                    g_windows[win_id].webview = webview;
                                }
                            }
                            return S_OK;
                        });
                env->CreateCoreWebView2Controller(hwnd, controllerHandler);
                controllerHandler->Release();
                return S_OK;
            });
    // Lewatkan nullptr untuk opsi default resmi WebView2 guna mencegah crash 0xc0000005 di WebView2Loader.dll
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, userDataFolder.c_str(), nullptr, envHandler);
    envHandler->Release();
}

// =============================================================================
// Super+Drag Move/Resize (Windows tidak punya fitur ini native seperti GNOME/KDE)
// =============================================================================
static HHOOK g_mouse_hook = NULL;
static HHOOK g_keyboard_hook = NULL;
static bool g_drag_active = false;
static bool g_drag_is_resize = false;
static POINT g_drag_start_cursor = {0, 0};
static RECT g_drag_start_rect = {0, 0, 0, 0};
static HWND g_drag_hwnd = NULL;
static int g_drag_corner = 0; // 0=BR,1=BL,2=TR,3=TL
static bool g_super_click_consumed = false;
static bool g_super_physically_down = false;

static HWND findWindowUnderPoint(POINT pt) {
    // 1. Tanya OS Windows: Window apa yang secara fisik (Z-Order tertinggi) ada di titik ini?
    HWND hit = WindowFromPoint(pt);
    if (!hit) return NULL;

    // 2. Karena area dalam (seperti grafik/WebView2) dihitung sebagai "anak/child window",
    // kita harus tarik terus ke atas sampai menemukan "induk" tertingginya (Root Window).
    hit = GetAncestor(hit, GA_ROOT);

    // 3. Validasi: Pastikan window yang ditunjuk benar-benar milik aplikasi kita.
    std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
    for (auto& [id, w] : g_windows) {
        if (w.hwnd == hit) return w.hwnd;
    }
    
    return NULL;
}

static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT* ms = (MSLLHOOKSTRUCT*)lParam;
        bool superDown = g_super_physically_down;

        if (!g_drag_active && superDown && (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN)) {
            HWND target = findWindowUnderPoint(ms->pt);
            if (target) {
                g_drag_active = true;
                g_super_click_consumed = true; // klik ini jatuh di window kita -> Super key-up nanti diblok
                g_drag_is_resize = (wParam == WM_RBUTTONDOWN);
                g_drag_start_cursor = ms->pt;
                GetWindowRect(target, &g_drag_start_rect);
                g_drag_hwnd = target;
                if (g_drag_is_resize) {
                    int midX = (g_drag_start_rect.left + g_drag_start_rect.right) / 2;
                    int midY = (g_drag_start_rect.top + g_drag_start_rect.bottom) / 2;
                    bool right = ms->pt.x > midX, bottom = ms->pt.y > midY;
                    g_drag_corner = (bottom ? 0 : 2) + (right ? 0 : 1);
                }
                return 1;
            }
        } else if (g_drag_active && wParam == WM_MOUSEMOVE) {
            int dx = ms->pt.x - g_drag_start_cursor.x;
            int dy = ms->pt.y - g_drag_start_cursor.y;
            RECT r = g_drag_start_rect;
            if (!g_drag_is_resize) {
                r.left += dx; r.right += dx; r.top += dy; r.bottom += dy;
            } else {
                const int MINW = 260, MINH = 300;
                switch (g_drag_corner) {
                    case 0: r.right += dx; r.bottom += dy; break;
                    case 1: r.left += dx; r.bottom += dy; break;
                    case 2: r.right += dx; r.top += dy; break;
                    case 3: r.left += dx; r.top += dy; break;
                }
                if (r.right - r.left < MINW) { if (g_drag_corner == 1 || g_drag_corner == 3) r.left = r.right - MINW; else r.right = r.left + MINW; }
                if (r.bottom - r.top < MINH) { if (g_drag_corner == 2 || g_drag_corner == 3) r.top = r.bottom - MINH; else r.bottom = r.top + MINH; }
            }
            SetWindowPos(g_drag_hwnd, NULL, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE);
        } else if (g_drag_active && (wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP)) {
            g_drag_active = false;
            g_drag_hwnd = NULL;
            saveWindowConfig(); // fix: resize custom (Super+klik-kanan) gak lewat WM_EXITSIZEMOVE
            return 1;
        }
    }
    return CallNextHookEx(g_mouse_hook, nCode, wParam, lParam);
}

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
        bool isSuperKey = (kb->vkCode == VK_LWIN || kb->vkCode == VK_RWIN);

        if (isSuperKey) {
            bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
            bool isKeyUp   = (wParam == WM_KEYUP   || wParam == WM_SYSKEYUP);

            if (isKeyDown) {
                g_super_physically_down = true;
            } else if (isKeyUp) {
                g_super_physically_down = false; // update state kita DULU, sebelum keputusan swallow

                if (g_super_click_consumed) {
                    g_super_click_consumed = false; // sekali pakai, lalu reset
                    return 1; // swallow -> Start Menu/Search tidak terbuka
                }
            }
        }
    }
    return CallNextHookEx(g_keyboard_hook, nCode, wParam, lParam);
}

// =============================================================================
// WinMain Entry Point
// =============================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    g_hInstance = hInstance;

    // Gunakan format heksadesimal 8-digit ARGB (Alpha 00 = 100% transparan)
    SetEnvironmentVariableW(L"WEBVIEW2_DEFAULT_BACKGROUND_COLOR", L"0x00000000");

    Log("---------------------------------------------------");
    Log("wWinMain START, PID=" + std::to_string(GetCurrentProcessId()) +
        ", IsAdmin=" + std::string(IsUserAnAdmin() ? "YES" : "NO"));
	
	HANDLE hSingleInstanceMutex = CreateMutexW(NULL, TRUE, L"RizkybyMONITOR_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Instance lain (mungkin elevated, di background) udah jalan — jangan buka lagi, keluar diam-diam
        Log("EXIT: instance lain sudah jalan (mutex sudah ada). Proses ini keluar.");
        return 0;
    }
    Log("Mutex OK: ini instance pertama/satu-satunya.");

    // WAJIB admin: ATA PASS THROUGH (SMART/TBW) & battery IOCTL gagal diam2 tanpa elevasi
    if (!IsUserAnAdmin()) {
        Log("Belum admin -> mencoba relaunch via ShellExecuteExW(runas)...");
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = L"runas";
        sei.lpFile = exePath;
        sei.nShow = SW_SHOWNORMAL;
        if (ShellExecuteExW(&sei)) {
            Log("EXIT: relaunch elevated BERHASIL dipicu. Proses non-admin ini keluar.");
            return 0; // instance baru (elevated) akan jalan gantiin, instance ini keluar
        } else {
            Log("PERINGATAN: ShellExecuteExW GAGAL (err=" + std::to_string(GetLastError()) +
                "). Kemungkinan UAC di-Cancel. App lanjut jalan TANPA admin (SMART/TBW/battery bakal gagal diam-diam).");
        }
    }
    Log("Lanjut ke inisialisasi utama (CoInitializeEx, WebView2, HTTP server)...");

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    g_app_dir = wstrToStr(exePath);

    // Data writable per-user (config.json JANGAN ditaruh di folder exe, bisa read-only)
    g_data_dir = g_app_dir;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // Inisialisasi awal surface DWM sebagai alpha=0
    wc.lpszClassName = WINDOW_CLASS;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    g_mouse_hook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
    g_keyboard_hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	
	StartDiskIoEtwSession();

    // Jalankan HTTP Server di background thread
    std::thread(startHttpServer).detach();

    // Telemetri awal
    updateTelemetry();

    // Background hardware caching
    std::thread([]() {
        initHardwareInfo();
        updateTelemetry();
    }).detach();
    std::thread(telemetryLoop).detach();

    // Baca konfigurasi multi-window dari array active_windows
    std::vector<int> active_ids;
    std::string cfg = readFile(g_data_dir + "\\config.json");
    if (!cfg.empty()) {
        size_t awp = cfg.find("\"active_windows\"");
        if (awp != std::string::npos) {
            size_t start = cfg.find('[', awp);
            size_t end = cfg.find(']', start);
            if (start != std::string::npos && end != std::string::npos) {
                std::string arr = cfg.substr(start + 1, end - start - 1);
                std::stringstream ss(arr);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    int id = std::atoi(item.c_str());
                    if (id > 0) active_ids.push_back(id);
                }
            }
        }
    }
    
    // Fallback darurat jika ini peluncuran pertama atau JSON rusak
    if (active_ids.empty()) {
        active_ids.push_back(1);
    }

    g_next_win_id = 0;
    // Buka kembali SEMUA window secara mandiri berdasarkan ID unik aslinya
    for (int id : active_ids) {
        if (id > g_next_win_id) g_next_win_id = id;
        createNewWindow(id);
    }

    // Windows Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_mouse_hook) UnhookWindowsHookEx(g_mouse_hook);
    if (g_keyboard_hook) UnhookWindowsHookEx(g_keyboard_hook);
    g_running = false;
    saveWindowConfig();
    CoUninitialize();
    return 0;
}

int main() {
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWDEFAULT);
}

#else
int main() {
    std::cerr << "Error: main_windows.cpp should only be compiled on Windows.\n";
    std::cerr << "Use src/main_linux.cpp for Linux builds.\n";
    return 1;
}
#endif // _WIN32
