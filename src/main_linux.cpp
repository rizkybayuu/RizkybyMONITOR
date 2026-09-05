#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <iomanip>

// GTK & WebKit2GTK – adaptif untuk semua distro
#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdk.h>
#include <math.h>

#if __has_include(<webkit2/webkit2.h>)
#include <webkit2/webkit2.h>
#elif __has_include(<webkit2gtk/webkit2.h>)
#include <webkit2gtk/webkit2.h>
#elif __has_include(<webkit2gtk-4.0/webkit2gtk.h>)
#include <webkit2gtk-4.0/webkit2gtk.h>
#else
#error "WebKit2GTK headers not found"
#endif

// -------------------------------------------------------------
// Data Structures & Global Variables
// -------------------------------------------------------------
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
    GtkWidget* window;
    GtkWidget* webview;
    bool is_on_top;
    std::string details = "[]";
    std::string mode = "dark";
    int font_size = 14;
    std::string selected_disk = "sda";
    std::string app_title = "";
};

static std::recursive_mutex g_win_mutex;
static std::map<int, WindowInstance> g_windows;
static int g_next_win_id = 1;
static std::string g_app_dir;
static int g_server_port = 8080;
static bool g_running = true;
static std::atomic<int> g_telemetry_interval_ms{1000};

// Metrics Cache Protected by Mutex
static std::mutex g_stats_mutex;
static std::string g_cached_stats_json;
static std::map<std::string, std::string> g_disk_details_map;

static std::map<std::string, CpuTick> g_last_cpu_ticks;
static std::map<int, ProcIoTick> g_last_proc_io_ticks;

static double g_last_net_time = 0.0;
static unsigned long long g_last_net_rx = 0;
static unsigned long long g_last_net_tx = 0;
static double g_cur_rx_rate = 0.0;
static double g_cur_tx_rate = 0.0;

static double g_last_disk_time = 0.0;
static unsigned long long g_last_disk_read = 0;
static unsigned long long g_last_disk_write = 0;
static std::map<std::string, std::pair<unsigned long long, unsigned long long>> g_last_dev_diskstats;
static double g_cur_disk_read_rate = 0.0;
static double g_cur_disk_write_rate = 0.0;

static std::string g_cpu_model = "";
static std::string g_gpu_model = "";
static std::string g_root_dev = "";

// Detailed Hardware Cache
static std::string g_cache_raw_cpu = "";
static std::string g_cache_raw_gpu = "";
static std::string g_cache_raw_ram = "";
static std::string g_cache_raw_net = "";
static std::string g_cache_raw_disk = "";
static std::string g_cache_ram_type_json = "";
static std::string g_cache_network_type = "";
static std::string g_cache_battery_tech = "";

// -------------------------------------------------------------
// Helper Functions
// -------------------------------------------------------------
static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    if (f.is_open()) {
        f << content;
    }
}

static std::string execCmd(const std::string& cmd) {
    char buffer[512];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// =============================================================================
// CPU Core Topology (Linux Kernel Hybrid Detection) — setara detectCpuCoreTypes()
// di versi Windows, tapi sumbernya dari kernel sysfs asli, bukan hardcoded.
// =============================================================================
struct CoreTopologyInfo {
    std::string tag;   // "P", "E", atau "C" (kalau CPU homogen/non-hybrid)
    std::string type;  // "p-core", "e-core"
};

static std::set<int> parseCpuRangeList(const std::string& raw) {
    std::set<int> ids;
    std::string s = trim(raw);
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        size_t dash = token.find('-');
        if (dash != std::string::npos) {
            int a = std::atoi(token.substr(0, dash).c_str());
            int b = std::atoi(token.substr(dash + 1).c_str());
            for (int i = a; i <= b; i++) ids.insert(i);
        } else if (!token.empty()) {
            ids.insert(std::atoi(token.c_str()));
        }
    }
    return ids;
}

static std::vector<CoreTopologyInfo> getDynamicCpuTopology(int total_logical) {
    // Default: homogen (bukan hybrid) -> semua ditandai "C" (Common/Core biasa),
    // PERSIS seperti default Windows kalau CPU tidak hybrid.
    std::vector<CoreTopologyInfo> topology(total_logical, { "C", "p-core" });

    // Kernel Intel hybrid (5.16+) mengekspos ini secara native lewat sysfs.
    // Kosong = bukan hybrid / kernel tidak support -> tetap homogen (fallback jujur).
    std::string pcore_raw = readFile("/sys/devices/cpu_core/cpus");
    std::string ecore_raw = readFile("/sys/devices/cpu_atom/cpus");

    bool is_hybrid = !trim(pcore_raw).empty() || !trim(ecore_raw).empty();
    if (!is_hybrid) return topology;

    std::set<int> pcores = parseCpuRangeList(pcore_raw);
    std::set<int> ecores = parseCpuRangeList(ecore_raw);

    for (int i = 0; i < total_logical; i++) {
        if (ecores.count(i)) {
            topology[i] = { "E", "e-core" };
        } else if (pcores.count(i)) {
            topology[i] = { "P", "p-core" };
        }
        // kalau core index tidak ada di kedua list (kasus langka), biarkan default "C"
    }
    return topology;
}

static bool isRootDiskDevice(const std::string& name) {
    if (name.empty()) return false;
    if (name.rfind("loop", 0) == 0 || name.rfind("zram", 0) == 0 || name.rfind("ram", 0) == 0 || name.rfind("dm-", 0) == 0) return false;
    if (name.rfind("nvme", 0) == 0 || name.rfind("mmcblk", 0) == 0) {
        return (name.find('p') == std::string::npos);
    }
    return !isdigit(name.back());
}

static std::string getOsPrettyName() {
    std::string content = readFile("/etc/os-release");
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.rfind("PRETTY_NAME=", 0) == 0) {
            std::string val = line.substr(12);
            if (!val.empty() && val.front() == '"') val.erase(0, 1);
            if (!val.empty() && val.back() == '"') val.pop_back();
            return val;
        }
    }
    return "Unknown Linux";
}

static long getUptimeSeconds() {
    std::string content = readFile("/proc/uptime");
    return content.empty() ? 0L : std::strtol(content.c_str(), nullptr, 10);
}

static std::string escapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if ((unsigned char)c < 32) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
            out += buf;
        } else {
            out += c;
        }
    }
    return out;
}

static std::string formatSpeed(double bytes_per_sec) {
    char buf[64];
    if (bytes_per_sec >= 1024.0 * 1024.0 * 1024.0) {
        snprintf(buf, sizeof(buf), "%.2f GB/s", bytes_per_sec / (1024.0 * 1024.0 * 1024.0));
    } else if (bytes_per_sec >= 1024.0 * 1024.0) {
        snprintf(buf, sizeof(buf), "%.1f MB/s", bytes_per_sec / (1024.0 * 1024.0));
    } else if (bytes_per_sec >= 1024.0) {
        snprintf(buf, sizeof(buf), "%.1f KB/s", bytes_per_sec / 1024.0);
    } else {
        snprintf(buf, sizeof(buf), "%.0f B/s", bytes_per_sec);
    }
    return std::string(buf);
}

static std::string formatBytesDynamic(double bytes) {
    char buf[64];
    if (bytes <= 0) return "0 B";
    if (bytes >= 1024.0 * 1024.0 * 1024.0) {
        snprintf(buf, sizeof(buf), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    } else if (bytes >= 1024.0 * 1024.0) {
        snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024.0) {
        snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
    } else {
        snprintf(buf, sizeof(buf), "%.0f B", bytes);
    }
    return std::string(buf);
}

static double getTimeSec() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now.time_since_epoch()).count();
}

// -------------------------------------------------------------
// Telemetry & Hardware Spec Probing
// -------------------------------------------------------------
struct SmartDriveInfo {
    std::string tbw_str;
    std::string remaining_str;
    std::string temp_str;
    std::string health_str;
};

static std::map<std::string, std::pair<double, SmartDriveInfo>> s_smart_cache;

static SmartDriveInfo probeDriveSmart(const std::string& dname) {
    double now = getTimeSec();
    if (s_smart_cache.find(dname) != s_smart_cache.end()) {
        if (now - s_smart_cache[dname].first < 60.0) {
            return s_smart_cache[dname].second;
        }
    }

    SmartDriveInfo info;
    info.tbw_str = "High-Speed Flash Storage";
    info.remaining_str = "N/A";
    info.temp_str = "N/A";
    info.health_str = "Unknown (Not Verified)";

    bool is_nvme = (dname.find("nvme") != std::string::npos);

    // JANGAN hardcode berdasarkan nama device (sda/sdb/dst) — nama device
    // bisa beda di tiap PC tergantung urutan enumerasi USB/SATA saat boot,
    // dan device di PC lain belum tentu punya bridge chip yang sama.
    // JANGAN JUGA menebak "-d sat"/"-d usbjmicron"/"-T permissive" —
    // command ATA-passthrough yang salah ke bridge yang tidak kompatibel
    // terbukti bisa bikin kernel panic (lihat riwayat testing sebelumnya).
    //
    // Solusi aman & universal: tanya ke smartctl sendiri lewat --scan-open,
    // yang punya database bridge USB dan akan kasih tahu tipe -d yang
    // benar KHUSUS untuk device ini, di PC manapun ia dijalankan.
    std::string scan_out = execCmd("timeout 1.0 smartctl --scan-open 2>/dev/null");
    std::string auto_flag; // kosong = smartctl tidak kenal bridge-nya
    {
        std::istringstream sc_iss(scan_out);
        std::string sline;
        while (std::getline(sc_iss, sline)) {
            if (sline.rfind("/dev/" + dname + " ", 0) == 0) {
                size_t dpos = sline.find("-d ");
                if (dpos != std::string::npos) {
                    size_t end = sline.find(' ', dpos + 3);
                    auto_flag = sline.substr(dpos + 3, end - (dpos + 3));
                }
                break;
            }
        }
    }

    std::string cmd;
    if (!auto_flag.empty()) {
        cmd = "timeout 1.0 smartctl -a -d " + auto_flag + " /dev/" + dname + " 2>/dev/null";
    } else {
        cmd = "timeout 1.0 smartctl -a /dev/" + dname + " 2>/dev/null";
    }
    std::string smart_out = execCmd(cmd);
    // Kalau smart_out tetap kosong di sini (bridge tidak dikenali sama
    // sekali oleh smartctl), kode di bawah akan otomatis jatuh ke fallback
    // kernel-counter (/sys/block/<dev>/stat) yang sudah ada — itu 100%
    // aman untuk device apapun karena tidak kirim command ke hardware.

    if (!smart_out.empty()) {
        double tbw_val = -1;
        int pct_used = -1;
        std::istringstream iss(smart_out);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("Temperature:") != std::string::npos) {
                size_t c = line.find(':');
                if (c != std::string::npos) {
                    int t = 0;
                    if (sscanf(line.c_str() + c + 1, "%d", &t) == 1 && t > 0 && t < 120) {
                        info.temp_str = std::to_string(t) + " °C";
                    }
                }
            }
            if (line.find("Data Units Written:") != std::string::npos) {
                size_t b1 = line.find('[');
                size_t b2 = line.find(']', b1);
                if (b1 != std::string::npos && b2 != std::string::npos) {
                    info.tbw_str = line.substr(b1 + 1, b2 - b1 - 1) + " Written";
                    sscanf(line.substr(b1 + 1).c_str(), "%lf", &tbw_val);
                }
            }
            // Fallback ATA SSD: Total_LBAs_Written (1 LBA = 512 bytes)
            if (tbw_val < 0 && line.find("Total_LBAs_Written") != std::string::npos) {
                std::istringstream liss(line);
                std::string tok;
                std::vector<std::string> toks;
                while (liss >> tok) toks.push_back(tok);
                if (toks.size() >= 10) {
                    try {
                        unsigned long long lbas = std::stoull(toks[9]);
                        tbw_val = (double)(lbas * 512ULL) / (1024.0*1024.0*1024.0*1024.0);
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%.3f TB Written", tbw_val);
                        info.tbw_str = std::string(buf);
                    } catch (...) {}
                }
            }
            if (line.find("Percentage Used:") != std::string::npos) {
                size_t c = line.find(':');
                if (c != std::string::npos) {
                    sscanf(line.c_str() + c + 1, "%d%%", &pct_used);
                }
            }
            if (line.find("self-assessment test result:") != std::string::npos) {
                size_t c = line.find(':');
                if (c != std::string::npos) {
                    info.health_str = trim(line.substr(c + 1));
                }
            }
        }

        if (tbw_val > 0 && pct_used > 0) {
            double total_rated = tbw_val / ((double)pct_used / 100.0);
            double rem_tb = std::max(0.0, total_rated - tbw_val);
            char buf[64];
            snprintf(buf, sizeof(buf), "%.3f TB Life Remaining (%d%% Remaining)", rem_tb, std::max(0, 100 - pct_used));
            info.remaining_str = std::string(buf);
        } else if (tbw_val > 0) {
            info.remaining_str = "N/A (Drive wear percentage not reported)";
        }
    }

    // Fallback JUJUR kalau smartctl gagal/tidak lengkap: baca counter kernel asli,
    // bukan angka konstan. Tidak ada nomor yang dikarang di sini.
    if (info.tbw_str == "High-Speed Flash Storage") {
        unsigned long long total_bytes = 0;

        // 1) Sektor tulis fisik dari /sys/block/<dev>/stat (field ke-7, index 6)
        std::string stat_str = readFile("/sys/block/" + dname + "/stat");
        if (!stat_str.empty()) {
            std::istringstream s_iss(stat_str);
            std::string tok;
            std::vector<std::string> toks;
            while (s_iss >> tok) toks.push_back(tok);
            if (toks.size() >= 7) {
                try { total_bytes = std::stoull(toks[6]) * 512ULL; } catch (...) {}
            }
        }

        // 2) Akumulasi lifetime write ext4 kalau ada partisi dari disk ini
        if (total_bytes == 0) {
            DIR* ext4_dir = opendir("/sys/fs/ext4");
            if (ext4_dir) {
                struct dirent* fe;
                while ((fe = readdir(ext4_dir)) != NULL) {
                    if (fe->d_name[0] == '.') continue;
                    std::string pname = fe->d_name;
                    if (pname.rfind(dname, 0) == 0) {
                        std::string lw = trim(readFile("/sys/fs/ext4/" + pname + "/lifetime_write_kbytes"));
                        if (!lw.empty()) total_bytes += std::strtoull(lw.c_str(), NULL, 10) * 1024ULL;
                    }
                }
                closedir(ext4_dir);
            }
        }

        if (total_bytes > 0) {
            double tb = (double)total_bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0);
            char buf[64];
            if (tb >= 1.0) snprintf(buf, sizeof(buf), "%.3f TB Written (Live Session)", tb);
            else snprintf(buf, sizeof(buf), "%.2f GB Written (Live Session)", tb * 1024.0);
            info.tbw_str = buf;

            info.remaining_str = "N/A (The rated TBW of the drive is unknown in the absence of official vendor data)";
        } else {
            info.tbw_str = "N/A (SMART unavailable, no kernel counter)";
        }

        info.health_str = "Unknown (SMART unavailable)";
    }

    // Suhu: kalau smartctl tidak melaporkan "Temperature:" sama sekali (baik karena
    // smartctl gagal total, atau berhasil tapi drive tidak expose suhu di output),
    // coba hwmon asli. Kalau memang tidak ada sensornya, biarkan "N/A" — jangan dikarang.
    if (info.temp_str == "N/A") {
        DIR* hw_dir = opendir("/sys/class/hwmon");
        if (hw_dir) {
            struct dirent* he;
            while ((he = readdir(hw_dir)) != NULL) {
                if (he->d_name[0] == '.') continue;
                std::string hpath = std::string("/sys/class/hwmon/") + he->d_name;
                std::string hname = trim(readFile(hpath + "/name"));
                if (hname == "nvme" || hname.find(dname) != std::string::npos || hname.find("drivetemp") != std::string::npos) {
                    std::string t1 = trim(readFile(hpath + "/temp1_input"));
                    if (!t1.empty()) {
                        int v = std::atoi(t1.c_str());
                        if (v > 1000) v /= 1000;
                        if (v > 0 && v < 120) info.temp_str = std::to_string(v) + " °C";
                    }
                }
            }
            closedir(hw_dir);
        }
    }

    s_smart_cache[dname] = {now, info};
    return info;
}

static std::string buildDiskDetailText(const std::string& dname) {
    SmartDriveInfo sinfo = probeDriveSmart(dname);

    std::stringstream detail_lines;
    detail_lines << "==================================================\n";
    detail_lines << "💽 STORAGE DEVICE TELEMETRY: /dev/" << dname << "\n";
    detail_lines << "==================================================\n";
    detail_lines << "SMART Health Status: " << sinfo.health_str << "\n";
    detail_lines << "Operating Temp     : " << sinfo.temp_str << "\n";
    detail_lines << "Total Bytes Written: " << sinfo.tbw_str << "\n";
    if (sinfo.remaining_str != "N/A") {
        detail_lines << "Remaining Endurance: " << sinfo.remaining_str << "\n";
    }
    detail_lines << "\n=== 1. PARTITION STRUCTURE, FILE SYSTEMS & UUIDs ===\n";
    detail_lines << execCmd("lsblk -o NAME,LABEL,FSTYPE,SIZE,FSAVAIL,FSUSE%,MOUNTPOINT,UUID,PARTUUID /dev/" + dname + " 2>/dev/null");
    detail_lines << "\n=== 2. KERNEL BLOCK QUEUE & I/O SCHEDULER SPECS ===\n";
    detail_lines << execCmd("lsblk -o NAME,PHY-SEC,LOG-SEC,SCHED,RQ-SIZE,RA,DISC-GRAN,DISC-MAX /dev/" + dname + " 2>/dev/null");
    detail_lines << "\n=== 3. MOUNTED FILESYSTEM USAGE & AVAILABLE SPACE ===\n";
    detail_lines << execCmd("df -hT 2>/dev/null | grep '/dev/" + dname + "'");
    detail_lines << "\n=== 4. HARDWARE IDENTITY & BUS PROPERTIES ===\n";
    detail_lines << execCmd("udevadm info --query=property --name=/dev/" + dname + " 2>/dev/null | grep -E 'ID_MODEL|ID_SERIAL|ID_REVISION|ID_VENDOR|ID_USB|ID_BUS|ID_FS|ID_PATH'");

    return detail_lines.str();
}

static std::string buildMemoryDetailText() {
    std::stringstream d;
    d << "==================================================\n";
    d << "🧠 MEMORY HIERARCHY: PHYSICAL RAM / ZRAM / SWAP\n";
    d << "==================================================\n\n";

    // --- Section 1: Physical RAM Overview ---
    d << "=== 1. PHYSICAL RAM OVERVIEW ===\n";
    std::string free_out = execCmd("free -h --si 2>/dev/null");
    if (free_out.empty()) free_out = execCmd("free -h 2>/dev/null");
    if (!free_out.empty()) d << free_out << "\n";

    // --- Section 2: DIMM Slot Details (dmidecode) ---
    d << "\n=== 2. DIMM SLOT DETAILS (Physical Modules) ===\n";
    std::string dmi_out = execCmd("timeout 1 dmidecode -t memory 2>/dev/null | grep -A 20 'Memory Device' | grep -E 'Size|Type|Speed|Manufacturer|Part Number|Locator|Form Factor|Rank|Voltage' 2>/dev/null");
    if (!dmi_out.empty()) {
        d << dmi_out << "\n";
    } else {
        // Fallback: try lshw
        std::string lshw_out = execCmd("timeout 1 lshw -short -class memory 2>/dev/null");
        if (!lshw_out.empty()) {
            d << lshw_out << "\n";
        } else {
            d << "  (Run with sudo for full DIMM details via dmidecode)\n";
            d << "  N/A - RAM slot details are not available without root access\n";
        }
    }

    // --- Section 3: Kernel Memory Statistics ---
    d << "\n=== 3. KERNEL MEMORY STATISTICS (/proc/meminfo) ===\n";
    std::string meminfo = readFile("/proc/meminfo");
    if (!meminfo.empty()) {
        // Extract key fields
        std::istringstream mis(meminfo);
        std::string line;
        std::vector<std::string> important_keys = {
            "MemTotal", "MemFree", "MemAvailable", "Buffers", "Cached",
            "SwapCached", "Active", "Inactive", "Active(anon)", "Inactive(anon)",
            "Active(file)", "Inactive(file)", "Dirty", "Writeback",
            "AnonPages", "Mapped", "Shmem", "KReclaimable", "Slab",
            "SReclaimable", "SUnreclaim", "KernelStack", "PageTables",
            "CommitLimit", "Committed_AS", "VmallocTotal", "VmallocUsed",
            "Hugepagesize", "DirectMap4k", "DirectMap2M", "DirectMap1G"
        };
        while (std::getline(mis, line)) {
            for (auto& key : important_keys) {
                if (line.rfind(key, 0) == 0 || line.find(key + ":") != std::string::npos) {
                    d << "  " << line << "\n";
                    break;
                }
            }
        }
    }

    // --- Section 4: ZRAM Compressed Swap ---
    d << "\n=== 4. ZRAM COMPRESSED SWAP ENGINE ===\n";
    std::string zramctl_out = execCmd("zramctl 2>/dev/null");
    if (!zramctl_out.empty()) {
        d << zramctl_out << "\n";
    } else {
        // Fallback: manual /sys reading
        std::string zram_disksize = readFile("/sys/block/zram0/disksize");
        std::string zram_algo = readFile("/sys/block/zram0/comp_algorithm");
        if (!zram_disksize.empty()) {
            unsigned long long zram_bytes = std::strtoull(zram_disksize.c_str(), NULL, 10);
            double zram_gb = (double)zram_bytes / (1024.0 * 1024.0 * 1024.0);
            d << "  Device     : /dev/zram0\n";
            char zbuf[32]; snprintf(zbuf, sizeof(zbuf), "%.1f GB", zram_gb);
            d << "  Disk Size  : " << zbuf << "\n";
            d << "  Algorithm  : " << trim(zram_algo) << "\n";

            std::string orig = readFile("/sys/block/zram0/mm_stat");
            if (!orig.empty()) {
                d << "  MM Stats   : " << trim(orig) << "\n";
            }
        } else {
            d << "  ZRAM: Not configured or not available\n";
        }
    }
    // ZRAM compression ratio
    std::string zram_orig = readFile("/sys/block/zram0/orig_data_size");
    std::string zram_comp = readFile("/sys/block/zram0/compr_data_size");
    if (!zram_orig.empty() && !zram_comp.empty()) {
        unsigned long long orig_bytes = std::strtoull(zram_orig.c_str(), NULL, 10);
        unsigned long long comp_bytes = std::strtoull(zram_comp.c_str(), NULL, 10);
        if (comp_bytes > 0) {
            double ratio = (double)orig_bytes / (double)comp_bytes;
            char rbuf[32]; snprintf(rbuf, sizeof(rbuf), "%.2fx", ratio);
            d << "  Compression Ratio: " << rbuf << " (" << (orig_bytes / 1048576) << " MB original → " << (comp_bytes / 1048576) << " MB compressed)\n";
        }
    }

    // --- Section 5: All Swap Devices ---
    d << "\n=== 5. ACTIVE SWAP DEVICES & PRIORITIES ===\n";
    std::string swapon_out = execCmd("swapon --show 2>/dev/null");
    if (!swapon_out.empty()) {
        d << swapon_out << "\n";
    } else {
        std::string proc_swaps = readFile("/proc/swaps");
        if (!proc_swaps.empty()) d << proc_swaps << "\n";
    }

    // --- Section 6: NUMA / Memory Topology ---
    d << "\n=== 6. MEMORY TOPOLOGY & NUMA ===\n";
    std::string numa_out = execCmd("lscpu 2>/dev/null | grep -iE 'NUMA|node|L1|L2|L3|cache' 2>/dev/null");
    if (!numa_out.empty()) {
        d << numa_out << "\n";
    }
    // Cache hierarchy
    for (int i = 0; i < 4; i++) {
        std::string ctype = readFile("/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i) + "/type");
        std::string csize = readFile("/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i) + "/size");
        std::string clevel = readFile("/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i) + "/level");
        if (!ctype.empty() && !csize.empty()) {
            d << "  L" << trim(clevel) << " " << trim(ctype) << " Cache: " << trim(csize) << "\n";
        }
    }

    // --- Section 7: Kernel VM Tunables ---
    d << "\n=== 7. KERNEL VM TUNABLES ===\n";
    std::string swappiness = readFile("/proc/sys/vm/swappiness");
    std::string vfs_cache = readFile("/proc/sys/vm/vfs_cache_pressure");
    std::string dirty_ratio = readFile("/proc/sys/vm/dirty_ratio");
    std::string dirty_bg = readFile("/proc/sys/vm/dirty_background_ratio");
    std::string overcommit = readFile("/proc/sys/vm/overcommit_memory");
    if (!swappiness.empty()) d << "  vm.swappiness          = " << trim(swappiness) << "\n";
    if (!vfs_cache.empty()) d << "  vm.vfs_cache_pressure  = " << trim(vfs_cache) << "\n";
    if (!dirty_ratio.empty()) d << "  vm.dirty_ratio         = " << trim(dirty_ratio) << "\n";
    if (!dirty_bg.empty()) d << "  vm.dirty_background    = " << trim(dirty_bg) << "\n";
    if (!overcommit.empty()) d << "  vm.overcommit_memory   = " << trim(overcommit) << "\n";

    return d.str();
}

static std::string buildRamTypeJson() {
    std::string dmi_full = execCmd("timeout 1 dmidecode -t memory 2>/dev/null");
    if (dmi_full.empty()) return "[]";

    std::vector<std::string> slots;
    std::istringstream iss(dmi_full);
    std::string line, locator, size, type, speed;
    bool in_block = false;
    auto flush = [&]() {
        if (in_block && !size.empty() && size.find("No Module") == std::string::npos) {
            std::string entry = (locator.empty() ? "Slot" : locator) + ": " + size;
            if (!type.empty())  entry += " " + type;
            if (!speed.empty()) entry += " (" + speed + ")";
            slots.push_back(entry);
        }
        in_block = false; locator.clear(); size.clear(); type.clear(); speed.clear();
    };
    while (std::getline(iss, line)) {
        if (trim(line) == "Memory Device") { flush(); in_block = true; continue; }
        if (!in_block) continue;
        size_t c = line.find(':');
        if (c == std::string::npos) continue;
        std::string key = trim(line.substr(0, c));
        std::string val = trim(line.substr(c + 1));
        if (key == "Locator") locator = val;
        else if (key == "Size") size = val;
        else if (key == "Type" && val != "Unknown") type = val;
        else if (key == "Speed" && val != "Unknown") speed = val;
    }
    flush();

    if (slots.empty()) return "[]";
    std::string json = "[";
    for (size_t i = 0; i < slots.size(); i++) json += (i ? ", \"" : "\"") + slots[i] + "\"";
    json += "]";
    return json;
}

static void initHardwareInfo() {
    std::ifstream f("/proc/cpuinfo");
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            if (line.find("model name") != std::string::npos) {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    g_cpu_model = trim(line.substr(colon + 1));
                    break;
                }
            }
        }
    }

    std::string findmnt = execCmd("findmnt -n -o SOURCE / 2>/dev/null");
    findmnt = trim(findmnt);
    if (!findmnt.empty()) {
        size_t p = findmnt.rfind('/');
        std::string dname = (p != std::string::npos) ? findmnt.substr(p + 1) : findmnt;
        while (!dname.empty() && isdigit(dname.back())) {
            dname.pop_back();
        }
        if (!dname.empty() && dname.back() == 'p' && dname.find("nvme") != std::string::npos) {
            dname.pop_back();
        }
        if (!dname.empty()) g_root_dev = dname;
    }

    g_cache_raw_cpu = execCmd("lscpu 2>/dev/null");
    if (g_cache_raw_cpu.empty()) g_cache_raw_cpu = readFile("/proc/cpuinfo");

    g_cache_raw_gpu = execCmd("lspci -v -d ::0300 2>/dev/null");
    if (g_cache_raw_gpu.empty()) g_cache_raw_gpu = "N/A (lspci tidak tersedia/GPU tidak terdeteksi)";

    {
        std::istringstream gss(g_cache_raw_gpu);
        std::string gline;
        if (std::getline(gss, gline)) {
            size_t colon = gline.find(": ");
            if (colon != std::string::npos) {
                std::string name = gline.substr(colon + 2);
                size_t rev = name.find(" (rev");
                if (rev != std::string::npos) name = name.substr(0, rev);
                g_gpu_model = trim(name);
            }
        }
        if (g_gpu_model.empty()) g_gpu_model = "N/A (GPU tidak terdeteksi)";
    }

    g_cache_raw_ram = buildMemoryDetailText();
    g_cache_raw_net = execCmd("ip -br a 2>/dev/null; echo ''; ip a 2>/dev/null");
    g_cache_raw_disk = execCmd("lsblk -o NAME,LABEL,FSTYPE,SIZE,MOUNTPOINT,UUID,PARTUUID,MODEL,SERIAL 2>/dev/null; echo ''; df -hT 2>/dev/null | grep -v tmpfs");

    g_cache_ram_type_json = buildRamTypeJson();

    // Pre-cache details for nvme0n1, sda, sdb
    std::map<std::string, std::string> dt_map;
    DIR* sys_block = opendir("/sys/block");
    if (sys_block) {
        struct dirent* be;
        while ((be = readdir(sys_block)) != NULL) {
            std::string dname = be->d_name;
            if (dname == "." || dname == ".." || dname.rfind("loop", 0) == 0 || dname.rfind("zram", 0) == 0) continue;
            dt_map[dname] = buildDiskDetailText(dname);
        }
        closedir(sys_block);
    }
    {
        std::lock_guard<std::mutex> lock(g_stats_mutex);
        g_disk_details_map = dt_map;
    }
}

struct ProcEntry {
    std::string name;
    std::string val;
};

struct DiskProcEntry {
    std::string name;
    std::string read;
    std::string write;
    double total_bytes;
    int pid = 0;
};

struct NetProcEntry {
    std::string name;
    std::string down;
    std::string up;
    double total;
};

struct GpuProcEntry {
    std::string name;
    double rcs;
    double bcs;
    double vcs;
    double vecs;
    double total;
};

// SESUDAH — pindahkan KEDUA fungsi ke luar, taruh SEBELUM baris 674 (static void updateTelemetry() {)
static std::vector<std::pair<std::string,std::string>> getMountBaseDisks() {
    std::vector<std::pair<std::string,std::string>> mounts;
    std::ifstream mf("/proc/mounts");
    std::string ln;
    while (std::getline(mf, ln)) {
        std::istringstream iss(ln);
        std::string src, tgt, fstype;
        if (!(iss >> src >> tgt >> fstype)) continue;
        if (src.rfind("/dev/", 0) != 0) continue;
        std::string base = src.substr(5);
        size_t p = base.rfind('p');
        if (base.rfind("nvme", 0) == 0 && p != std::string::npos && p > base.find('n')) base = base.substr(0, p);
        else while (!base.empty() && isdigit(base.back())) base.pop_back();
        mounts.push_back({tgt, base});
    }
    std::sort(mounts.begin(), mounts.end(), [](auto&a, auto&b){ return a.first.size() > b.first.size(); });
    return mounts;
}

static std::string diskOfPid(int pid, const std::vector<std::pair<std::string,std::string>>& mounts) {
    std::string fd_dir = "/proc/" + std::to_string(pid) + "/fd";
    DIR* d = opendir(fd_dir.c_str());
    if (!d) return "";
    struct dirent* e; char buf[1024];
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        ssize_t len = readlink((fd_dir + "/" + e->d_name).c_str(), buf, sizeof(buf)-1);
        if (len <= 0) continue;
        buf[len] = '\0';
        std::string t(buf);
        for (auto& mp : mounts) {
            if (t.rfind(mp.first, 0) == 0) { closedir(d); return mp.second; }
        }
    }
    closedir(d);
    return "";
}

// ============================================================================
// TELEMETRY HARDWARE/KERNEL ASLI (MURNI READ SYSFS/PROC, TANPA TEBAKAN)
// ============================================================================
static unsigned long long getCpuSmartCacheTotalBytes() {
    // sysconf tetap prioritas #1 (paling akurat & murah)
    long l3 = sysconf(_SC_LEVEL3_CACHE_SIZE);
    if (l3 > 0) return (unsigned long long)l3;

    // Fallback: HARUS filter level=="3", jangan dijumlah semua level.
    // "Smart Cache" Intel = L3 doang, bukan L1+L2+L3.
    unsigned long long total_bytes = 0;
    for (int i = 0; i < 8; i++) {
        std::string base = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i);
        std::string lvl = trim(readFile(base + "/level"));
        if (lvl != "3") continue;
        std::string sz_str = trim(readFile(base + "/size"));
        if (sz_str.empty()) continue;

        unsigned long long bytes = 0;
        char unit = 0;
        if (sscanf(sz_str.c_str(), "%llu%c", &bytes, &unit) >= 1) {
            if (unit == 'K' || unit == 'k') bytes *= 1024ULL;
            else if (unit == 'M' || unit == 'm') bytes *= 1024ULL * 1024ULL;
            else if (unit == 'G' || unit == 'g') bytes *= 1024ULL * 1024ULL * 1024ULL;
            total_bytes += bytes;
        }
    }
    return total_bytes;
}

static std::string getCpuSmartCacheReal() {
    unsigned long long bytes = getCpuSmartCacheTotalBytes();
    if (bytes > 0) return formatBytesDynamic((double)bytes);
    return "N/A";
}

// AFTER — tambah struct + vector adapter tepat di bawahnya
struct RealGpuMem {
    unsigned long long dedicated_total = 0;
    unsigned long long dedicated_used  = 0;
    unsigned long long shared_total    = 0;
    unsigned long long shared_used     = 0;
    bool is_uma = false;
    bool has_dedicated = false;
};

struct GpuAdapterInfo {
    std::string id;
    std::string name;
    bool is_egpu;
    double dedicated_vram_gb;
    double shared_vram_gb;
    double usage_pct;
};
static std::vector<GpuAdapterInfo> g_gpu_adapters;

// Bagian STATIS (fakta hardware, gak berubah selama runtime) - diprobe SEKALI
static bool g_gpu_static_probed   = false;
static bool g_gpu_is_uma          = false;
static bool g_gpu_has_dedicated   = false;
static unsigned long long g_gpu_dedicated_total = 0;
static unsigned long long g_gpu_shared_total    = 0;

static void probeGpuStaticInfoOnce() {
    if (g_gpu_static_probed) return;
    g_gpu_static_probed = true;

    // 1. GLX Mesa Query - HANYA SEKALI, bukan tiap tick
    std::string glx_out = execCmd("glxinfo -B 2>/dev/null");
    if (!glx_out.empty()) {
        if (glx_out.find("Unified memory: yes") != std::string::npos) g_gpu_is_uma = true;
        size_t vm_pos = glx_out.find("Video memory:");
        if (vm_pos != std::string::npos) {
            unsigned long long vm_mb = 0;
            if (sscanf(glx_out.c_str() + vm_pos + 13, "%lluMB", &vm_mb) == 1) {
                if (g_gpu_is_uma) g_gpu_shared_total = vm_mb * 1024ULL * 1024ULL;
                else { g_gpu_dedicated_total = vm_mb * 1024ULL * 1024ULL; g_gpu_has_dedicated = true; }
            }
        }
    }

    // 2. Dedicated VRAM fisik via sysfs DRM
    for (int c = 0; c < 4; c++) {
        std::string card_path = "/sys/class/drm/card" + std::to_string(c);
        std::string vram_tot = trim(readFile(card_path + "/device/mem_info_vram_total"));
        if (vram_tot.empty()) vram_tot = trim(readFile(card_path + "/lmem_total_bytes"));
        if (!vram_tot.empty()) {
            unsigned long long val = std::strtoull(vram_tot.c_str(), nullptr, 10);
            if (val > 0) { g_gpu_dedicated_total = val; g_gpu_has_dedicated = true; g_gpu_is_uma = false; }
        }
    }

    // 3. NVIDIA dGPU/eGPU - HANYA SEKALI
    if (!g_gpu_has_dedicated) {
        std::string nv_out = execCmd("nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits 2>/dev/null");
        if (!nv_out.empty()) {
            unsigned long long tot_mb = std::strtoull(nv_out.c_str(), nullptr, 10);
            if (tot_mb > 0) { g_gpu_dedicated_total = tot_mb * 1024ULL * 1024ULL; g_gpu_has_dedicated = true; g_gpu_is_uma = false; }
        }
    }

    // 4. Shared VRAM Total (GTT sysfs / TTM)
    if (g_gpu_shared_total == 0) {
        for (int c = 0; c < 4; c++) {
            std::string gtt_tot = trim(readFile("/sys/class/drm/card" + std::to_string(c) + "/device/mem_info_gtt_total"));
            if (!gtt_tot.empty()) {
                unsigned long long val = std::strtoull(gtt_tot.c_str(), nullptr, 10);
                if (val > 0) g_gpu_shared_total += val;
            }
        }
        if (g_gpu_shared_total == 0) {
            std::string ttm_pages = trim(readFile("/sys/module/ttm/parameters/pages_limit"));
            if (!ttm_pages.empty()) {
                unsigned long long pages = std::strtoull(ttm_pages.c_str(), nullptr, 10);
                long page_size = sysconf(_SC_PAGESIZE);
                if (pages > 0 && page_size > 0) g_gpu_shared_total = pages * (unsigned long long)page_size;
            }
        }
    }
}

// AFTER — sisipkan fungsi probeGpuAdapters() SEBELUM baris getRealGpuMemory()
static bool g_gpu_adapters_probed = false;

static void probeGpuAdapters() {
    if (g_gpu_adapters_probed) return;
    g_gpu_adapters_probed = true;
    probeGpuStaticInfoOnce();

    std::vector<GpuAdapterInfo> list;
    std::string lspci_out = execCmd(
        "lspci -nn 2>/dev/null | grep -Ei "
        "'VGA compatible controller|3D controller|Display controller'");

    std::vector<unsigned long long> nvidia_vram_mb;
    {
        std::string nv = execCmd("nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits 2>/dev/null");
        std::istringstream nvs(nv);
        std::string line;
        while (std::getline(nvs, line)) {
            unsigned long long v = std::strtoull(trim(line).c_str(), nullptr, 10);
            if (v > 0) nvidia_vram_mb.push_back(v);
        }
    }
    size_t nvidia_idx = 0;

    std::istringstream iss(lspci_out);
    std::string line;
    int idx = 0;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        size_t colon = line.find(": ");
        if (colon == std::string::npos) continue;

        std::string pci_bus = trim(line.substr(0, line.find(' ')));
        std::string name = line.substr(colon + 2);
        size_t rev = name.find(" (rev");
        if (rev != std::string::npos) name = name.substr(0, rev);
        size_t br = name.rfind(" [");
        if (br != std::string::npos && name.find(']', br) == name.size() - 1) name = name.substr(0, br);
        name = trim(name);
        if (name.empty()) name = "Unknown GPU";

        std::string name_upper = name;
        for (auto& c : name_upper) c = (char)toupper((unsigned char)c);
        bool is_intel = name_upper.find("INTEL") != std::string::npos;

        GpuAdapterInfo info;
        info.id = "gpu" + std::to_string(idx);
        info.name = name;
        info.usage_pct = 0.0;
        info.dedicated_vram_gb = 0.0;
        info.shared_vram_gb = 0.0;

        if (name_upper.find("NVIDIA") != std::string::npos && nvidia_idx < nvidia_vram_mb.size()) {
            info.dedicated_vram_gb = (double)nvidia_vram_mb[nvidia_idx] / 1024.0;
            nvidia_idx++;
        } else if (!is_intel) {
            for (int c = 0; c < 8; c++) {
                std::string uevent = readFile("/sys/class/drm/card" + std::to_string(c) + "/device/uevent");
                if (uevent.find(pci_bus) == std::string::npos) continue;
                std::string vram_tot = trim(readFile("/sys/class/drm/card" + std::to_string(c) + "/device/mem_info_vram_total"));
                if (vram_tot.empty()) vram_tot = trim(readFile("/sys/class/drm/card" + std::to_string(c) + "/lmem_total_bytes"));
                if (!vram_tot.empty()) {
                    unsigned long long val = std::strtoull(vram_tot.c_str(), nullptr, 10);
                    if (val > 0) info.dedicated_vram_gb = (double)val / (1024.0 * 1024.0 * 1024.0);
                }
                break;
            }
        }

        if (is_intel) {
            info.shared_vram_gb = (double)g_gpu_shared_total / (1024.0 * 1024.0 * 1024.0);
        }
        info.is_egpu = (!is_intel && info.dedicated_vram_gb >= 0.5);

        list.push_back(info);
        idx++;
    }

    if (list.empty()) {
        GpuAdapterInfo def;
        def.id = "gpu0";
        def.name = g_gpu_model.empty() ? "N/A (GPU tidak terdeteksi)" : g_gpu_model;
        def.is_egpu = false;
        def.dedicated_vram_gb = 0.0;
        def.shared_vram_gb = (double)g_gpu_shared_total / (1024.0 * 1024.0 * 1024.0);
        def.usage_pct = 0.0;
        list.push_back(def);
    }

    std::sort(list.begin(), list.end(), [](const GpuAdapterInfo& a, const GpuAdapterInfo& b) {
        return a.dedicated_vram_gb > b.dedicated_vram_gb;
    });

    g_gpu_adapters = list;
}

static RealGpuMem getRealGpuMemory() {
    probeGpuStaticInfoOnce(); // execCmd cuma jalan di panggilan PERTAMA

    RealGpuMem mem;
    mem.is_uma          = g_gpu_is_uma;
    mem.has_dedicated   = g_gpu_has_dedicated;
    mem.dedicated_total = g_gpu_dedicated_total;
    mem.shared_total    = g_gpu_shared_total;

    // Dedicated VRAM Used - murah, aman dibaca tiap tick
    for (int c = 0; c < 4; c++) {
        std::string vram_used = trim(readFile("/sys/class/drm/card" + std::to_string(c) + "/device/mem_info_vram_used"));
        if (!vram_used.empty()) mem.dedicated_used += std::strtoull(vram_used.c_str(), nullptr, 10);
    }

    // 5. Cek Penggunaan VRAM Realtime dari /proc/*/fdinfo/* (dengan Deduplikasi Client ID)
    std::map<std::string, unsigned long long> client_shared_bytes;
    std::map<std::string, unsigned long long> client_dedicated_bytes;

    DIR* proc_d = opendir("/proc");
    if (proc_d) {
        struct dirent* pe;
        while ((pe = readdir(proc_d)) != NULL) {
            if (!isdigit(pe->d_name[0])) continue;
            std::string pid_str = pe->d_name;
            std::string fd_dir = std::string("/proc/") + pid_str + "/fdinfo";
            DIR* fdd = opendir(fd_dir.c_str());
            if (!fdd) continue;
            struct dirent* fe;
            while ((fe = readdir(fdd)) != NULL) {
                if (fe->d_name[0] == '.') continue;
                std::string content = readFile(fd_dir + "/" + fe->d_name);
                if (content.find("drm-driver:") == std::string::npos) continue;

                std::string client_id = "";
                unsigned long long fd_shared = 0;
                unsigned long long fd_dedicated = 0;

                std::istringstream fss(content);
                std::string line;
                while (std::getline(fss, line)) {
                    if (line.rfind("drm-client-id:", 0) == 0) {
                        client_id = trim(line.substr(14));
                    }
                    else if (line.rfind("drm-total-system", 0) == 0 ||
                        line.rfind("drm-shmem-memory", 0) == 0 ||
                        line.rfind("drm-resident-system", 0) == 0) {

                        size_t colon = line.find(':');
                    if (colon != std::string::npos) {
                        unsigned long long val = 0;
                        char unit[16] = {0};
                        if (sscanf(line.c_str() + colon + 1, "%llu %15s", &val, unit) >= 1) {
                            unsigned long long bytes = val;
                            if (strcasecmp(unit, "KiB") == 0 || strcasecmp(unit, "KB") == 0) bytes *= 1024ULL;
                            else if (strcasecmp(unit, "MiB") == 0 || strcasecmp(unit, "MB") == 0) bytes *= 1024ULL * 1024ULL;
                            else if (strcasecmp(unit, "GiB") == 0 || strcasecmp(unit, "GB") == 0) bytes *= 1024ULL * 1024ULL * 1024ULL;
                            if (bytes > fd_shared) fd_shared = bytes;
                        }
                    }
                        } else if (line.rfind("drm-total-local", 0) == 0 || line.rfind("drm-resident-local", 0) == 0) {
                            size_t colon = line.find(':');
                            if (colon != std::string::npos) {
                                unsigned long long val = 0;
                                char unit[16] = {0};
                                if (sscanf(line.c_str() + colon + 1, "%llu %15s", &val, unit) >= 1) {
                                    unsigned long long bytes = val;
                                    if (strcasecmp(unit, "KiB") == 0 || strcasecmp(unit, "KB") == 0) bytes *= 1024ULL;
                                    else if (strcasecmp(unit, "MiB") == 0 || strcasecmp(unit, "MB") == 0) bytes *= 1024ULL * 1024ULL;
                                    else if (strcasecmp(unit, "GiB") == 0 || strcasecmp(unit, "GB") == 0) bytes *= 1024ULL * 1024ULL * 1024ULL;
                                    if (bytes > fd_dedicated) fd_dedicated = bytes;
                                }
                            }
                        }
                }

                std::string key = client_id.empty() ? (pid_str + "_" + fe->d_name) : client_id;
                if (fd_shared > client_shared_bytes[key]) client_shared_bytes[key] = fd_shared;
                if (fd_dedicated > client_dedicated_bytes[key]) client_dedicated_bytes[key] = fd_dedicated;
            }
            closedir(fdd);
        }
        closedir(proc_d);
    }

    for (const auto& kv : client_shared_bytes) mem.shared_used += kv.second;
    for (const auto& kv : client_dedicated_bytes) {
        if (mem.has_dedicated) mem.dedicated_used += kv.second;
    }
    return mem;
}

static void updateTelemetry() {
    double now = getTimeSec();

    // 1. CPU Usage
    std::map<std::string, double> cpu_usages;
    std::ifstream stat_file("/proc/stat");
    if (stat_file.is_open()) {
        std::string line;
        while (std::getline(stat_file, line)) {
            if (line.rfind("cpu", 0) == 0) {
                std::istringstream iss(line);
                std::string name;
                unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
                iss >> name >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
                unsigned long long cur_idle = idle + iowait;
                unsigned long long cur_total = user + nice + system + idle + iowait + irq + softirq + steal;

                if (g_last_cpu_ticks.find(name) != g_last_cpu_ticks.end()) {
                    auto last = g_last_cpu_ticks[name];
                    unsigned long long total_diff = cur_total - last.total;
                    unsigned long long idle_diff = cur_idle - last.idle;
                    if (total_diff > 0) {
                        double u = 100.0 * (1.0 - (double)idle_diff / (double)total_diff);
                        cpu_usages[name] = std::round(u * 10.0) / 10.0;
                    } else {
                        cpu_usages[name] = 0.0;
                    }
                } else {
                    cpu_usages[name] = 0.0;
                }
                g_last_cpu_ticks[name] = {cur_idle, cur_total};
            }
        }
    }

    // 2. CPU Frequencies
    std::vector<int> cpu_freqs;
    for (int i = 0; i < 16; i++) {
        std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_cur_freq";
        std::string val = readFile(path);
        if (!val.empty()) {
            cpu_freqs.push_back(std::atoi(val.c_str()) / 1000);
        }
    }

    // 3. Memory & Swaps
    unsigned long long mem_total = 0, mem_free = 0, mem_avail = 0;
    std::ifstream mem_file("/proc/meminfo");
    if (mem_file.is_open()) {
        std::string line;
        while (std::getline(mem_file, line)) {
            unsigned long long val = 0;
            if (sscanf(line.c_str(), "MemTotal: %llu kB", &val) == 1) mem_total = val * 1024;
            else if (sscanf(line.c_str(), "MemFree: %llu kB", &val) == 1) mem_free = val * 1024;
            else if (sscanf(line.c_str(), "MemAvailable: %llu kB", &val) == 1) mem_avail = val * 1024;
        }
    }
    if (mem_avail == 0) mem_avail = mem_free;
    unsigned long long mem_used = (mem_total > mem_avail) ? (mem_total - mem_avail) : 0;

    unsigned long long zram_total = 0, zram_used = 0;
    unsigned long long disk_swap_total = 0, disk_swap_used = 0;
    std::ifstream swap_file("/proc/swaps");
    if (swap_file.is_open()) {
        std::string line;
        std::getline(swap_file, line);
        while (std::getline(swap_file, line)) {
            char dev[256];
            unsigned long long total_k = 0, used_k = 0;
            if (sscanf(line.c_str(), "%255s %*s %llu %llu", dev, &total_k, &used_k) >= 3) {
                if (strstr(dev, "zram")) {
                    zram_total += total_k * 1024;
                    zram_used += used_k * 1024;
                } else {
                    disk_swap_total += total_k * 1024;
                    disk_swap_used += used_k * 1024;
                }
            }
        }
    }

    // 4. Real-Time GPU Utilization & Frequencies via DRM RC6 Residency
    int gpu_freq = 0;
    std::string g_freq_str = readFile("/sys/class/drm/card0/gt_cur_freq_mhz");
    if (!g_freq_str.empty()) gpu_freq = std::atoi(g_freq_str.c_str());

    static unsigned long long s_last_rc6 = 0;
    static double s_last_gpu_time = 0.0;
    double gpu_usage_rcs = 0.0;

    std::string rc6_str = readFile("/sys/class/drm/card0/power/rc6_residency_ms");
    if (rc6_str.empty()) rc6_str = readFile("/sys/class/drm/card0/gt/gt0/rc6_residency_ms");
    if (!rc6_str.empty()) {
        unsigned long long cur_rc6 = std::strtoull(rc6_str.c_str(), NULL, 10);
        if (s_last_gpu_time > 0.0 && cur_rc6 >= s_last_rc6) {
            double dt_ms = (now - s_last_gpu_time) * 1000.0;
            double d_rc6 = (double)(cur_rc6 - s_last_rc6);
            if (dt_ms > 0.0) {
                double idle_pct = (d_rc6 / dt_ms) * 100.0;
                gpu_usage_rcs = std::max(0.0, std::min(100.0, 100.0 - idle_pct));
                gpu_usage_rcs = std::round(gpu_usage_rcs * 10.0) / 10.0;
            }
        }
        s_last_rc6 = cur_rc6;
        s_last_gpu_time = now;
    }

    // 5. Network Stats
    unsigned long long total_rx = 0, total_tx = 0;
    std::ifstream net_file("/proc/net/dev");
    if (net_file.is_open()) {
        std::string line;
        std::getline(net_file, line);
        std::getline(net_file, line);
        while (std::getline(net_file, line)) {
            size_t col = line.find(':');
            if (col != std::string::npos) {
                std::string ifname = trim(line.substr(0, col));
                if (ifname != "lo") {
                    std::istringstream iss(line.substr(col + 1));
                    unsigned long long rx_b = 0, tx_b = 0, d1, d2, d3, d4, d5, d6, d7;
                    iss >> rx_b >> d1 >> d2 >> d3 >> d4 >> d5 >> d6 >> d7 >> tx_b;
                    total_rx += rx_b;
                    total_tx += tx_b;
                }
            }
        }
    }

    if (g_last_net_time > 0.0) {
        double dt = now - g_last_net_time;
        if (dt > 0.0 && g_last_net_rx > 0) {
            g_cur_rx_rate = (total_rx >= g_last_net_rx) ? ((total_rx - g_last_net_rx) / dt) : 0.0;
            g_cur_tx_rate = (total_tx >= g_last_net_tx) ? ((total_tx - g_last_net_tx) / dt) : 0.0;
        }
    }
    g_last_net_rx = total_rx;
    g_last_net_tx = total_tx;
    g_last_net_time = now;

    // 6. Comprehensive Disks Detection & Telemetry
    unsigned long long total_read_sec = 0, total_write_sec = 0;
    std::map<std::string, std::pair<unsigned long long, unsigned long long>> cur_proc_diskstats;

    std::ifstream disk_file("/proc/diskstats");
    if (disk_file.is_open()) {
        std::string line;
        while (std::getline(disk_file, line)) {
            char dname[64];
            unsigned long long rd_sec = 0, wr_sec = 0;
            if (sscanf(line.c_str(), "%*d %*d %63s %*d %*d %llu %*d %*d %*d %llu", dname, &rd_sec, &wr_sec) >= 3) {
                std::string name(dname);
                if (name.rfind("loop", 0) == 0 || name.rfind("zram", 0) == 0) continue;
                cur_proc_diskstats[name] = {rd_sec, wr_sec};
                if (isRootDiskDevice(name)) {
                    total_read_sec += rd_sec;
                    total_write_sec += wr_sec;
                }
            }
        }
    }

    double dt_disk = (g_last_disk_time > 0.0) ? (now - g_last_disk_time) : 0.5;
    if (dt_disk <= 0.0) dt_disk = 0.5;

    if (g_last_disk_time > 0.0 && g_last_disk_read > 0) {
        g_cur_disk_read_rate = (total_read_sec >= g_last_disk_read) ? ((total_read_sec - g_last_disk_read) * 512.0 / dt_disk) : 0.0;
        g_cur_disk_write_rate = (total_write_sec >= g_last_disk_write) ? ((total_write_sec - g_last_disk_write) * 512.0 / dt_disk) : 0.0;
    }
    g_last_disk_read = total_read_sec;
    g_last_disk_write = total_write_sec;
    g_last_disk_time = now;

    // Discover Block Devices
    std::stringstream disks_array;
    disks_array << "[";
    bool first_disk = true;
    std::vector<std::string> ssd_models_list;
    std::vector<std::string> detected_disk_devs;
    std::map<std::string, double> dev_activity_now;

    DIR* sys_block = opendir("/sys/block");
    if (sys_block) {
        struct dirent* be;
        while ((be = readdir(sys_block)) != NULL) {
            std::string dname = be->d_name;
            if (dname == "." || dname == ".." || dname.rfind("loop", 0) == 0 || dname.rfind("zram", 0) == 0) continue;

            std::string size_raw = readFile(std::string("/sys/block/") + dname + "/size");
            if (size_raw.empty()) continue;
            unsigned long long sectors = std::strtoull(size_raw.c_str(), NULL, 10);
            if (sectors == 0) continue;
            detected_disk_devs.push_back(dname);
            double size_gb = (double)(sectors * 512ULL) / (1024.0 * 1024.0 * 1024.0);

            std::string model = trim(readFile(std::string("/sys/block/") + dname + "/device/model"));
            std::string vendor = trim(readFile(std::string("/sys/block/") + dname + "/device/vendor"));
            std::string full_model = vendor.empty() ? model : (vendor + " " + model);
            if (full_model.empty()) full_model = dname;

            bool is_rotational = false;
            std::string rota = readFile(std::string("/sys/block/") + dname + "/queue/rotational");
            if (!rota.empty() && rota[0] == '1') is_rotational = true;

            bool is_removable = false;
            std::string rm_str = readFile(std::string("/sys/block/") + dname + "/removable");
            if (!rm_str.empty() && rm_str[0] == '1') is_removable = true;

            bool is_root = (dname == g_root_dev);
            bool is_nvme = (dname.rfind("nvme", 0) == 0);
            bool is_usb = (dname.rfind("sd", 0) == 0 && (is_removable || size_gb <= 128.0 || full_model.find("SanDisk") != std::string::npos || full_model.find("Portable") != std::string::npos || full_model.find("ProductCode") != std::string::npos));

            std::string tran = is_nvme ? "nvme" : (is_usb ? "usb" : "sata");

            // Drive Classification & Descriptions (Adaptive for ANY drive)
            std::string icon = "⚡";
            std::string drive_type = "Solid State Drive";
            bool is_ssd_flag = false;
            bool is_hdd_flag = false;

            SmartDriveInfo sinfo = probeDriveSmart(dname);
            std::string tbw_str = sinfo.tbw_str;
            std::string remaining_str = sinfo.remaining_str;
            std::string temp_str = sinfo.temp_str;
            std::string health_str = sinfo.health_str;

            if (is_nvme) {
                icon = "⚡";
                drive_type = "PCIe NVMe Solid State Drive";
                is_ssd_flag = true;
                is_hdd_flag = false;
            } else if (is_removable || (is_usb && (full_model.find("ProductCode") != std::string::npos || size_gb <= 64.0))) {
                icon = "💾";
                drive_type = "USB Flash Drive (Removable Flash)";
                tbw_str = "NAND Flash (USB Mass Storage)";
                remaining_str = "Wear-Leveling Flash Cells";
                // Suhu asli dari hwmon kalau ada sensornya; kebanyakan flashdisk memang tidak
                // punya sensor suhu sama sekali, jadi N/A di sini valid, bukan bug.
                temp_str = "N/A";
                DIR* fd_hw = opendir("/sys/class/hwmon");
                if (fd_hw) {
                    struct dirent* fhe;
                    while ((fhe = readdir(fd_hw)) != NULL) {
                        if (fhe->d_name[0] == '.') continue;
                        std::string hpath = std::string("/sys/class/hwmon/") + fhe->d_name;
                        std::string hname = trim(readFile(hpath + "/name"));
                        if (hname.find(dname) != std::string::npos || hname.find("drivetemp") != std::string::npos) {
                            std::string t1 = trim(readFile(hpath + "/temp1_input"));
                            if (!t1.empty()) {
                                int v = std::atoi(t1.c_str());
                                if (v > 1000) v /= 1000;
                                if (v > 0 && v < 120) temp_str = std::to_string(v) + " °C";
                            }
                        }
                    }
                    closedir(fd_hw);
                }
                health_str = "Plug & Play / Operational (Healthy)";
                is_ssd_flag = false;
                is_hdd_flag = false;
            } else if (is_rotational && !is_usb) {
                icon = "💽";
                drive_type = "HDD (Mechanical Hard Drive)";
                tbw_str = "Rotational Platter (5400/7200 RPM)";
                remaining_str = "N/A";
                is_hdd_flag = true;
                is_ssd_flag = false;
            } else {
                // External SSD or SATA SSD
                icon = "⚡";
                drive_type = is_usb ? "Portable External SSD (USB/UASP)" : "SATA Solid State Drive (SSD)";
                is_ssd_flag = true;
                is_hdd_flag = false;
                // Tidak ada override lagi di sini — tbw_str/remaining_str/temp_str/health_str
                // sudah diisi dari probeDriveSmart() di atas (SMART asli, atau fallback
                // kernel /sys/block yang jujur kalau SMART tidak tersedia).
            }

            char sz_buf[32];
            snprintf(sz_buf, sizeof(sz_buf), "%.1f GB", size_gb);
            ssd_models_list.push_back(dname + " (" + std::string(sz_buf) + "): " + full_model);

            // Compute Partition Used and Free Space via /proc/mounts + statvfs
            std::string usage_str = "Raw Block Storage";
            std::string partitions_str = "No active filesystem mount";
            unsigned long long total_fs_bytes = 0, free_fs_bytes = 0;
            std::vector<std::string> mnt_list;
            std::ifstream mounts_f("/proc/mounts");
            if (mounts_f.is_open()) {
                std::string mline;
                while (std::getline(mounts_f, mline)) {
                    if (mline.rfind("/dev/" + dname, 0) == 0) {
                        std::istringstream miss(mline);
                        std::string mdev, mmnt, mfstype;
                        if (miss >> mdev >> mmnt >> mfstype) {
                            struct statvfs sv;
                            if (statvfs(mmnt.c_str(), &sv) == 0) {
                                unsigned long long tot = sv.f_blocks * sv.f_frsize;
                                unsigned long long fre = sv.f_bavail * sv.f_frsize;
                                total_fs_bytes += tot;
                                free_fs_bytes += fre;
                                std::string short_mnt = (mmnt == "/") ? "/" : mmnt.substr(mmnt.find_last_of('/') + 1);
                                mnt_list.push_back(mdev.substr(5) + " (" + short_mnt + ": " + mfstype + ")");
                            }
                        }
                    }
                }
            }

            if (total_fs_bytes > 0) {
                unsigned long long used_fs = (total_fs_bytes >= free_fs_bytes) ? (total_fs_bytes - free_fs_bytes) : 0;
                double used_gb = (double)used_fs / (1024.0 * 1024.0 * 1024.0);
                double free_gb = (double)free_fs_bytes / (1024.0 * 1024.0 * 1024.0);
                double pct = ((double)used_fs / total_fs_bytes) * 100.0;
                char u_buf[128];
                snprintf(u_buf, sizeof(u_buf), "%.1f GB Used (%.0f%%) | %.1f GB Free", used_gb, pct, free_gb);
                usage_str = u_buf;
                if (!mnt_list.empty()) {
                    partitions_str = "";
                    for (size_t mi = 0; mi < mnt_list.size(); mi++) {
                        if (mi > 0) partitions_str += ", ";
                        partitions_str += mnt_list[mi];
                    }
                }
            } else {
                usage_str = std::string(sz_buf) + " Unallocated / Flash Storage";
            }

            double dev_r_rate = 0.0, dev_w_rate = 0.0;
            if (g_last_disk_time > 0.0 && g_last_dev_diskstats.find(dname) != g_last_dev_diskstats.end()) {
                auto prev_st = g_last_dev_diskstats[dname];
                auto cur_st = cur_proc_diskstats[dname];
                if (cur_st.first >= prev_st.first) dev_r_rate = (cur_st.first - prev_st.first) * 512.0 / dt_disk;
                if (cur_st.second >= prev_st.second) dev_w_rate = (cur_st.second - prev_st.second) * 512.0 / dt_disk;
            }

            dev_activity_now[dname] = dev_r_rate + dev_w_rate;

            std::string detail_text = "";
            {
                std::lock_guard<std::mutex> lock(g_stats_mutex);
                if (g_disk_details_map.find(dname) != g_disk_details_map.end()) {
                    detail_text = g_disk_details_map[dname];
                }
            }
            if (detail_text.empty()) {
                detail_text = "=== Block Device Telemetry: /dev/" + dname + " ===\nModel: " + full_model + "\nCapacity: " + sz_buf + "\nUsage: " + usage_str + "\nPartitions: " + partitions_str + "\nStatus: " + health_str + "\nTotal Written: " + tbw_str + "\nRemaining: " + remaining_str;
            }

            if (!first_disk) disks_array << ",";
            first_disk = false;

            disks_array << "{";
            disks_array << "\"id\":\"" << dname << "\",";
            disks_array << "\"dev\":\"" << dname << "\",";
            disks_array << "\"name\":\"" << escapeJson(full_model + " (" + sz_buf + ")") << "\",";
            disks_array << "\"model\":\"" << escapeJson(full_model) << "\",";
            disks_array << "\"size\":\"" << sz_buf << "\",";
            disks_array << "\"capacity_str\":\"" << sz_buf << " Total Capacity\",";
            disks_array << "\"usage_str\":\"" << escapeJson(usage_str) << "\",";
            disks_array << "\"partitions_str\":\"" << escapeJson(partitions_str) << "\",";
            disks_array << "\"icon\":\"" << icon << "\",";
            disks_array << "\"label\":\"" << icon << " " << escapeJson(full_model) << " (" << sz_buf << ")\",";
            disks_array << "\"type\":\"" << escapeJson(drive_type) << "\",";
            disks_array << "\"transport\":\"" << tran << "\",";
            disks_array << "\"is_root\":" << (is_root ? "true" : "false") << ",";
            disks_array << "\"is_rizky\":" << (is_root ? "true" : "false") << ",";
            disks_array << "\"is_ssd\":" << (is_ssd_flag ? "true" : "false") << ",";
            disks_array << "\"is_hdd\":" << (is_hdd_flag ? "true" : "false") << ",";
            disks_array << "\"is_flash\":" << (is_removable ? "true" : "false") << ",";
            disks_array << "\"read_rate\":" << dev_r_rate << ",";
            disks_array << "\"write_rate\":" << dev_w_rate << ",";
            disks_array << "\"tbw_str\":\"" << escapeJson(tbw_str) << "\",";
            disks_array << "\"remaining_str\":\"" << escapeJson(remaining_str) << "\",";
            disks_array << "\"temp_str\":\"" << escapeJson(temp_str) << "\",";
            disks_array << "\"health_str\":\"" << escapeJson(health_str) << "\",";
            disks_array << "\"detail_text\":\"" << escapeJson(detail_text) << "\"";
            disks_array << "}";
        }
        closedir(sys_block);
    }
    g_last_dev_diskstats = cur_proc_diskstats;
    disks_array << "]";

    // 7. Real-Time Thermal — CPU package asli, prioritas: hwmon coretemp > thermal_zone x86_pkg_temp > N/A
    int temp_c = -999; // -999 = sensor tidak tersedia, konsisten sama bat_temp_c (baris 1561)

    // Prioritas 1: hwmon "coretemp" (paling otoritatif, terbukti akurat dari data real kamu)
    DIR* cpu_hw_dir = opendir("/sys/class/hwmon");
    if (cpu_hw_dir) {
        struct dirent* he;
        while ((he = readdir(cpu_hw_dir)) != NULL) {
            if (he->d_name[0] == '.') continue;
            std::string hpath = std::string("/sys/class/hwmon/") + he->d_name;
            std::string hname = trim(readFile(hpath + "/name"));
            if (hname != "coretemp" && hname != "k10temp" && hname != "zenpower") continue;

            int package_temp = -1, core_max = -1;
            for (int i = 1; i <= 16; i++) {
                std::string label = trim(readFile(hpath + "/temp" + std::to_string(i) + "_label"));
                std::string raw = trim(readFile(hpath + "/temp" + std::to_string(i) + "_input"));
                if (raw.empty()) continue;
                int v = std::atoi(raw.c_str());
                if (v > 1000) v /= 1000;
                if (v < 5 || v > 115) continue;

                if (label.find("Package") != std::string::npos || label.find("Tctl") != std::string::npos || label.find("Tdie") != std::string::npos) {
                    package_temp = v;
                } else if (label.find("Core") != std::string::npos || label.empty()) {
                    if (v > core_max) core_max = v;
                }
            }
            temp_c = (package_temp > 0) ? package_temp : core_max;
            if (temp_c > 0) break;
        }
        closedir(cpu_hw_dir);
    }

    // Prioritas 2 (fallback): cari zona thermal SPESIFIK bernama "x86_pkg_temp" — bukan blind-max semua zona
    if (temp_c <= 0) {
        DIR* th_dir = opendir("/sys/class/thermal");
        if (th_dir) {
            struct dirent* te;
            while ((te = readdir(th_dir)) != NULL) {
                if (strncmp(te->d_name, "thermal_zone", 12) != 0) continue;
                std::string zpath = std::string("/sys/class/thermal/") + te->d_name;
                std::string ztype = trim(readFile(zpath + "/type"));
                if (ztype != "x86_pkg_temp") continue;

                int v = std::atoi(trim(readFile(zpath + "/temp")).c_str());
                if (v > 1000) v /= 1000;
                if (v >= 5 && v <= 115) temp_c = v;
                break;
            }
            closedir(th_dir);
        }
    }

    // 7b. Real-Time Battery Detail (Selengkap Windows: Model, Health, Voltage, Rate, Cycle, Temp, Runtime)
    bool bat_present = false;
    int battery_pct = -1;                 // -1 = tidak ada baterai fisik / gagal dibaca
    std::string bat_status_raw = "Unknown";
    std::string bat_charge_state = "Desktop AC Power";
    std::string bat_model = "";
    std::string bat_manufacturer = "";
    std::string bat_chemistry = "";
    int bat_health_pct = -1;
    int bat_temp_c = -999;                // -999 = sensor suhu tidak tersedia di driver ini
    long bat_capacity_now_mwh = -1;
    long bat_capacity_full_mwh = -1;
    long bat_design_capacity_mwh = -1;
    int bat_voltage_mv = -1;
    int bat_rate_mw = 0;
    long bat_estimated_runtime_sec = -1;
    int bat_cycle_count = -1;

    DIR* bat_dir = opendir("/sys/class/power_supply");
    if (bat_dir) {
        struct dirent* be;
        while ((be = readdir(bat_dir)) != NULL) {
            if (strncmp(be->d_name, "BAT", 3) == 0) {
                std::string base = std::string("/sys/class/power_supply/") + be->d_name + "/";
                bat_present = true;

                std::string cap_str = readFile(base + "capacity");
                if (!cap_str.empty()) battery_pct = std::atoi(cap_str.c_str());

                bat_status_raw = trim(readFile(base + "status"));
                if (bat_status_raw.empty()) bat_status_raw = "Unknown";

                bat_model = trim(readFile(base + "model_name"));
                bat_manufacturer = trim(readFile(base + "manufacturer"));

                std::string tech = trim(readFile(base + "technology"));
                std::string upperTech = tech;
                for (auto& c : upperTech) c = toupper(c);
                if (upperTech == "LI-ION" || upperTech == "LION") bat_chemistry = "Lithium-Ion (Li-ion)";
                else if (upperTech == "LI-POLY" || upperTech == "LIPO") bat_chemistry = "Lithium-Polymer (Li-Po)";
                else if (upperTech == "NIMH") bat_chemistry = "Nickel-Metal Hydride (NiMH)";
                else if (upperTech == "NICD") bat_chemistry = "Nickel-Cadmium (NiCd)";
                else if (upperTech == "LIFE") bat_chemistry = "Lithium Iron Phosphate (LiFePO4)";
                else if (!tech.empty()) bat_chemistry = tech;

                // Cycle Count
                std::string cyc_str = trim(readFile(base + "cycle_count"));
                if (!cyc_str.empty()) {
                    int cyc = std::atoi(cyc_str.c_str());
                    if (cyc > 0) bat_cycle_count = cyc;
                }

                // Voltage (µV -> mV)
                std::string volt_str = trim(readFile(base + "voltage_now"));
                if (!volt_str.empty()) {
                    long uv = std::atol(volt_str.c_str());
                    if (uv > 0) bat_voltage_mv = (int)(uv / 1000);
                }

                // Kapasitas: prioritaskan energy_* (µWh, langsung dlm satuan energi)
                std::string en_now = trim(readFile(base + "energy_now"));
                std::string en_full = trim(readFile(base + "energy_full"));
                std::string en_design = trim(readFile(base + "energy_full_design"));
                if (!en_now.empty()) bat_capacity_now_mwh = std::atol(en_now.c_str()) / 1000;
                if (!en_full.empty()) bat_capacity_full_mwh = std::atol(en_full.c_str()) / 1000;
                if (!en_design.empty()) bat_design_capacity_mwh = std::atol(en_design.c_str()) / 1000;

                // Fallback: driver charge-based (µAh) -> dikonversi ke mWh pakai voltage
                if (bat_capacity_full_mwh <= 0 && bat_voltage_mv > 0) {
                    std::string ch_now = trim(readFile(base + "charge_now"));
                    std::string ch_full = trim(readFile(base + "charge_full"));
                    std::string ch_design = trim(readFile(base + "charge_full_design"));
                    double vBat = bat_voltage_mv / 1000.0;
                    if (!ch_now.empty()) bat_capacity_now_mwh = (long)((std::atol(ch_now.c_str()) / 1000.0) * vBat);
                    if (!ch_full.empty()) bat_capacity_full_mwh = (long)((std::atol(ch_full.c_str()) / 1000.0) * vBat);
                    if (!ch_design.empty()) bat_design_capacity_mwh = (long)((std::atol(ch_design.c_str()) / 1000.0) * vBat);
                }

                if (bat_design_capacity_mwh > 0 && bat_capacity_full_mwh > 0) {
                    bat_health_pct = (int)(((double)bat_capacity_full_mwh / (double)bat_design_capacity_mwh) * 100.0);
                    if (bat_health_pct > 100) bat_health_pct = 100;
                }

                // Rate (daya charge/discharge saat ini)
                std::string pw_now = trim(readFile(base + "power_now"));
                if (!pw_now.empty()) {
                    bat_rate_mw = (int)(std::atol(pw_now.c_str()) / 1000);
                } else {
                    std::string cur_now = trim(readFile(base + "current_now"));
                    if (!cur_now.empty() && bat_voltage_mv > 0) {
                        double uA = std::atof(cur_now.c_str());
                        bat_rate_mw = (int)((uA / 1000.0) * (bat_voltage_mv / 1000.0));
                    }
                }
                if (bat_status_raw == "Discharging" && bat_rate_mw > 0) bat_rate_mw = -bat_rate_mw;

                // Estimasi Runtime: prioritas nilai langsung dari driver, fallback hitung manual
                std::string tte = trim(readFile(base + "time_to_empty_now"));
                std::string ttf = trim(readFile(base + "time_to_full_now"));
                if (bat_status_raw == "Discharging" && !tte.empty() && std::atol(tte.c_str()) > 0) {
                    bat_estimated_runtime_sec = std::atol(tte.c_str()) * 60; // driver report dlm menit
                } else if (bat_status_raw == "Charging" && !ttf.empty() && std::atol(ttf.c_str()) > 0) {
                    bat_estimated_runtime_sec = std::atol(ttf.c_str()) * 60;
                } else if (bat_rate_mw != 0 && bat_capacity_now_mwh > 0 && bat_capacity_full_mwh > 0) {
                    double wattsAbs = std::abs(bat_rate_mw) / 1000.0;
                    if (wattsAbs > 0.05) {
                        long targetMwh = (bat_status_raw == "Charging") ? (bat_capacity_full_mwh - bat_capacity_now_mwh) : bat_capacity_now_mwh;
                        if (targetMwh > 0) bat_estimated_runtime_sec = (long)((targetMwh / 1000.0) / wattsAbs * 3600.0);
                    }
                }

                // Suhu Baterai (tidak semua driver ACPI expose file ini)
                std::string t_str = trim(readFile(base + "temp"));
                if (!t_str.empty()) {
                    double raw = std::atof(t_str.c_str());
                    double celsius = raw / 10.0; // sysfs 'temp' dalam persepuluh derajat Celsius
                    if (celsius > -30.0 && celsius < 110.0) bat_temp_c = (int)(celsius + 0.5);
                }

                bool is_full = (battery_pct >= 99) ||
                               (bat_capacity_full_mwh > 0 && bat_capacity_now_mwh >= bat_capacity_full_mwh);
                if (bat_status_raw == "Charging") bat_charge_state = is_full ? "Fully Charged (Plugged In)" : "Charging";
                else if (bat_status_raw == "Discharging") bat_charge_state = "Discharging (On Battery)";
                else if (bat_status_raw == "Full") bat_charge_state = "Fully Charged (Plugged In)";
                else if (bat_status_raw == "Not charging") bat_charge_state = "Plugged In (Not Charging)";
                else bat_charge_state = is_full ? "Fully Charged" : "Idle";

                break;
            }
        }
        closedir(bat_dir);
    }

    if (!bat_present) {
        g_cache_battery_tech = "<strong>Power Source:</strong> Desktop AC Power (No Battery Detected)<br><strong>Status:</strong> Running on Main Line Power";
    } else {
        std::stringstream ss;
        std::string displayName = bat_model.empty() ? "Internal Battery" : bat_model;
        if (!bat_manufacturer.empty() && displayName.find(bat_manufacturer) == std::string::npos) {
            displayName += " (" + bat_manufacturer + ")";
        }
        ss << "<strong>Model:</strong> " << displayName;
        if (battery_pct >= 0) ss << " (" << battery_pct << "% Charged)";
        ss << "<br>";

        ss << "<strong>Power Status:</strong> " << bat_charge_state;
        if (bat_rate_mw != 0) {
            double w = std::abs(bat_rate_mw) / 1000.0;
            if (w >= 0.1) ss << " [" << (bat_rate_mw > 0 ? "+" : "-") << std::fixed << std::setprecision(1) << w << " W]";
        }
        if (bat_estimated_runtime_sec > 0 && bat_charge_state.find("Discharging") != std::string::npos) {
            int hrs = (int)(bat_estimated_runtime_sec / 3600);
            int mins = (int)((bat_estimated_runtime_sec % 3600) / 60);
            ss << " (" << hrs << "h " << mins << "m remaining)";
        }
        ss << "<br>";

        if (!bat_chemistry.empty()) ss << "<strong>Technology:</strong> " << bat_chemistry;
        if (bat_health_pct >= 0) {
            if (!bat_chemistry.empty()) ss << " | ";
            ss << "<strong>Health:</strong> " << bat_health_pct << "%";
        }
        if (bat_cycle_count > 0) ss << " (" << bat_cycle_count << " Cycles)";

        bool has_cap = (bat_capacity_full_mwh > 0 || bat_design_capacity_mwh > 0);
        bool has_volt = (bat_voltage_mv > 0);
        if (has_cap || has_volt) {
            ss << "<br><strong>Capacity:</strong> ";
            if (bat_capacity_now_mwh > 0 && bat_capacity_full_mwh > 0) {
                ss << std::fixed << std::setprecision(1) << (bat_capacity_now_mwh / 1000.0)
                   << " / " << (bat_capacity_full_mwh / 1000.0) << " Wh";
            } else if (bat_capacity_full_mwh > 0) {
                ss << std::fixed << std::setprecision(1) << (bat_capacity_full_mwh / 1000.0) << " Wh";
            } else if (bat_design_capacity_mwh > 0) {
                ss << std::fixed << std::setprecision(1) << (bat_design_capacity_mwh / 1000.0) << " Wh";
            }
            if (has_volt) {
                ss << " | <strong>Voltage:</strong> " << std::fixed << std::setprecision(2)
                   << (bat_voltage_mv / 1000.0) << " V";
            }
        }

        if (bat_temp_c > -100) ss << " | <strong>Temp:</strong> " << bat_temp_c << " °C";
        else ss << " | <strong>Temp:</strong> N/A (Not Set)";

        g_cache_battery_tech = ss.str();
    }

    std::string wifi_info = execCmd("nmcli -t -f active,ssid,signal,freq,rate,security dev wifi 2>/dev/null | grep '^yes'");
    std::string ip_addr = trim(execCmd("ip -br a 2>/dev/null | grep -E 'UP|wlo1' | awk '{print $1 \": \" $3}'"));

    if (!wifi_info.empty()) {
        // Terhubung ke Wi-Fi -> parse data ASLI, tanpa nilai default karangan.
        std::vector<std::string> parts;
        std::stringstream ss(wifi_info);
        std::string item;
        while (std::getline(ss, item, ':')) parts.push_back(item);
        std::string ssid = (parts.size() >= 2 && !parts[1].empty()) ? parts[1] : "N/A";
        std::string sig  = (parts.size() >= 3 && !parts[2].empty()) ? parts[2] : "N/A";
        std::string freq = (parts.size() >= 4 && !parts[3].empty()) ? parts[3] : "N/A";
        std::string rate = (parts.size() >= 5 && !parts[4].empty()) ? parts[4] : "N/A";

        g_cache_network_type = "<strong>Connection Status:</strong> CONNECTED (Wi-Fi Wireless Network)<br>"
        "<strong>Wi-Fi SSID:</strong> <span style='color:#38bdf8; font-weight:800;'>" + ssid + "</span><br>"
        "<strong>Signal Quality:</strong> " + sig + "% (" + rate + ") | <strong>Frequency:</strong> " + freq + "<br>"
        "<strong>IP Address:</strong> " + ip_addr;
    } else {
        // Tidak terdeteksi Wi-Fi aktif -> laporkan jujur, bukan karang status "CONNECTED".
        g_cache_network_type = "<strong>Connection Status:</strong> No Active Wi-Fi Connection<br>"
        "<strong>IP Address:</strong> " + (ip_addr.empty() ? "N/A" : ip_addr);
    }

    // 8. Real-time Processes Collection (CPU, Memory, Disk I/O, Network, GPU)
    std::vector<ProcEntry> top_cpu_procs, top_mem_procs;
    std::vector<DiskProcEntry> disk_procs;
    std::vector<NetProcEntry> net_procs;
    std::vector<GpuProcEntry> gpu_procs;

    // CPU Process list
    long ncpu_for_proc = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpu_for_proc < 1) ncpu_for_proc = 1;

    std::string ps_cpu = execCmd("ps -eo pcpu,cputime,comm --sort=-%cpu 2>/dev/null | tail -n +2 | head -n 40");
    std::istringstream psc_iss(ps_cpu);
    std::string pline;
    while (std::getline(psc_iss, pline)) {
        std::istringstream l_iss(pline);
        std::string pcpu, ptime, pname;
        if (l_iss >> pcpu >> ptime) {
            std::getline(l_iss, pname);
            pname = trim(pname);
            if (pname.empty()) continue;

            double raw_pcpu = std::atof(pcpu.c_str());
            double norm_pcpu = std::min(100.0, raw_pcpu / (double)ncpu_for_proc);
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", norm_pcpu);
            std::string v = std::string(buf) + "%";
            if (!ptime.empty() && ptime != "00:00:00") v += " (" + ptime + ")";
            top_cpu_procs.push_back({pname.substr(0, 20), v});
        }
    }

    // Memory Process list
    std::string ps_mem = execCmd("ps -eo comm,%mem,rss --sort=-%mem 2>/dev/null | tail -n +2 | head -n 40");
    std::istringstream psm_iss(ps_mem);
    while (std::getline(psm_iss, pline)) {
        std::istringstream l_iss(pline);
        std::string pname, pmem;
        unsigned long long rss_k = 0;
        if (l_iss >> pname >> pmem >> rss_k) {
            std::string sz;
            if (rss_k >= 1024 * 1024) {
                char b[32];
                snprintf(b, sizeof(b), "%.1f GB", (double)rss_k / (1024.0 * 1024.0));
                sz = b;
            } else {
                char b[32];
                snprintf(b, sizeof(b), "%.1f MB", (double)rss_k / 1024.0);
                sz = b;
            }
            top_mem_procs.push_back({pname.substr(0, 20), sz + " (" + pmem + "%)"});
        }
    }

    // Disk I/O per Process via /proc/[pid]/io
    std::map<int, ProcIoTick> cur_proc_io_ticks;
    DIR* proc_dir = opendir("/proc");
    if (proc_dir) {
        struct dirent* pe;
        while ((pe = readdir(proc_dir)) != NULL) {
            if (!isdigit(pe->d_name[0])) continue;
            int pid = std::atoi(pe->d_name);

            std::string io_path = std::string("/proc/") + pe->d_name + "/io";
            std::ifstream io_file(io_path);
            if (!io_file.is_open()) continue;

            unsigned long long r_bytes = 0, w_bytes = 0;
            std::string io_line;
            while (std::getline(io_file, io_line)) {
                if (io_line.rfind("read_bytes:", 0) == 0) {
                    sscanf(io_line.c_str(), "read_bytes: %llu", &r_bytes);
                } else if (io_line.rfind("write_bytes:", 0) == 0) {
                    sscanf(io_line.c_str(), "write_bytes: %llu", &w_bytes);
                }
            }

            cur_proc_io_ticks[pid] = {r_bytes, w_bytes};

            if (g_last_proc_io_ticks.find(pid) != g_last_proc_io_ticks.end()) {
                auto const& prev = g_last_proc_io_ticks[pid];
                double delta_r = (r_bytes >= prev.r_bytes) ? ((r_bytes - prev.r_bytes) / dt_disk) : 0.0;
                double delta_w = (w_bytes >= prev.w_bytes) ? ((w_bytes - prev.w_bytes) / dt_disk) : 0.0;
                double tot = delta_r + delta_w;

                if (tot > 50.0) {
                    std::string comm = trim(readFile(std::string("/proc/") + pe->d_name + "/comm"));
                    if (comm.empty()) comm = "proc_" + std::to_string(pid);
                    disk_procs.push_back({comm.substr(0, 18), formatSpeed(delta_r), formatSpeed(delta_w), tot, pid});
                }
            }
        }
        closedir(proc_dir);
    }
    g_last_proc_io_ticks = cur_proc_io_ticks;

    // lalu saat membangun disk_procs, simpan juga pid tiap entry, dan:
    std::map<std::string, std::vector<DiskProcEntry>> disk_procs_map;
    auto mounts = getMountBaseDisks();
    for (const auto& proc : disk_procs) {
        std::string dev = diskOfPid(proc.pid, mounts);
        if (!dev.empty()) disk_procs_map[dev].push_back(proc);
    }

    // Network Sockets / Processes Extraction (100% Real-Time Dynamic)
    static std::map<int, ProcIoTick> s_last_net_proc_io;
    std::map<int, ProcIoTick> cur_net_proc_io;
    std::string ss_out = execCmd("ss -tuapn 2>/dev/null");
    std::map<int, std::string> net_pids;
    size_t ss_pos = 0;
    while ((ss_pos = ss_out.find("users:((\"", ss_pos)) != std::string::npos) {
        size_t p_start = ss_pos + 9;
        size_t p_end = ss_out.find('"', p_start);
        if (p_end == std::string::npos) break;
        std::string pname = ss_out.substr(p_start, p_end - p_start);

        size_t pid_pos = ss_out.find("pid=", p_end);
        if (pid_pos != std::string::npos && pid_pos < p_end + 40) {
            int pid = std::atoi(ss_out.c_str() + pid_pos + 4);
            if (pid > 0 && pname != "ss") {
                net_pids[pid] = pname;
            }
        }
        ss_pos = p_end + 1;
    }

    for (auto const& [pid, pname] : net_pids) {
        std::string io_str = readFile("/proc/" + std::to_string(pid) + "/io");
        unsigned long long r_b = 0, w_b = 0;
        if (!io_str.empty()) {
            std::istringstream ioss(io_str);
            std::string ioline;
            while (std::getline(ioss, ioline)) {
                if (ioline.rfind("read_bytes:", 0) == 0) sscanf(ioline.c_str(), "read_bytes: %llu", &r_b);
                else if (ioline.rfind("write_bytes:", 0) == 0) sscanf(ioline.c_str(), "write_bytes: %llu", &w_b);
            }
        }
        cur_net_proc_io[pid] = {r_b, w_b};

        double dr = 0.0, dw = 0.0;
        if (s_last_net_proc_io.find(pid) != s_last_net_proc_io.end()) {
            auto const& prev = s_last_net_proc_io[pid];
            if (dt_disk > 0.0) {
                dr = (r_b >= prev.r_bytes) ? ((r_b - prev.r_bytes) / dt_disk) : 0.0;
                dw = (w_b >= prev.w_bytes) ? ((w_b - prev.w_bytes) / dt_disk) : 0.0;
            }
        }
        double tot = dr + dw;
        net_procs.push_back({pname.substr(0, 18), formatSpeed(dw), formatSpeed(dr), tot});
    }
    s_last_net_proc_io = cur_net_proc_io;

    std::sort(net_procs.begin(), net_procs.end(), [](const NetProcEntry& a, const NetProcEntry& b) {
        return a.total > b.total;
    });

    // Real-Time GPU Clients Extraction via DRM Device Nodes
    std::string fuser_out = execCmd("fuser /dev/dri/renderD128 /dev/dri/card0 2>/dev/null");
    std::istringstream f_iss(fuser_out);
    int gpid;                          // ADD THIS
    std::set<int> seen_gpids;          // ADD THIS

    static std::map<int, std::map<std::string, unsigned long long>> s_last_gpu_ticks;
    double sum_rcs = 0.0, sum_bcs = 0.0, sum_vcs = 0.0, sum_vecs = 0.0;

    while (f_iss >> gpid) {
        if (seen_gpids.count(gpid)) continue;
        seen_gpids.insert(gpid);
        std::string comm = trim(readFile("/proc/" + std::to_string(gpid) + "/comm"));
        if (comm.empty()) continue;

        // dedupe by drm-client-id: dup()/fork()'d fds report identical cumulative ns,
        // so summing raw fds would double-count. Keep last value per client-id.
        std::map<std::string, std::map<std::string, unsigned long long>> per_client;
        std::string fdinfo_dir = "/proc/" + std::to_string(gpid) + "/fdinfo";
        DIR* d = opendir(("/proc/" + std::to_string(gpid) + "/fd").c_str());
        if (!d) continue;
        struct dirent* fe;
        while ((fe = readdir(d)) != NULL) {
            if (fe->d_name[0] == '.') continue;
            std::string content = readFile(fdinfo_dir + "/" + fe->d_name);
            if (content.find("drm-driver:") == std::string::npos) continue;
            std::istringstream fss(content);
            std::string fl, client_id = fe->d_name;
            std::map<std::string, unsigned long long> engines;
            while (std::getline(fss, fl)) {
                if (fl.rfind("drm-client-id:", 0) == 0) client_id = fl.substr(14);
                else if (fl.rfind("drm-engine-", 0) == 0) {
                    size_t c = fl.find(':');
                    engines[fl.substr(11, c - 11)] = strtoull(fl.c_str() + c + 1, nullptr, 10);
                }
            }
            if (!engines.empty()) per_client[client_id] = engines;
        }
        closedir(d);

        std::map<std::string, unsigned long long> ns;
        for (auto const& [cid, eng] : per_client)
            for (auto const& [k, v] : eng) ns[k] += v;

            auto& prev = s_last_gpu_ticks[gpid];
        auto pct = [&](const std::string& k) {
            unsigned long long cur = ns.count(k) ? ns[k] : 0, p = prev.count(k) ? prev[k] : 0;
            double d = (cur >= p && dt_disk > 0.0) ? (double)(cur - p) / (dt_disk * 1e9) * 100.0 : 0.0;
            return std::min(100.0, std::max(0.0, d));
        };
        double rcs = pct("render"), bcs = pct("copy"), vcs = pct("video"), vecs = pct("video-enhance");
        sum_rcs += rcs; sum_bcs += bcs; sum_vcs += vcs; sum_vecs += vecs;
        prev = ns;

        double total = rcs + bcs + vcs + vecs;
        if (total > 0.05) gpu_procs.push_back({comm.substr(0, 18), rcs, bcs, vcs, vecs, total});
    }

    sum_rcs = std::min(100.0, sum_rcs);
    sum_bcs = std::min(100.0, sum_bcs);
    sum_vcs = std::min(100.0, sum_vcs);
    sum_vecs = std::min(100.0, sum_vecs);

    std::sort(gpu_procs.begin(), gpu_procs.end(), [](const GpuProcEntry& a, const GpuProcEntry& b) {
        return a.total > b.total;
    });

    std::stringstream cpu_proc_json, mem_proc_json, net_proc_json, gpu_proc_json, disk_proc_json;

    // CPU JSON
    cpu_proc_json << "[";
    for (size_t i = 0; i < top_cpu_procs.size(); i++) {
        if (i > 0) cpu_proc_json << ",";
        cpu_proc_json << "{\"name\":\"" << escapeJson(top_cpu_procs[i].name) << "\",\"val\":\"" << escapeJson(top_cpu_procs[i].val) << "\"}";
    }
    cpu_proc_json << "]";

    // Memory JSON
    mem_proc_json << "[";
    for (size_t i = 0; i < top_mem_procs.size(); i++) {
        if (i > 0) mem_proc_json << ",";
        mem_proc_json << "{\"name\":\"" << escapeJson(top_mem_procs[i].name) << "\",\"val\":\"" << escapeJson(top_mem_procs[i].val) << "\"}";
    }
    mem_proc_json << "]";

    // Disk Processes JSON
    disk_proc_json << "[";
    for (size_t i = 0; i < std::min((size_t)30, disk_procs.size()); i++) {
        if (i > 0) disk_proc_json << ",";
        disk_proc_json << "{\"name\":\"" << escapeJson(disk_procs[i].name) << "\",\"read\":\"" << escapeJson(disk_procs[i].read) << "\",\"write\":\"" << escapeJson(disk_procs[i].write) << "\"}";
    }
    disk_proc_json << "]";

    std::stringstream disk_procs_map_json;
    disk_procs_map_json << "{";
    bool first_dmap = true;
    for (auto const& [dname, plist] : disk_procs_map) {
        if (!first_dmap) disk_procs_map_json << ",";
        first_dmap = false;
        disk_procs_map_json << "\"" << dname << "\":[";
        for (size_t i = 0; i < std::min((size_t)30, plist.size()); i++) {
            if (i > 0) disk_procs_map_json << ",";
            disk_procs_map_json << "{\"name\":\"" << escapeJson(plist[i].name) << "\",\"read\":\"" << escapeJson(plist[i].read) << "\",\"write\":\"" << escapeJson(plist[i].write) << "\"}";
        }
        disk_procs_map_json << "]";
    }
    disk_procs_map_json << "}";

    // GPU Processes JSON
    gpu_proc_json << "[";
    for (size_t i = 0; i < gpu_procs.size(); i++) {
        if (i > 0) gpu_proc_json << ",";
        gpu_proc_json << "{\"name\":\"" << escapeJson(gpu_procs[i].name) << "\",\"rcs\":" << gpu_procs[i].rcs << ",\"bcs\":" << gpu_procs[i].bcs << ",\"vcs\":" << gpu_procs[i].vcs << ",\"vecs\":" << gpu_procs[i].vecs << ",\"total\":" << gpu_procs[i].total << "}";
    }
    gpu_proc_json << "]";

    // Network Processes JSON
    net_proc_json << "[";
    for (size_t i = 0; i < std::min((size_t)30, net_procs.size()); i++) {
        if (i > 0) net_proc_json << ",";
        net_proc_json << "{\"name\":\"" << escapeJson(net_procs[i].name) << "\",\"down\":\"" << escapeJson(net_procs[i].down) << "\",\"up\":\"" << escapeJson(net_procs[i].up) << "\",\"total\":" << net_procs[i].total << "}";
    }
    net_proc_json << "]";

    // Detailed Specs and Tooltips
    std::string ssd_models_joined = "";
    for (size_t i = 0; i < ssd_models_list.size(); i++) {
        if (i > 0) ssd_models_joined += "<br>";
        ssd_models_joined += ssd_models_list[i];
    }

    // Ambil data riil dari kernel sebelum menyusun JSON
    unsigned long long smart_cache_bytes = getCpuSmartCacheTotalBytes();
    std::string smart_cache_str = (smart_cache_bytes > 0) ? formatBytesDynamic((double)smart_cache_bytes) : "N/A";
    RealGpuMem gpu_mem = getRealGpuMemory();

    std::string dedicated_vram_str = (gpu_mem.dedicated_total > 0) ?
    (formatBytesDynamic((double)gpu_mem.dedicated_used) + " / " + formatBytesDynamic((double)gpu_mem.dedicated_total)) : "N/A";

    std::string shared_vram_str = "N/A";
    if (gpu_mem.shared_used > 0) {
        shared_vram_str = formatBytesDynamic((double)gpu_mem.shared_used);
    } else if (gpu_mem.shared_total > 0) {
        shared_vram_str = "0 B";
    }

    // Construct Full JSON
    std::stringstream json;
    json << "{";
    json << "\"os_type\":\"linux\",";
    json << "\"hardware\":{\"cpu_model\":\"" << escapeJson(g_cpu_model) << "\",\"gpu_model\":\"" << escapeJson(g_gpu_model) << "\"},";
    json << "\"ram\":{\"total\":" << mem_total << ",\"used\":" << mem_used << ",\"free\":" << mem_avail << "},";
    json << "\"zram\":{\"total\":" << zram_total << ",\"used\":" << zram_used << "},";
    json << "\"swap\":{\"total\":" << disk_swap_total << ",\"used\":" << disk_swap_used << "},";

    // Key Root Smart Cache untuk Frontend
    json << "\"smart_cache\":{\"total\":" << smart_cache_bytes << ",\"display_str\":\"" << escapeJson(smart_cache_str) << "\"},";
    json << "\"cpu_cache\":{\"smart_cache_total\":" << smart_cache_bytes << ",\"total\":" << smart_cache_bytes << "},";

    // Key Root VRAM untuk Frontend
    json << "\"vram\":{"
    << "\"dedicated_total\":" << gpu_mem.dedicated_total << ","
    << "\"dedicated_used\":" << gpu_mem.dedicated_used << ","
    << "\"shared_total\":" << gpu_mem.shared_total << ","
    << "\"shared_used\":" << gpu_mem.shared_used << ","
    << "\"dedicated_str\":\"" << escapeJson(dedicated_vram_str) << "\","
    << "\"shared_str\":\"" << escapeJson(shared_vram_str) << "\""
    << "},";

    // CPU (Menambahkan field smart_cache)
    int core_count = (int)cpu_freqs.size();
    if (core_count <= 0) core_count = 1;
    std::vector<CoreTopologyInfo> cpu_top = getDynamicCpuTopology(core_count);

    json << "\"cpu\":{\"smart_cache\":\"" << escapeJson(smart_cache_str) << "\",\"total_usage\":" << (cpu_usages.count("cpu") ? cpu_usages["cpu"] : 0.0) << ",\"cores\":[";
    for (int i = 0; i < core_count; i++) {
        if (i > 0) json << ",";
        std::string cname = "cpu" + std::to_string(i);
        json << (cpu_usages.count(cname) ? cpu_usages[cname] : 0.0);
    }
    json << "],\"core_tags\":[";
    for (int i = 0; i < core_count; i++) {
        if (i > 0) json << ",";
        json << "\"" << cpu_top[i].tag << i << "\"";
    }
    json << "],\"core_types\":[";
    for (int i = 0; i < core_count; i++) {
        if (i > 0) json << ",";
        json << "\"" << cpu_top[i].type << "\"";
    }
    json << "],\"freqs\":[";
    for (size_t i = 0; i < cpu_freqs.size(); i++) {
        if (i > 0) json << ",";
        json << cpu_freqs[i];
    }
    json << "]},";

    // AFTER — tambahkan blok "gpus" langsung setelahnya
    json << "\"gpu\":{"
    << "\"freq\":" << gpu_freq << ","
    << "\"dedicated_vram_used\":" << gpu_mem.dedicated_used << ","
    << "\"dedicated_vram_total\":" << gpu_mem.dedicated_total << ","
    << "\"shared_vram_used\":" << gpu_mem.shared_used << ","
    << "\"shared_vram_total\":" << gpu_mem.shared_total << ","
    << "\"has_dedicated\":" << (gpu_mem.has_dedicated ? "true" : "false") << ","
    << "\"usage\":{\"rcs\":" << sum_rcs << ",\"bcs\":" << sum_bcs << ",\"vcs\":" << sum_vcs << ",\"vecs\":" << sum_vecs << "}"
    << "},";

    probeGpuAdapters();
    if (!g_gpu_adapters.empty()) g_gpu_adapters[0].usage_pct = gpu_usage_rcs;
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
        if (ga.is_egpu) {
            json << "{\"name\":\"3D/Render\",\"val\":\"" << (int)ga.usage_pct << "%\",\"sub\":\"Hardware Engine\"}";
        } else {
            json << "{\"name\":\"iGPU\",\"val\":\"Unified\",\"sub\":\"Execution Units\"}";
        }
        json << "]}";
    }
    json << "],";

    // Network
    json << "\"network\":{\"rx_rate\":" << g_cur_rx_rate << ",\"tx_rate\":" << g_cur_tx_rate << "},";

    // Disks
    json << "\"disk\":{\"read_rate\":" << g_cur_disk_read_rate << ",\"write_rate\":" << g_cur_disk_write_rate << ",\"disks\":" << disks_array.str() << ",\"list\":" << disks_array.str() << "},";

    // Sensors
    json << "\"sensors\":{\"temp\":" << temp_c << ",\"battery\":" << battery_pct << "},";

    // Processes
    json << "\"processes\":{\"cpu\":" << cpu_proc_json.str() << ",\"mem\":" << mem_proc_json.str() << ",\"disk\":" << disk_proc_json.str() << ",\"disk_per_dev\":" << disk_procs_map_json.str() << ",\"net\":" << net_proc_json.str() << ",\"gpu\":" << gpu_proc_json.str() << "},";

    json << "\"details\":{";
    json << "\"raw_cpu\":\"" << escapeJson(g_cache_raw_cpu) << "\",";
    json << "\"raw_gpu\":\"" << escapeJson(g_cache_raw_gpu) << "\",";
    json << "\"raw_ram\":\"" << escapeJson(g_cache_raw_ram) << "\",";
    json << "\"raw_net\":\"" << escapeJson(g_cache_raw_net) << "\",";
    json << "\"raw_disk\":\"" << escapeJson(g_cache_raw_disk) << "\",";
    json << "\"ram_type\":" << g_cache_ram_type_json << ",";
    json << "\"zram_info\":\"8.0 GB zstd Compressed in-RAM Swap Pool (Priority 100)\",";
    json << "\"swap_info\":\"Active ZRAM Memory Pool (Priority 100) + 32 GB NOCOW Btrfs Swapfile (Priority 10)\",";
    json << "\"ssd_model\":\"" << escapeJson(ssd_models_joined) << "\",";
    json << "\"network_type\":\"" << escapeJson(g_cache_network_type) << "\",";
    json << "\"battery_tech\":\"" << escapeJson(g_cache_battery_tech) << "\",";
    struct utsname uts_info;
    uname(&uts_info);
    std::string os_name = getOsPrettyName() + " " + std::string(uts_info.machine);
    json << "\"uptime\":" << getUptimeSeconds()
    << ",\"os\":\"" << escapeJson(os_name)
    << "\",\"kernel\":\"" << escapeJson(std::string(uts_info.release)) << "\"";
    json << "},"; // <-- Tambahkan koma di sini

    // === BLOK CREDITS DINAMIS ===
    std::string plat_target = "Linux (" + std::string(uts_info.machine) + " Architecture)";

    json << "\"credits\":{";
    json << "\"valid\":true,";
    json << "\"app_name\":\"RizkybyMONITOR\",";
    json << "\"app_version\":\"v1.1\",";
    json << "\"developer\":\"Rizky Bayu\",";
    json << "\"github\":\"https://github.com/rizkybayuu\",";
    json << "\"pills\":[\"C++\",\"HTML\",\"CSS\",\"JavaScript\",\"GTK+ 3.0\",\"WebKit2GTK\"],";
    json << "\"sections\":[";
    json << "  {\"heading\":\"System Specifications\",\"specs\":[";
    json << "    {\"label\":\"Core Runtime\",\"val\":\"Native ISO C++17\"},";
    json << "    {\"label\":\"Interface Host\",\"val\":\"WebKit2GTK / Linux Display Server (X11/Wayland)\"},";
    json << "    {\"label\":\"Telemetry Pipeline\",\"val\":\"Linux Kernel Telemetry (/proc & /sys)\"},";
    json << "    {\"label\":\"Storage Diagnostics\",\"val\":\"Sysfs Block Engine / SMART Passthrough\"},";
    json << "    {\"label\":\"Graphics Provider\",\"val\":\"Linux Direct Rendering Manager (DRM / KMS)\"}";
    json << "  ]},";
    std::string latency_val = std::to_string(g_telemetry_interval_ms.load()) + "ms Non-Blocking Polling (Sub-millisecond Compute)";
    json << "  {\"heading\":\"Architecture Details\",\"specs\":[";
    json << "    {\"label\":\"Telemetry Latency\",\"val\":\"" << escapeJson(latency_val) << "\"},";
    json << "    {\"label\":\"Subprocess Overhead\",\"val\":\"Minimal Subprocess Overhead (Kernel /sys & /proc with smartctl/nvidia-smi passthrough)\"},";
    json << "    {\"label\":\"Memory Topology\",\"val\":\"Physical RAM, ZRAM Compressed Engine & SWAP Pool\"},";
    json << "    {\"label\":\"Display Mode\",\"val\":\"Frameless GTK App-Paintable (Native Cairo Clipping)\"}";
    json << "  ]},";
    json << "  {\"heading\":\"Execution Environment\",\"specs\":[";
    json << "    {\"label\":\"Platform Target\",\"val\":\"" << escapeJson(plat_target) << "\"},";
    json << "    {\"label\":\"Security & Integrity\",\"val\":\"Standard User Mode Execution\"},";
    json << "    {\"label\":\"Binary Footprint\",\"val\":\"Compiled Native Binary / Lightweight Footprint\"}";
    json << "  ]}";
    json << "]}";

    json << "}"; // <-- Penutup Root JSON

    {
        std::lock_guard<std::mutex> lock(g_stats_mutex);
        g_cached_stats_json = json.str();
    }
    }

    static void telemetryLoop() {
        while (g_running) {
            updateTelemetry();
            int interval = g_telemetry_interval_ms.load();
            if (interval < 500) interval = 500;
            usleep((useconds_t)interval * 1000);
        }
    }

    // -------------------------------------------------------------
    // Window Management (GTK3 Native)
    // -------------------------------------------------------------
    static gboolean on_window_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
        (void)user_data;
        if (event->button == GDK_BUTTON_PRIMARY) {
            // Kalau Super/Mod4 (atau Alt, tergantung WM) sedang ditekan,
            // JANGAN rebut drag duluan — biarkan WM (Cinnamon/Mutter/dll)
            // yang menangani move/resize/snap-nya sendiri lewat protokol
            // _NET_WM_MOVERESIZE. Ini yang bikin macet di Mint kalau direbut duluan.
            if (event->state & (GDK_MOD4_MASK | GDK_MOD1_MASK)) {
                return FALSE; // biarkan event diteruskan ke WM
            }
            gtk_window_begin_move_drag(GTK_WINDOW(widget), event->button, (gint)event->x_root, (gint)event->y_root, event->time);
            return TRUE;
        }
        return FALSE;
    }

    static void saveWindowConfig() {
        std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
        if (g_windows.empty()) return;
        std::string cfg_path = g_app_dir + "/config.json";
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"telemetry_interval\": " << g_telemetry_interval_ms.load() << ",\n";
        ss << "  \"window_count\": " << g_windows.size() << ",\n";
        ss << "  \"per_window_settings\": {\n";
        bool first = true;
        for (auto const& [id, win] : g_windows) {
            gint w = 1100, h = 800, x = 100, y = 100;
            if (win.window != NULL) {
                gtk_window_get_size(GTK_WINDOW(win.window), &w, &h);
                gtk_window_get_position(GTK_WINDOW(win.window), &x, &y);
            }
            if (!first) ss << ",\n";
            first = false;
            ss << "    \"" << id << "\": {\"width\": " << w << ", \"height\": " << h << ", \"x\": " << x << ", \"y\": " << y
            << ", \"on_top\": " << (win.is_on_top ? "true" : "false")
            << ", \"details\": " << (win.details.empty() ? "[]" : win.details)
            << ", \"mode\": \"" << win.mode << "\""
            << ", \"font_size\": " << win.font_size
            << ", \"selected_disk\": \"" << win.selected_disk << "\""
            << ", \"app_title\": \"" << escapeJson(win.app_title) << "\""
            << "}";
        }
        ss << "\n  }\n}\n";
        writeFile(cfg_path, ss.str());
    }

    static gboolean on_decide_policy(WebKitWebView *web_view, WebKitPolicyDecision *decision,
                                     WebKitPolicyDecisionType type, gpointer user_data) {
        (void)user_data;

        if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION &&
            type != WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) {
            return FALSE;
            }

            WebKitNavigationAction *action = webkit_navigation_policy_decision_get_navigation_action(
                WEBKIT_NAVIGATION_POLICY_DECISION(decision));

            // HANYA KLIK KIRI (mouse button 1) yang diizinkan memicu pembukaan link ke browser!
            // Klik kanan (button 3) atau lainnya DIBLOKIR TOTAL agar tidak pernah buka website!
            guint mouse_button = webkit_navigation_action_get_mouse_button(action);
        if (mouse_button != 1 && mouse_button != 0) {
            webkit_policy_decision_ignore(decision);
            return TRUE;
        }

        WebKitURIRequest *request = webkit_navigation_action_get_request(action);
        const char *uri = webkit_uri_request_get_uri(request);
        if (!uri) return FALSE;

        std::string url(uri);
        if (url.rfind("http://127.0.0.1:", 0) == 0 || url.rfind("http://localhost:", 0) == 0) {
            return FALSE;
        }

		// Ganti std::system(cmd.c_str()); dengan ini:
		GError* err = NULL;
		if (!g_app_info_launch_default_for_uri(uri, NULL, &err)) {
			if (err) g_error_free(err);
		}

        webkit_policy_decision_ignore(decision);
        return TRUE;
    }

    static gboolean on_web_view_context_menu(WebKitWebView *web_view, gpointer context_menu, gpointer event, gpointer user_data) {
        (void)web_view; (void)context_menu; (void)event; (void)user_data;
        return TRUE;
    }

    static gboolean on_window_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
        (void)widget; (void)user_data;
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_paint(cr);
        return FALSE;
    }

    static gboolean on_window_configure(GtkWidget *widget, gpointer event, gpointer user_data) {
        (void)widget; (void)event; (void)user_data;
        saveWindowConfig();
        return FALSE;
    }

    static void createNewWindow(int win_id) {
        GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

        gtk_widget_set_app_paintable(win, TRUE);
        GdkScreen *screen = gdk_screen_get_default();
        if (!gdk_screen_is_composited(screen)) {
            std::cerr << "[WARN] Compositor tidak aktif — rounded/transparent window bisa tampil solid hitam." << std::endl;
        }
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
        if (visual) gtk_widget_set_visual(win, visual);

        int w_width = 1100, w_height = 800;
        int w_x = -1, w_y = -1;
        bool w_on_top = false;
        std::string w_details = "[]";
        std::string w_mode = "dark";
        int w_font_size = 14;
        std::string w_selected_disk = "sda";
        std::string w_app_title = "";

        std::string cfg_str = readFile(g_app_dir + "/config.json");
        if (!cfg_str.empty()) {
            std::string key = "\"" + std::to_string(win_id) + "\":";
            size_t pos = cfg_str.find(key);
            if (pos != std::string::npos) {
                size_t b_open = cfg_str.find('{', pos);
                size_t b_close = cfg_str.find('}', b_open);
                if (b_open != std::string::npos && b_close != std::string::npos) {
                    std::string block = cfg_str.substr(b_open, b_close - b_open + 1);
                    size_t pw = block.find("\"width\":");
                    if (pw != std::string::npos) sscanf(block.c_str() + pw, "\"width\": %d", &w_width);
                    size_t ph = block.find("\"height\":");
                    if (ph != std::string::npos) sscanf(block.c_str() + ph, "\"height\": %d", &w_height);
                    size_t px = block.find("\"x\":");
                    if (px != std::string::npos) sscanf(block.c_str() + px, "\"x\": %d", &w_x);
                    size_t py = block.find("\"y\":");
                    if (py != std::string::npos) sscanf(block.c_str() + py, "\"y\": %d", &w_y);
                    if (block.find("\"on_top\": true") != std::string::npos || block.find("\"on_top\":true") != std::string::npos) {
                        w_on_top = true;
                    }
                    size_t d_pos = block.find("\"details\":");
                    if (d_pos != std::string::npos) {
                        size_t arr_s = block.find('[', d_pos);
                        size_t arr_e = block.find(']', arr_s);
                        if (arr_s != std::string::npos && arr_e != std::string::npos) {
                            w_details = block.substr(arr_s, arr_e - arr_s + 1);
                        }
                    }
                    size_t m_pos = block.find("\"mode\":");
                    if (m_pos != std::string::npos) {
                        if (block.find("\"light\"", m_pos) != std::string::npos) w_mode = "light";
                        else if (block.find("\"dark\"", m_pos) != std::string::npos) w_mode = "dark";
                    }
                    size_t f_pos = block.find("\"font_size\":");
                    if (f_pos != std::string::npos) {
                        sscanf(block.c_str() + f_pos, "\"font_size\": %d", &w_font_size);
                    }
                    size_t s_pos = block.find("\"selected_disk\":");
                    if (s_pos != std::string::npos) {
                        size_t q1 = block.find('"', s_pos + 16);
                        size_t q2 = (q1 != std::string::npos) ? block.find('"', q1 + 1) : std::string::npos;
                        if (q1 != std::string::npos && q2 != std::string::npos) {
                            w_selected_disk = block.substr(q1 + 1, q2 - q1 - 1);
                        }
                    }
                    size_t at_pos = block.find("\"app_title\":");
                    if (at_pos != std::string::npos) {
                        size_t q1 = block.find('"', at_pos + 12);
                        size_t q2 = (q1 != std::string::npos) ? block.find('"', q1 + 1) : std::string::npos;
                        if (q1 != std::string::npos && q2 != std::string::npos) {
                            w_app_title = block.substr(q1 + 1, q2 - q1 - 1);
                        }
                    }
                }
            }
        }

        std::string title = w_app_title.empty() ? ("RizkybyMONITOR " + std::to_string(win_id)) : w_app_title;
        gtk_window_set_title(GTK_WINDOW(win), title.c_str());
        gtk_window_set_default_size(GTK_WINDOW(win), w_width, w_height);

        // TRUE FRAMELESS: No titlebar, no window borders
        gtk_window_set_decorated(GTK_WINDOW(win), FALSE);

        if (w_x < 0 || w_y < 0) {
            w_x = 100 + ((win_id - 1) * 40);
            w_y = 100 + ((win_id - 1) * 40);
        }
        gtk_window_move(GTK_WINDOW(win), w_x, w_y);
        gtk_window_resize(GTK_WINDOW(win), w_width, w_height);

        if (w_on_top) {
            gtk_window_set_keep_above(GTK_WINDOW(win), TRUE);
        }

        g_signal_connect_data(win, "button-press-event", (GCallback)on_window_button_press, NULL, NULL, (GConnectFlags)0);
        g_signal_connect_data(win, "configure-event", (GCallback)on_window_configure, (gpointer)(intptr_t)win_id, NULL, (GConnectFlags)0);
        g_signal_connect_data(win, "draw", (GCallback)on_window_draw, NULL, NULL, (GConnectFlags)0);

        GtkWidget *webview = webkit_web_view_new();
        // Background WebKit transparan murni
        GdkRGBA transparent = { 0.0, 0.0, 0.0, 0.0 };
        webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(webview), &transparent);

        WebKitSettings *settings = webkit_web_view_get_settings((WebKitWebView*)webview);
        webkit_settings_set_enable_developer_extras(settings, TRUE);
        webkit_settings_set_enable_webgl(settings, TRUE);
        webkit_settings_set_enable_webaudio(settings, TRUE);
        webkit_settings_set_enable_smooth_scrolling(settings, TRUE);
        webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);

        g_signal_connect_data(webview, "context-menu", (GCallback)on_web_view_context_menu, NULL, NULL, (GConnectFlags)0);
        g_signal_connect(webview, "decide-policy", G_CALLBACK(on_decide_policy), NULL);
        gtk_container_add(GTK_CONTAINER(win), webview);

        std::string url = "http://127.0.0.1:" + std::to_string(g_server_port) + "/?win=" + std::to_string(win_id);
        webkit_web_view_load_uri((WebKitWebView*)webview, url.c_str());

        WindowInstance inst;
        inst.id = win_id;
        inst.window = win;
        inst.webview = webview;
        inst.is_on_top = w_on_top;
        inst.details = w_details;
        inst.mode = w_mode;
        inst.font_size = w_font_size;
        inst.selected_disk = w_selected_disk;
        inst.app_title = w_app_title;

        {
            std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
            g_windows[win_id] = inst;
        }

        gtk_widget_show_all(win);
    }

    static gboolean cb_create_window(gpointer data) {
        int win_id = (int)(intptr_t)data;
        createNewWindow(win_id);
        return FALSE;
    }

    static gboolean cb_close_window(gpointer data) {
        int win_id = (int)(intptr_t)data;
        std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
        if (g_windows.find(win_id) != g_windows.end()) {
            gtk_widget_destroy(g_windows[win_id].window);
            g_windows.erase(win_id);
            saveWindowConfig();
        }
        if (g_windows.empty()) {
            gtk_main_quit();
            exit(0);
        }
        return FALSE;
    }

    static gboolean cb_toggle_on_top(gpointer data) {
        int win_id = (int)(intptr_t)data;
        std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
        if (g_windows.find(win_id) != g_windows.end()) {
            bool cur = g_windows[win_id].is_on_top;
            g_windows[win_id].is_on_top = !cur;
            gtk_window_set_keep_above(GTK_WINDOW(g_windows[win_id].window), !cur);
            saveWindowConfig();
        }
        return FALSE;
    }

    static gboolean cb_copy_clipboard(gpointer data) {
        char *text = (char*)data;
        if (text) {
            GdkAtom atom = gdk_atom_intern("CLIPBOARD", FALSE);
            GtkClipboard *clipboard = gtk_clipboard_get_for_display(gdk_display_get_default(), atom);
            if (clipboard) {
                gtk_clipboard_set_text(clipboard, text, -1);
                // Opsional: panggil gtk_clipboard_store() agar data bertahan setelah aplikasi ditutup
                //gtk_clipboard_store(clipboard);
            }
            free(text);
        }
        return FALSE;
    }

    static gboolean cb_quit_app(gpointer) {
        saveWindowConfig();
        gtk_main_quit();
        exit(0);
        return FALSE;
    }

    // -------------------------------------------------------------
    // Native HTTP Server
    // -------------------------------------------------------------
    static void handleClient(int client_fd) {
        std::string req;
        char buffer[4096];
        ssize_t n;
        while ((n = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
            req.append(buffer, n);
            if (req.find("\r\n\r\n") != std::string::npos) {
                size_t cl_pos = req.find("Content-Length:");
                if (cl_pos != std::string::npos) {
                    size_t cl = std::strtoul(req.c_str() + cl_pos + 15, nullptr, 10);
                    size_t body_start = req.find("\r\n\r\n") + 4;
                    if (req.size() - body_start >= cl) break;
                } else {
                    break;
                }
            }
        }
        if (req.empty()) {
            close(client_fd);
            return;
        }
        std::istringstream iss(req);
        std::string method, path, protocol;
        iss >> method >> path >> protocol;

        std::string response_headers;
        std::string response_body;

        if (method == "GET" && (path == "/" || path.rfind("/?", 0) == 0 || path == "/index.html")) {
            std::string html_path = g_app_dir + "/index.html";
            response_body = readFile(html_path);
            response_headers = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
        }
        else if (method == "GET" && (path == "/api/stats" || path.rfind("/api/stats", 0) == 0)) {
            {
                std::lock_guard<std::mutex> lock(g_stats_mutex);
                response_body = g_cached_stats_json;
            }
            response_headers = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
        }
        else if (method == "GET" && path.rfind("/api/config", 0) == 0) {
            std::string cfg = readFile(g_app_dir + "/config.json");
            if (cfg.empty()) cfg = "{\"window_count\": 1}";
            response_body = cfg;
            response_headers = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
        }
        else if (method == "POST" && path == "/api/config") {
            size_t body_pos = req.find("\r\n\r\n");
            if (body_pos != std::string::npos) {
                std::string body = req.substr(body_pos + 4);
                int wid = 1;
                size_t wid_pos = body.find("\"window_id\":");
                if (wid_pos != std::string::npos) {
                    size_t val_pos = body.find_first_of("0123456789", wid_pos + 11);
                    if (val_pos != std::string::npos) {
                        wid = std::atoi(body.c_str() + val_pos);
                    }
                }
                if (wid < 1) wid = 1;

                size_t ti_pos = body.find("\"telemetry_interval\"");
                if (ti_pos == std::string::npos) ti_pos = body.find("\"telemetry_ms\"");
                if (ti_pos != std::string::npos) {
                    size_t val_pos = body.find_first_of("0123456789", ti_pos + 14);
                    if (val_pos != std::string::npos) {
                        int interval = std::atoi(body.c_str() + val_pos);
                        if (interval >= 500) {
                            g_telemetry_interval_ms = interval;
                        }
                    }
                }

                std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
                if (g_windows.find(wid) == g_windows.end()) {
                    WindowInstance inst;
                    inst.id = wid;
                    inst.window = NULL;
                    inst.webview = NULL;
                    inst.is_on_top = false;
                    g_windows[wid] = inst;
                }

                size_t d_pos = body.find("\"details\":");
                if (d_pos != std::string::npos) {
                    size_t arr_s = body.find('[', d_pos);
                    size_t arr_e = body.find(']', arr_s);
                    if (arr_s != std::string::npos && arr_e != std::string::npos) {
                        g_windows[wid].details = body.substr(arr_s, arr_e - arr_s + 1);
                    }
                }
                size_t m_pos = body.find("\"mode\":");
                if (m_pos != std::string::npos) {
                    if (body.find("\"light\"", m_pos) != std::string::npos && body.find("\"light\"", m_pos) < m_pos + 20) g_windows[wid].mode = "light";
                    else if (body.find("\"dark\"", m_pos) != std::string::npos && body.find("\"dark\"", m_pos) < m_pos + 20) g_windows[wid].mode = "dark";
                }
                size_t f_pos = body.find("\"font_size\":");
                if (f_pos != std::string::npos) {
                    int fs = 14;
                    if (sscanf(body.c_str() + f_pos, "\"font_size\": %d", &fs) == 1) {
                        g_windows[wid].font_size = fs;
                    }
                }
                size_t s_pos = body.find("\"selected_disk\":");
                if (s_pos != std::string::npos) {
                    size_t q1 = body.find('"', s_pos + 16);
                    size_t q2 = (q1 != std::string::npos) ? body.find('"', q1 + 1) : std::string::npos;
                    if (q1 != std::string::npos && q2 != std::string::npos) {
                        g_windows[wid].selected_disk = body.substr(q1 + 1, q2 - q1 - 1);
                    }
                }
                size_t at_pos = body.find("\"app_title\":");
                if (at_pos != std::string::npos) {
                    size_t q1 = body.find('"', at_pos + 12);
                    size_t q2 = (q1 != std::string::npos) ? body.find('"', q1 + 1) : std::string::npos;
                    if (q1 != std::string::npos && q2 != std::string::npos) {
                        g_windows[wid].app_title = body.substr(q1 + 1, q2 - q1 - 1);
                        if (g_windows[wid].window != NULL) {
                            gtk_window_set_title(GTK_WINDOW(g_windows[wid].window), g_windows[wid].app_title.c_str());
                        }
                    }
                }
            }
            saveWindowConfig();
            response_body = "{\"status\":\"ok\"}";
            response_headers = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
        }
        else if (method == "POST" && path == "/api/copy") {
            size_t body_pos = req.find("\r\n\r\n");
            if (body_pos != std::string::npos) {
                std::string body = req.substr(body_pos + 4);
                g_idle_add(cb_copy_clipboard, strdup(body.c_str()));
            }
            response_body = "{\"status\":\"ok\"}";
            response_headers = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
        }
        else if (method == "POST" && path == "/api/duplicate") {
            g_next_win_id++;
            g_idle_add(cb_create_window, (gpointer)(intptr_t)g_next_win_id);
            response_body = "{\"status\":\"ok\",\"win\":" + std::to_string(g_next_win_id) + "}";
            response_headers = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
        }
        else if (method == "POST" && path.rfind("/api/on_top", 0) == 0) {
            int target_id = 1;
            size_t q = path.find("win=");
            if (q != std::string::npos) target_id = std::atoi(path.substr(q + 4).c_str());

            bool next_state = true;
            {
                std::lock_guard<std::recursive_mutex> lock(g_win_mutex);
                if (g_windows.find(target_id) != g_windows.end()) {
                    next_state = !g_windows[target_id].is_on_top;
                }
            }
            g_idle_add(cb_toggle_on_top, (gpointer)(intptr_t)target_id);
            response_body = "{\"status\":\"ok\",\"on_top\":" + std::string(next_state ? "true" : "false") + "}";
            response_headers = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
        }
        else if (method == "POST" && path.rfind("/api/close_window", 0) == 0) {
            int target_id = 1;
            size_t q = path.find("win=");
            if (q != std::string::npos) target_id = std::atoi(path.substr(q + 4).c_str());
            g_idle_add(cb_close_window, (gpointer)(intptr_t)target_id);
            response_body = "{\"status\":\"ok\"}";
            response_headers = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
        }
        else if (method == "POST" && path == "/api/quit") {
            g_idle_add(cb_quit_app, NULL);
            response_body = "{\"status\":\"ok\"}";
            response_headers = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
        }
		else if (method == "GET" && path.rfind("/api/disk-detail", 0) == 0) {
            std::string dev = "sda";
            size_t q = path.find("dev=");
            if (q != std::string::npos) {
                dev = path.substr(q + 4);
                size_t amp = dev.find('&');
                if (amp != std::string::npos) dev = dev.substr(0, amp);
            }

            // Sanitasi ketat: Cegah Command Injection (hanya izinkan alphanumeric, dash, underscore)
            bool is_safe = !dev.empty() && dev.size() <= 32;
            for (char c : dev) {
                if (!isalnum((unsigned char)c) && c != '_' && c != '-') {
                    is_safe = false;
                    break;
                }
            }

            if (!is_safe) {
                response_body = "{\"error\":\"Invalid device identifier\"}";
                response_headers = "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
            } else {
                std::string detail = "";
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
                response_body = "{\"detail\":\"" + escapeJson(detail) + "\"}";
                response_headers = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(response_body.size()) + "\r\n\r\n";
            }
        }
        else {
            response_body = "404 Not Found";
            response_headers = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n";
        }

        send(client_fd, response_headers.c_str(), response_headers.size(), 0);
        send(client_fd, response_body.c_str(), response_body.size(), 0);
        close(client_fd);
    }

    static void startHttpServer() {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) return;

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        #ifdef SO_REUSEPORT
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        #endif

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        bool bound = false;
        for (int p = 8080; p <= 8095; ++p) {
            addr.sin_port = htons(p);
            if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) >= 0) {
                g_server_port = p;
                bound = true;
                break;
            }
        }
        if (!bound) {
            close(server_fd);
            return;
        }

        if (listen(server_fd, 64) < 0) {
            close(server_fd);
            return;
        }

        while (g_running) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd >= 0) {
                std::thread(handleClient, client_fd).detach();
            }
        }
        close(server_fd);
    }

    // -------------------------------------------------------------
    // Main Entry Point
    // -------------------------------------------------------------
    int main(int argc, char *argv[]) {
        if (!getenv("DISPLAY")) {
            setenv("DISPLAY", ":0", 0);
        }
        if (!getenv("WAYLAND_DISPLAY")) {
            setenv("WAYLAND_DISPLAY", "wayland-0", 0);
        }
        setenv("GDK_BACKEND", "x11", 0);
        setenv("GTK_CSD", "0", 1);

        char exe_buf[1024];
        ssize_t len = readlink("/proc/self/exe", exe_buf, sizeof(exe_buf) - 1);
        if (len > 0) {
            exe_buf[len] = '\0';
            std::string full_path(exe_buf);
            size_t last_slash = full_path.find_last_of('/');
            if (last_slash != std::string::npos) {
                g_app_dir = full_path.substr(0, last_slash);
            }
        }
        if (g_app_dir.empty()) {
            g_app_dir = "."; // Langsung pakai direktori aktif saat perintah dijalankan
        }

        // Start HTTP Server immediately so port 8080 binds with 0 latency
        std::thread server_thread(startHttpServer);
        server_thread.detach();

        // Initial Telemetry Sample
        updateTelemetry();

        std::thread hw_thread([]() {
            initHardwareInfo();
            updateTelemetry();
        });
        hw_thread.detach();

        std::thread telem_thread(telemetryLoop);
        telem_thread.detach();

        usleep(50000);

        // Initialize GTK
        if (!gtk_init_check(&argc, &argv)) {
            std::cerr << "Note: Display server is not currently running or reachable (e.g. in headless/TTY mode)." << std::endl;
            std::cerr << "Run 'plasma on' or launch from graphical desktop." << std::endl;
            while (g_running) {
                sleep(1);
            }
            return 1;
        }

        // Read saved window count
        int num_windows = 1;
        std::string cfg = readFile(g_app_dir + "/config.json");
        if (!cfg.empty()) {
            size_t ti_pos = cfg.find("\"telemetry_interval\"");
            if (ti_pos != std::string::npos) {
                size_t val_pos = cfg.find_first_of("0123456789", ti_pos + 20);
                if (val_pos != std::string::npos) {
                    int interval = std::atoi(cfg.c_str() + val_pos);
                    if (interval >= 500) g_telemetry_interval_ms = interval;
                }
            }
            size_t pos = cfg.find("\"window_count\":");
            if (pos != std::string::npos) {
                sscanf(cfg.c_str() + pos, "\"window_count\": %d", &num_windows);
            }
        }
        if (num_windows < 1) num_windows = 1;

        for (int i = 1; i <= num_windows; i++) {
            g_next_win_id = i;
            createNewWindow(i);
        }

        gtk_main();
        return 0;
    }
