import http.server
import socketserver
import json
import time
import os
import glob
import subprocess
from http import HTTPStatus
import re
import threading

PORT = 8080

# Global state for calculating rates
last_cpu_stats = {}
last_net = {'time': time.time(), 'rx': 0, 'tx': 0}
last_disk = {'time': time.time(), 'read': 0, 'write': 0}
open_window_callback = None
close_window_callback = None
toggle_on_top_callback = None

def set_open_window_callback(fn):
    global open_window_callback
    open_window_callback = fn

def set_close_window_callback(fn):
    global close_window_callback
    close_window_callback = fn

def set_toggle_on_top_callback(fn):
    global toggle_on_top_callback
    toggle_on_top_callback = fn

RIZKY_UUID = "67003a8d-300a-4430-8468-ed19441e0027"
rizky_base_dev = None
rizky_initial_writes = None
BASE_TBW = 8.86 # Hasil baca manual dari smartctl (dalam TB)
MAX_TBW = 600.0 # Asumsi SSD 1TB SanDisk Extreme

cpu_model_name = "Unknown CPU"
try:
    with open('/proc/cpuinfo', 'r') as f:
        for line in f:
            if "model name" in line:
                cpu_model_name = line.split(':')[1].strip()
                break
except:
    pass

gpu_model_name = "Intel Corporation Alder Lake-UP3 GT2 [Iris Xe Graphics]"

def get_fallback_net_processes():
    try:
        cmd = 'echo "232390183" | sudo -S ss -tuapn 2>/dev/null'
        out = subprocess.check_output(cmd, shell=True, text=True, timeout=0.8)
        pnames = []
        for line in out.splitlines():
            if 'users:' in line:
                idx = line.find('users:(("')
                if idx != -1:
                    pname = line[idx+9:].split('"')[0]
                    if pname and pname not in pnames and pname != 'ss':
                        pnames.append(pname)
        
        fallback = []
        for idx, pname in enumerate(pnames):
            fallback.append({
                'name': pname[:18],
                'up': "0.0 KB/s",
                'down': "0.0 KB/s",
                'total': 0.0
            })
        
        default_services = ['NetworkManager', 'systemd-resolved', 'firefox', 'chrome', 'sshd']
        for sname in default_services:
            if not any(f['name'] == sname for f in fallback):
                fallback.append({
                    'name': sname,
                    'up': '0.0 KB/s',
                    'down': '0.0 KB/s',
                    'total': 0.0
                })
        return fallback[:30]
    except:
        return [
            {'name': 'NetworkManager', 'up': '0.1 KB/s', 'down': '0.2 KB/s', 'total': 0.3},
            {'name': 'systemd-resolved', 'up': '0.1 KB/s', 'down': '0.1 KB/s', 'total': 0.2},
            {'name': 'firefox', 'up': '0.1 KB/s', 'down': '0.1 KB/s', 'total': 0.2},
            {'name': 'chrome', 'up': '0.0 KB/s', 'down': '0.1 KB/s', 'total': 0.1},
            {'name': 'sshd', 'up': '0.0 KB/s', 'down': '0.1 KB/s', 'total': 0.1}
        ]

nethogs_top = get_fallback_net_processes()

def update_nethogs_loop():
    global nethogs_top
    import subprocess, os, re
    cmd = 'echo "232390183" | sudo -S stdbuf -oL -eL nethogs -t 2>/dev/null'
    while True:
        try:
            p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, text=True, bufsize=1)
            proc_map = {}
            while True:
                line = p.stdout.readline()
                if not line: break
                parts = line.strip().split('\t')
                if len(parts) >= 3:
                    prog, sent, recv = parts[0], parts[1], parts[2]
                    tokens = [t for t in prog.split('/') if t]
                    pname = None
                    if len(tokens) >= 3 and tokens[-1].isdigit() and tokens[-2].isdigit():
                        pname = tokens[-3]
                    elif len(tokens) >= 1:
                        pname = tokens[0] if not tokens[0].isdigit() else None

                    if pname and pname not in ('unknown', 'unknown TCP', 'ss'):
                        try:
                            sent_kb = float(sent)
                            recv_kb = float(recv)
                            proc_map[pname] = {'sent': sent_kb, 'recv': recv_kb}
                        except: pass
                elif 'Refreshing:' in line:
                    all_procs = []
                    seen = set()
                    try:
                        ss_out = subprocess.check_output("echo 232390183 | sudo -S ss -tuapn 2>/dev/null", shell=True, text=True, timeout=0.5)
                        for sline in ss_out.splitlines():
                            if "users:" in sline:
                                for m in re.findall(r'users:\(\("([^"]+)"', sline):
                                    if m and m not in seen and m != 'ss':
                                        seen.add(m)
                                        all_procs.append(m)
                    except: pass

                    res = []
                    for k, v in sorted(proc_map.items(), key=lambda x: x[1]['sent'] + x[1]['recv'], reverse=True):
                        if k not in seen:
                            seen.add(k)
                        s_val = v['sent']
                        r_val = v['recv']
                        res.append({
                            'name': k[:18],
                            'up': f"{s_val:.1f} KB/s",
                            'down': f"{r_val:.1f} KB/s",
                            'total': s_val + r_val
                        })
                    
                    for pname in all_procs:
                        if pname not in proc_map:
                            res.append({
                                'name': pname[:18],
                                'up': "0.0 KB/s",
                                'down': "0.0 KB/s",
                                'total': 0.0
                            })
                    
                    if res:
                        nethogs_top = res[:30]
                    proc_map = {}
        except:
            pass
        time.sleep(0.5)

gpu_usage = {'rcs': 0.0, 'bcs': 0.0, 'vcs': 0.0, 'vecs': 0.0}
gpu_processes = []

def update_gpu_loop():
    global gpu_usage, gpu_processes
    import subprocess, json, time
    cmd = 'echo "232390183" | sudo -S stdbuf -oL -eL intel_gpu_top -J 2>/dev/null'
    while True:
        try:
            p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, text=True, bufsize=1)
            buf = ""
            depth = 0
            for line in p.stdout:
                if not line: break
                for char in line:
                    if char == "{":
                        depth += 1
                    elif char == "}":
                        depth -= 1
                    buf += char
                    if depth == 0 and "{" in buf:
                        try:
                            s_idx = buf.find("{")
                            data = json.loads(buf[s_idx:])
                            buf = ""
                            
                            # 1. Overall GPU Usage
                            engines = data.get("engines", {})
                            rcs, bcs, vcs, vecs = 0.0, 0.0, 0.0, 0.0
                            for k, v in engines.items():
                                busy = float(v.get("busy", 0))
                                if "Render/3D" in k: rcs = max(rcs, busy)
                                elif "Blitter" in k: bcs = max(bcs, busy)
                                elif "VideoEnhance" in k: vecs = max(vecs, busy)
                                elif "Video" in k: vcs = max(vcs, busy)
                            gpu_usage = {'rcs': round(rcs, 1), 'bcs': round(bcs, 1), 'vcs': round(vcs, 1), 'vecs': round(vecs, 1)}
                            
                            # 2. Per-Process GPU Usage breakdown (All active GPU clients)
                            clients = data.get("clients", {})
                            plist = []
                            for cid, cinfo in clients.items():
                                pname = cinfo.get("name", "unknown")
                                e_classes = cinfo.get("engine-classes", {})
                                c_rcs = float(e_classes.get("Render/3D", {}).get("busy", 0))
                                c_bcs = float(e_classes.get("Blitter", {}).get("busy", 0))
                                c_vcs = float(e_classes.get("Video", {}).get("busy", 0))
                                c_vecs = float(e_classes.get("VideoEnhance", {}).get("busy", 0))
                                total = c_rcs + c_bcs + c_vcs + c_vecs
                                
                                plist.append({
                                    'name': pname[:18],
                                    'rcs': round(c_rcs, 1),
                                    'bcs': round(c_bcs, 1),
                                    'vcs': round(c_vcs, 1),
                                    'vecs': round(c_vecs, 1),
                                    'total': round(total, 1)
                                })
                            
                            plist.sort(key=lambda x: (x['total'], x['name']), reverse=True)
                            gpu_processes = plist[:20]
                        except:
                            buf = ""
        except:
            pass
        time.sleep(0.5)

def get_rizky_device():
    global rizky_base_dev
    if not rizky_base_dev:
        uuid_path = f"/dev/disk/by-uuid/{RIZKY_UUID}"
        if os.path.exists(uuid_path):
            real_path = os.path.realpath(uuid_path)
            dev_name = os.path.basename(real_path)
            match = re.match(r'([a-zA-Z]+[0-9]*[a-zA-Z]*)[0-9]*', dev_name)
            if match:
                if 'nvme' in dev_name:
                    rizky_base_dev = dev_name.split('p')[0]
                else:
                    rizky_base_dev = dev_name.rstrip('0123456789')
    return rizky_base_dev

def get_cpu_stats():
    stats = {}
    try:
        with open('/proc/stat', 'r') as f:
            for line in f:
                if line.startswith('cpu'):
                    parts = line.split()
                    name = parts[0]
                    idle = float(parts[4]) + float(parts[5])
                    total = sum(float(x) for x in parts[1:8])
                    stats[name] = {'idle': idle, 'total': total}
    except:
        pass
    return stats

def get_cpu_usage():
    global last_cpu_stats
    current_stats = get_cpu_stats()
    usages = {}
    for name, current in current_stats.items():
        if name in last_cpu_stats:
            last = last_cpu_stats[name]
            total_diff = current['total'] - last['total']
            idle_diff = current['idle'] - last['idle']
            if total_diff > 0:
                usage = 100.0 * (1.0 - idle_diff / total_diff)
                usages[name] = round(usage, 1)
            else:
                usages[name] = 0.0
        else:
            usages[name] = 0.0
    last_cpu_stats = current_stats
    return usages

def get_cpu_freqs():
    freqs = []
    for i in range(12):
        f = f'/sys/devices/system/cpu/cpu{i}/cpufreq/scaling_cur_freq'
        if os.path.exists(f):
            try:
                with open(f, 'r') as fp:
                    freqs.append(int(fp.read().strip()) // 1000)
            except:
                freqs.append(0)
        else:
            freqs.append(0)
    return freqs

def get_mem_info():
    mem = {}
    try:
        with open('/proc/meminfo', 'r') as f:
            for line in f:
                parts = line.split(':')
                if len(parts) == 2:
                    mem[parts[0].strip()] = int(parts[1].strip().split()[0]) * 1024
    except:
        pass
    return mem

def get_swaps():
    zram = {'total': 0, 'used': 0}
    disk_swap = {'total': 0, 'used': 0}
    try:
        with open('/proc/swaps', 'r') as f:
            lines = f.readlines()[1:]
            for line in lines:
                parts = line.split()
                if len(parts) >= 3:
                    name, total, used = parts[0], int(parts[2])*1024, int(parts[3])*1024
                    if 'zram' in name:
                        zram['total'] += total
                        zram['used'] += used
                    else:
                        disk_swap['total'] += total
                        disk_swap['used'] += used
    except:
        pass
    return zram, disk_swap

def get_gpu_freq():
    f = '/sys/class/drm/card0/gt_cur_freq_mhz'
    if os.path.exists(f):
        try:
            with open(f, 'r') as fp: return int(fp.read().strip())
        except:
            pass
    return 0

net_stats = {'rx_rate': 0.0, 'tx_rate': 0.0}
def update_net_loop():
    global net_stats
    last_rx, last_tx = 0, 0
    last_t = time.time()
    while True:
        try:
            rx, tx = 0, 0
            with open('/proc/net/dev', 'r') as f:
                for line in f.readlines()[2:]:
                    parts = line.split()
                    if not parts[0].startswith('lo:'):
                        if ":" in parts[0]:
                            sub = parts[0].split(":")[1]
                            if sub:
                                rx += int(sub)
                                tx += int(parts[8])
                            else:
                                rx += int(parts[1])
                                tx += int(parts[9])
            now = time.time()
            dt = now - last_t
            if dt > 0 and last_rx > 0:
                net_stats['rx_rate'] = round((rx - last_rx) / dt, 1)
                net_stats['tx_rate'] = round((tx - last_tx) / dt, 1)
            last_rx, last_tx, last_t = rx, tx, now
        except:
            pass
        time.sleep(0.5)

def get_network_stats():
    return net_stats

TBW_STATE_FILE = os.path.expanduser('~/.rizkybymonitor_tbw.json')
def load_tbw():
    try:
        if os.path.exists(TBW_STATE_FILE):
            with open(TBW_STATE_FILE, 'r') as f:
                return float(json.load(f).get('base_tbw', 8.900))
    except:
        pass
    return 8.900 # Fallback initial

def save_tbw(tbw):
    try:
        with open(TBW_STATE_FILE, 'w') as f:
            json.dump({'base_tbw': tbw}, f)
    except:
        pass

rizky_initial_writes = None
base_tbw_cached = load_tbw()
last_save_time = 0

smart_cache = {}

def get_disk_smart_info(dname, tran, is_rizky, current_lifetime_tbw, remaining_tbw):
    now = time.time()
    cached = smart_cache.get(dname)
    if cached and (now - cached['time'] < 10):
        return cached['tbw'], cached['temp'], cached['health'], cached['remaining']

    temp_str = "N/A"
    health_str = "PASSED / OK"
    tbw_str = "High-Speed Storage"
    remaining_str = "N/A"

    if is_rizky:
        tbw_str = f"{current_lifetime_tbw:.3f} TB Written"
        remaining_str = f"{remaining_tbw:.3f} TB Life Remaining"
        temp_str = "38 °C"
    elif tran == "nvme" or "nvme" in dname:
        try:
            cmd_sm = f'echo "232390183" | sudo -S smartctl -a /dev/{dname} 2>/dev/null'
            out_sm = subprocess.check_output(cmd_sm, shell=True, text=True, timeout=0.5)
            pct_val = None
            written_tb_val = None
            for sline in out_sm.splitlines():
                if 'Temperature:' in sline:
                    temp_str = sline.split(':', 1)[1].strip().split()[0] + " °C"
                if 'Data Units Written:' in sline:
                    m_tbw = re.search(r'\[(.*?)\]', sline)
                    if m_tbw:
                        raw_tbw = m_tbw.group(1).replace(' Written', '').strip()
                        tbw_str = f"{raw_tbw} Written"
                        m_num = re.search(r'([\d\.]+)\s*TB', raw_tbw, re.IGNORECASE)
                        if m_num:
                            written_tb_val = float(m_num.group(1))
                if 'Percentage Used:' in sline:
                    m_pct = re.search(r'(\d+)%', sline)
                    if m_pct:
                        pct_val = int(m_pct.group(1))
                if 'result:' in sline or 'assessment' in sline:
                    if ':' in sline:
                        health_str = sline.split(':', 1)[1].strip()
            
            if written_tb_val is not None and pct_val is not None and pct_val > 0:
                total_rated_tb = written_tb_val / (pct_val / 100.0)
                rem_tb = max(0.0, total_rated_tb - written_tb_val)
                remaining_str = f"{rem_tb:.3f} TB Life Remaining"
            elif written_tb_val is not None:
                remaining_str = f"Estimated {written_tb_val * 2.5:.1f} TB Rated"
            else:
                remaining_str = "N/A"
        except: pass
    
    smart_cache[dname] = {'time': now, 'tbw': tbw_str, 'temp': temp_str, 'health': health_str, 'remaining': remaining_str}
    return tbw_str, temp_str, health_str, remaining_str

last_disk_per_dev = {}

def get_disk_stats():
    global last_disk, last_disk_per_dev, rizky_initial_writes, base_tbw_cached, last_save_time
    now = time.time()
    r_dev = get_rizky_device() # Look up RizkybySSD by UUID
    
    # 1. Root Linux OS Device Detection
    root_dev = ""
    try:
        findmnt = subprocess.check_output("findmnt -n -o SOURCE / 2>/dev/null", shell=True, text=True).strip()
        root_dev = re.sub(r'p?\d+$', '', findmnt.replace('/dev/', ''))
    except: pass

    # 2. Discover all physical block devices via lsblk
    raw_devs = []
    try:
        lsblk_raw = subprocess.check_output("lsblk -J -b -d -o NAME,MODEL,SIZE,ROTA,TRAN,TYPE,VENDOR,SERIAL 2>/dev/null", shell=True, text=True)
        data = json.loads(lsblk_raw)
        raw_devs = data.get('blockdevices', [])
    except: pass

    # 3. Read per-disk sectors from /proc/diskstats
    proc_stats = {}
    total_read_sectors = 0
    total_write_sectors = 0
    rizky_current_writes = 0

    try:
        with open('/proc/diskstats', 'r') as f:
            for line in f:
                parts = line.split()
                if len(parts) >= 14:
                    dname = parts[2]
                    total_read_sectors += int(parts[5])
                    total_write_sectors += int(parts[9])
                    proc_stats[dname] = {
                        'read_sectors': int(parts[5]),
                        'write_sectors': int(parts[9])
                    }
                    if r_dev and dname == r_dev:
                        rizky_current_writes = int(parts[9])
    except: pass

    # Calculate overall aggregate R/W rates
    dt_agg = now - last_disk['time']
    agg_r_rate = ((total_read_sectors - last_disk['read']) * 512) / dt_agg if dt_agg > 0 and last_disk['read'] > 0 else 0
    agg_w_rate = ((total_write_sectors - last_disk['write']) * 512) / dt_agg if dt_agg > 0 and last_disk['write'] > 0 else 0
    last_disk = {'time': now, 'read': total_read_sectors, 'write': total_write_sectors}

    # Calculate RizkybySSD session & lifetime TBW
    session_tbw = 0
    if rizky_initial_writes is None and rizky_current_writes > 0:
        rizky_initial_writes = rizky_current_writes
    if rizky_initial_writes is not None and rizky_current_writes > 0:
        session_tbw = (rizky_current_writes - rizky_initial_writes) * 512
        
    if rizky_current_writes > 0:
        current_lifetime_tbw = base_tbw_cached + (session_tbw / (1024 ** 4))
        remaining_tbw = max(0.0, 600.0 - current_lifetime_tbw)
        if now - last_save_time > 10:
            save_tbw(current_lifetime_tbw)
            last_save_time = now
    else:
        current_lifetime_tbw = 0.0
        remaining_tbw = 0.0

    # 4. Build Detailed Disks List with Priority Hierarchy
    disks_list = []
    for dev in raw_devs:
        dname = dev.get('name', '')
        if not dname or dev.get('type') != 'disk' or dname.startswith('zram') or dname.startswith('loop'):
            continue
        
        model = dev.get('model') or dev.get('vendor') or dname
        size_bytes = int(dev.get('size', 0))
        size_gb = round(size_bytes / (1024**3), 1)
        rota = dev.get('rota') # True for HDD, False for SSD
        tran = dev.get('tran') or ('nvme' if 'nvme' in dname else 'sata')
        
        # Real-time rates per disk
        r_sec = proc_stats.get(dname, {}).get('read_sectors', 0)
        w_sec = proc_stats.get(dname, {}).get('write_sectors', 0)
        
        last_d = last_disk_per_dev.get(dname, {'time': now, 'read': r_sec, 'write': w_sec})
        dt_d = now - last_d['time']
        r_rate_d = ((r_sec - last_d['read']) * 512) / dt_d if dt_d > 0 and last_d['read'] > 0 else 0
        w_rate_d = ((w_sec - last_d['write']) * 512) / dt_d if dt_d > 0 and last_d['write'] > 0 else 0
        last_disk_per_dev[dname] = {'time': now, 'read': r_sec, 'write': w_sec}
        
        # Priority Hierarchy
        # Priority 1: RizkybySSD (SanDisk / match UUID / r_dev / sda)
        # Priority 2: OS Linux Root Disk
        # Priority 3: Other disks sorted by mountpoint / name
        is_rizky = (r_dev and dname == r_dev) or ('RizkybySSD' in model or 'SanDisk' in model or dname == 'sda')
        is_root = (root_dev and dname == root_dev)
        
        priority = 3
        if is_rizky: priority = 1
        elif is_root: priority = 2

        rm = bool(dev.get('rm')) # Removable media flag
        is_usb = (tran == "usb")
        
        # Comprehensive Drive Classification
        if dname.startswith('mmcblk'):
            drive_type = "SD Card / MMC Storage"
            tbw_str = "Secure Digital (SD/eMMC) Flash"
            icon = "🎴"
            is_hdd_flag = False
        elif is_usb and (rm or size_gb <= 128 or "ProductCode" in model or "Flash" in model or "Thumb" in model) and not is_rizky:
            drive_type = "USB Flash Drive (Removable Flash)"
            tbw_str = "NAND Flash (USB Mass Storage)"
            icon = "🔌"
            is_hdd_flag = False
        elif is_rizky or (is_usb and (size_gb > 128 or "SSD" in model or "SanDisk" in model or "NVMe" in model)):
            drive_type = "Portable External SSD (USB/UASP)"
            tbw_str = "Portable Solid State Drive"
            icon = "⚡"
            is_hdd_flag = False
        elif tran == "nvme" or "nvme" in dname:
            drive_type = "PCIe NVMe Solid State Drive"
            tbw_str = "PCIe Gen4 NVMe Storage"
            icon = "⚡"
            is_hdd_flag = False
        elif rota and not is_usb:
            drive_type = "HDD (Mechanical Hard Drive)"
            tbw_str = "Rotational Platter (5400/7200 RPM)"
            icon = "💽"
            is_hdd_flag = True
        else:
            drive_type = "SATA Solid State Drive"
            tbw_str = "SATA SSD Storage"
            icon = "💾"
            is_hdd_flag = False

        s_tbw, s_temp, s_health, s_rem = get_disk_smart_info(dname, tran, is_rizky, current_lifetime_tbw, remaining_tbw)
        if is_rizky or (tran == "nvme" or "nvme" in dname):
            tbw_str = s_tbw
            temp_str = s_temp
            health_str = s_health
            remaining_str = s_rem
        else:
            temp_str = s_temp
            health_str = s_health
            remaining_str = "N/A"

        is_ssd = is_rizky or ("SSD" in drive_type) or ("NVMe" in drive_type)

        detail_lines = ['==================================================']
        detail_lines.append(f"💽 STORAGE DEVICE TELEMETRY: /dev/{dname}")
        detail_lines.append('==================================================')
        detail_lines.append(f"Device Model       : {model}")
        detail_lines.append(f"Capacity           : {size_gb} GB")
        detail_lines.append(f"Drive Category     : {drive_type}")
        detail_lines.append(f"Transport Interface: {tran.upper()}")
        detail_lines.append(f"SMART Health Status: {health_str}")
        detail_lines.append(f"Operating Temp     : {temp_str}")
        detail_lines.append(f"Total Bytes Written: {tbw_str}")
        if remaining_str != 'N/A':
            detail_lines.append(f"Remaining Endurance: {remaining_str}")

        try:
            ls1 = subprocess.check_output(f'lsblk -o NAME,LABEL,FSTYPE,SIZE,FSAVAIL,FSUSE%,MOUNTPOINT,UUID,PARTUUID /dev/{dname} 2>/dev/null', shell=True, text=True, timeout=0.3).strip()
            if ls1: detail_lines.append(f"\n=== 1. PARTITION STRUCTURE, FILE SYSTEMS & UUIDs ===\n{ls1}")
        except: pass

        try:
            ls2 = subprocess.check_output(f'lsblk -o NAME,PHY-SEC,LOG-SEC,SCHED,RQ-SIZE,RA,DISC-GRAN,DISC-MAX /dev/{dname} 2>/dev/null', shell=True, text=True, timeout=0.3).strip()
            if ls2: detail_lines.append(f"\n=== 2. KERNEL BLOCK QUEUE & I/O SCHEDULER SPECS ===\n{ls2}")
        except: pass

        try:
            df_out = subprocess.check_output(f'df -hT 2>/dev/null | grep "/dev/{dname}"', shell=True, text=True, timeout=0.3).strip()
            if df_out: detail_lines.append(f"\n=== 3. MOUNTED FILESYSTEM USAGE & AVAILABLE SPACE ===\n{df_out}")
        except: pass

        try:
            ud_out = subprocess.check_output(f'udevadm info --query=property --name=/dev/{dname} 2>/dev/null | grep -E "ID_MODEL|ID_SERIAL|ID_REVISION|ID_VENDOR|ID_USB|ID_BUS|ID_FS|ID_PATH|ID_PART_TABLE_TYPE"', shell=True, text=True, timeout=0.3).strip()
            if ud_out: detail_lines.append(f"\n=== 4. HARDWARE IDENTITY & BUS PROPERTIES ===\n{ud_out}")
        except: pass

        sys_attrs = []
        for attr in ['queue/rotational', 'queue/logical_block_size', 'queue/physical_block_size', 'queue/scheduler', 'queue/read_ahead_kb', 'device/vendor', 'device/model', 'device/rev']:
            path = f'/sys/block/{dname}/{attr}'
            if os.path.exists(path):
                try:
                    with open(path, 'r') as f:
                        val = f.read().strip()
                        sys_attrs.append(f"{attr.replace('/', '.')}: {val}")
                except: pass
        if sys_attrs:
            detail_lines.append('\n=== 5. SYSFS KERNEL DEVICE ATTRIBUTES ===\n' + '\n'.join(sys_attrs))

        detail_text = "\n".join(detail_lines)

        disks_list.append({
            'id': dname,
            'dev': dname,
            'icon': icon,
            'name': f"{model} ({size_gb} GB)",
            'label': f"{icon} {model} ({size_gb} GB)",
            'dev_path': f"/dev/{dname}",
            'model': model,
            'size': f"{size_gb} GB",
            'type': drive_type,
            'is_hdd': is_hdd_flag,
            'is_ssd': is_ssd,
            'transport': tran,
            'is_root': is_root,
            'priority': priority,
            'read_rate': r_rate_d,
            'write_rate': w_rate_d,
            'tbw_str': tbw_str,
            'remaining_str': remaining_str,
            'temp_str': temp_str,
            'health_str': health_str,
            'detail_text': detail_text
        })
        
    disks_list.sort(key=lambda x: (x['priority'], x['id']))

    return {
        'read_rate': agg_r_rate, 
        'write_rate': agg_w_rate,
        'disks': disks_list,
        'rizkybySSD': {
            'device': r_dev or "Not Found",
            'session_bytes': session_tbw,
            'lifetime_tbw_str': f"{current_lifetime_tbw:.3f} TB",
            'remaining_tbw_str': f"{remaining_tbw:.3f} TB"
        }
    }

def get_thermal_and_battery():
    temp = 0
    temps = []
    
    # 1. Check /sys/class/hwmon for coretemp / cpu sensors first
    try:
        for hwmon in glob.glob('/sys/class/hwmon/hwmon*'):
            name_file = os.path.join(hwmon, 'name')
            hw_name = ''
            if os.path.exists(name_file):
                with open(name_file, 'r') as f:
                    hw_name = f.read().strip().lower()
            
            if 'coretemp' in hw_name or 'cpu' in hw_name or 'k10temp' in hw_name or 'zenpower' in hw_name:
                for inp in glob.glob(os.path.join(hwmon, 'temp*_input')):
                    try:
                        with open(inp, 'r') as f:
                            t = int(f.read().strip()) // 1000
                            if 20 <= t <= 115:
                                temps.append(t)
                    except:
                        pass
    except:
        pass
        
    # 2. Check /sys/class/thermal/thermal_zone*
    if not temps:
        try:
            for tz_dir in glob.glob('/sys/class/thermal/thermal_zone*'):
                type_file = os.path.join(tz_dir, 'type')
                temp_file = os.path.join(tz_dir, 'temp')
                if os.path.exists(type_file) and os.path.exists(temp_file):
                    try:
                        with open(type_file, 'r') as ft:
                            tz_type = ft.read().strip().lower()
                        with open(temp_file, 'r') as f:
                            raw_t = int(f.read().strip())
                            t = raw_t // 1000 if raw_t > 2000 else raw_t
                        if t > 0 and 20 <= t <= 115:
                            if any(k in tz_type for k in ['pkg', 'core', 'cpu', 'sen', 'tcpu', 'x86']):
                                temps.append(t)
                    except:
                        pass
        except:
            pass

    if temps:
        temp = max(temps)
    else:
        temp = 45 # Reasonable fallback

    battery = 100
    try:
        for bat in glob.glob('/sys/class/power_supply/BAT*/capacity'):
            with open(bat, 'r') as f:
                battery = int(f.read().strip())
                break
    except:
        pass
        
    return temp, battery

hardware_details_cache = None

def get_hardware_details():
    global hardware_details_cache
    if hardware_details_cache is not None:
        return hardware_details_cache
        
    details = {
        'ram_type': [],
        'zram_info': '8.0 GB zstd Compressed RAM Swap Pool',
        'swap_info': 'Active ZRAM Memory Pool (Priority 100)',
        'ssd_model': 'SanDisk Portable SSD (931.5G) + NVMe (238.5G)',
        'network_type': 'Intel Wi-Fi + Realtek Gigabit Ethernet',
        'battery_tech': 'Li-ion Battery (SR Real Battery)'
    }

    # RAM Probing (dmidecode)
    try:
        cmd = 'echo "232390183" | sudo -S dmidecode -t memory 2>/dev/null'
        out = subprocess.check_output(cmd, shell=True, text=True)
        blocks = out.split('Memory Device')
        ram_list = []
        for i, block in enumerate(blocks[1:]):
            if 'Size: No Module Installed' in block: continue
            ram_size = ""
            ram_type = ""
            ram_speed = ""
            ram_mfg = ""
            for line in block.splitlines():
                line = line.strip()
                if line.startswith('Size:') and 'Detail' not in line:
                    ram_size = line.split(':', 1)[1].strip()
                elif line.startswith('Type:') and 'Detail' not in line:
                    ram_type = line.split(':', 1)[1].strip()
                elif line.startswith('Speed:') and 'Configured' not in line:
                    ram_speed = line.split(':', 1)[1].strip()
                elif line.startswith('Manufacturer:'):
                    mfg = line.split(':', 1)[1].strip()
                    if not mfg.startswith('0x'): ram_mfg = mfg
            if ram_size:
                label = f"Slot {i+1}: {ram_size} {ram_type} ({ram_speed})".strip()
                if ram_mfg: label += f" - {ram_mfg}"
                ram_list.append(label)
        if ram_list:
            details['ram_type'] = ram_list
        else:
            details['ram_type'] = ["Slot 1: 8 GB DDR4-3200 SODIMM", "Slot 2: 8 GB DDR4-3200 SODIMM"]
    except:
        details['ram_type'] = ["Slot 1: 8 GB DDR4-3200 SODIMM", "Slot 2: 8 GB DDR4-3200 SODIMM"]

    # SSD / Disk Probing
    try:
        cmd = 'lsblk -d -o NAME,MODEL,SIZE | grep -E "sda|sdb|nvme0n1"'
        out = subprocess.check_output(cmd, shell=True, text=True)
        lines = out.strip().split('\n')
        ssd_list = []
        for line in lines:
            parts = line.split()
            if len(parts) >= 3:
                dev = parts[0].upper()
                model = " ".join(parts[1:-1])
                size = parts[-1]
                ssd_list.append(f"{dev} ({size}): {model}")
        if ssd_list:
            details['ssd_model'] = "<br>".join(ssd_list)
    except:
        pass

    # Real-Time Network Probing (SSID, Signal, Frequency, Speed, Security, IP, Hardware)
    try:
        net_lines = []
        ssid = "Brambang 313"
        
        # Method 1: Active Connection Query
        try:
            cmd1 = "nmcli -t -f NAME,TYPE connection show --active 2>/dev/null"
            out1 = subprocess.check_output(cmd1, shell=True, text=True).strip()
            for line in out1.splitlines():
                if 'wireless' in line or 'wifi' in line:
                    parts = line.split(':')
                    if parts[0]:
                        ssid = parts[0]
                        break
        except: pass

        # Method 2: iwgetid fallback
        if not ssid:
            try:
                cmd2 = "iwgetid -r 2>/dev/null"
                fetched_ssid = subprocess.check_output(cmd2, shell=True, text=True).strip()
                if fetched_ssid: ssid = fetched_ssid
            except: pass

        if not ssid:
            ssid = "Brambang 313"

        # Method 3: Signal & Rate details
        signal = "100"
        rate = "270 Mbit/s"
        freq = "2437 MHz (2.4 GHz)"
        security = "WPA1 WPA2"
        try:
            cmd3 = "nmcli -t -f active,ssid,signal,freq,rate,security dev wifi 2>/dev/null | grep '^yes'"
            out3 = subprocess.check_output(cmd3, shell=True, text=True).strip()
            if out3:
                wparts = out3.split(':')
                if len(wparts) >= 6:
                    if wparts[1]: ssid = wparts[1]
                    if wparts[2]: signal = wparts[2]
                    if wparts[3]: freq = wparts[3]
                    if wparts[4]: rate = wparts[4]
                    if wparts[5]: security = wparts[5]
        except: pass

        net_lines.append("<strong>Connection Status:</strong> CONNECTED (Wi-Fi Wireless Network)")
        net_lines.append(f"<strong>Wi-Fi SSID:</strong> <span style='color:#38bdf8; font-weight:800; font-size:1.15em;'>{ssid}</span>")
        net_lines.append(f"<strong>Frequency & Security:</strong> {freq} | {security}")
        net_lines.append(f"<strong>Signal Quality & Speed:</strong> {signal}% Quality ({rate})")

        # IP Address Probing
        ip_addr_str = "wlo1: 192.168.1.60/24"
        try:
            ip_out = subprocess.check_output("ip -br a 2>/dev/null | grep -E 'UP|wlo1|wlan0|enp2s0'", shell=True, text=True).strip()
            ips = []
            for iline in ip_out.splitlines():
                parts = iline.split()
                if len(parts) >= 3 and parts[2] != 'N/A':
                    ips.append(f"{parts[0]}: {parts[2]}")
            if ips:
                ip_addr_str = ", ".join(ips)
        except: pass
        net_lines.append(f"<strong>IP Address:</strong> {ip_addr_str}")

        # Hardware Chipsets
        net_lines.append("<strong>Hardware Controller:</strong><br>Intel Corporation Alder Lake-P PCH CNVi WiFi<br>Realtek RTL8111 PCI Express Gigabit Ethernet")

        details['network_type'] = "<br>".join(net_lines)
    except:
        details['network_type'] = "<strong>Connection Status:</strong> CONNECTED (Wi-Fi Wireless)<br><strong>Wi-Fi SSID:</strong> <span style='color:#38bdf8; font-weight:800;'>Brambang 313</span><br><strong>IP Address:</strong> wlo1: 192.168.1.60/24"

    # Real-Time Battery Probing
    try:
        batt_info = []
        for bat in glob.glob('/sys/class/power_supply/BAT*'):
            model = ""
            status = ""
            capacity = ""
            if os.path.exists(f'{bat}/model_name'):
                with open(f'{bat}/model_name', 'r') as f: model = f.read().strip()
            if os.path.exists(f'{bat}/status'):
                with open(f'{bat}/status', 'r') as f: status = f.read().strip()
            if os.path.exists(f'{bat}/capacity'):
                with open(f'{bat}/capacity', 'r') as f: capacity = f.read().strip()
            
            line = f"<strong>Model:</strong> {model or 'Laptop Battery'}"
            if capacity: line += f" ({capacity}% Charged)"
            if status: line += f" - Status: {status}"
            batt_info.append(line)
            
        if batt_info:
            details['battery_tech'] = "<br>".join(batt_info)
        else:
            details['battery_tech'] = "AC Power Adapter (Plugged In / Desktop)"
    except:
        pass
    # RAW DATA EXTRACTION FOR COMPLEX VIEW
    try:
        import subprocess
        details['raw_cpu'] = subprocess.check_output('lscpu', shell=True, text=True).strip()
    except: details['raw_cpu'] = "No detailed CPU data available."
    
    try:
        # Get detailed info for all VGAs
        details['raw_gpu'] = subprocess.check_output('lspci -v -d ::0300', shell=True, text=True).strip()
    except: details['raw_gpu'] = "No detailed GPU data available."

    try:
        details['raw_ram'] = subprocess.check_output('echo "232390183" | sudo -S dmidecode -t memory 2>/dev/null', shell=True, text=True).strip()
    except: details['raw_ram'] = "No detailed RAM data available."

    try:
        lsblk_out = subprocess.check_output('lsblk -o NAME,LABEL,FSTYPE,SIZE,MOUNTPOINT,UUID,PARTUUID,MODEL,SERIAL', shell=True, text=True).strip()
        df_out = subprocess.check_output('df -hT | grep -v tmpfs | grep -v devtmpfs', shell=True, text=True).strip()
        udev_out = subprocess.check_output('udevadm info --query=property --name=/dev/sda | grep -E "ID_MODEL|ID_SERIAL|ID_REVISION|ID_PART_TABLE_TYPE|ID_VENDOR"', shell=True, text=True).strip()
        
        raw_disk = "=== Detailed Partitions & UUIDs ===\\n" + lsblk_out + "\\n\\n=== Partition Usage ===\\n" + df_out + "\\n\\n=== RizkybySSD Hardware Identity ===\\n" + udev_out
        
        details['raw_disk'] = raw_disk
    except: details['raw_disk'] = "No detailed Disk data available."

    try:
        details['raw_net'] = subprocess.check_output('ip a', shell=True, text=True).strip()
    except: details['raw_net'] = "No detailed Network data available."
    
    hardware_details_cache = details
    return details



def get_top_processes():
    try:
        cpu_cmd = "ps -eo comm,%cpu,cputime --sort=-%cpu | tail -n +2 | head -n 40"
        cpu_out = subprocess.check_output(cpu_cmd, shell=True, text=True)
        top_cpu = []
        for line in cpu_out.strip().split('\n'):
            parts = line.split()
            if len(parts) >= 3:
                name = parts[0][:20]
                cpu_pct = parts[1] + "%"
                time_str = parts[2]
                val_str = f"{cpu_pct} ({time_str})" if time_str and time_str != "00:00:00" else cpu_pct
                top_cpu.append({"name": name, "val": val_str})

        mem_cmd = "ps -eo comm,%mem,rss --sort=-%mem | tail -n +2 | head -n 40"
        mem_out = subprocess.check_output(mem_cmd, shell=True, text=True)
        top_mem = []
        for line in mem_out.strip().split('\n'):
            parts = line.split()
            if len(parts) >= 3:
                name = parts[0][:20]
                mem_pct = parts[1] + "%"
                try:
                    rss_kb = int(parts[2])
                    if rss_kb >= 1024 * 1024:
                        size_str = f"{rss_kb / 1048576:.1f} GB"
                    else:
                        size_str = f"{rss_kb / 1024:.1f} MB"
                    val_str = f"{size_str} ({mem_pct})"
                except:
                    val_str = mem_pct
                top_mem.append({"name": name, "val": val_str})
        
        return top_cpu, top_mem
    except:
        return [], []

get_cpu_usage()
get_network_stats()
get_disk_stats()

class APIHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/api/copy':
            content_length = int(self.headers.get('Content-Length', 0))
            post_data = self.rfile.read(content_length).decode('utf-8')
            
            copied = False
            import subprocess
            clipboard_cmds = [
                ['wl-copy'],
                ['xclip', '-selection', 'clipboard'],
                ['xsel', '--clipboard', '--input'],
                ['pbcopy'],
                ['clip.exe']
            ]
            for cmd in clipboard_cmds:
                try:
                    res = subprocess.run(cmd, input=post_data.encode('utf-8'), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=1.0)
                    if res.returncode == 0:
                        copied = True
                        break
                except (FileNotFoundError, subprocess.SubprocessError):
                    continue
            
            if copied:
                self.send_response(200)
            else:
                self.send_response(500)
                
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            return
        elif self.path == '/api/quit':
            content_length = int(self.headers.get('Content-Length', 0))
            post_data = self.rfile.read(content_length).decode('utf-8') if content_length > 0 else '{}'
            try:
                import json
                data = json.loads(post_data)
                if 'win_count' in data:
                    config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'config.json')
                    with open(config_path, 'w') as f:
                        json.dump({'window_count': int(data['win_count'])}, f)
            except: pass
            self.send_response(200)
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            import threading, time
            threading.Thread(target=lambda: (time.sleep(0.2), os._exit(0))).start()
            return
        elif self.path.startswith('/api/close_window'):
            from urllib.parse import urlparse, parse_qs
            parsed_url = urlparse(self.path)
            query_params = parse_qs(parsed_url.query)
            win_id = query_params.get('win', ['1'])[0]
            if close_window_callback:
                close_window_callback(win_id)
        elif self.path.startswith('/api/on_top'):
            from urllib.parse import urlparse, parse_qs
            parsed_url = urlparse(self.path)
            query_params = parse_qs(parsed_url.query)
            win_id = query_params.get('win', ['1'])[0]
            if toggle_on_top_callback:
                toggle_on_top_callback(win_id)
            self.send_response(200)
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            return
        elif self.path == '/api/duplicate':
            if open_window_callback:
                open_window_callback()
            self.send_response(200)
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            return
        elif self.path.startswith('/api/config'):
            content_length = int(self.headers.get('Content-Length', 0))
            post_data = self.rfile.read(content_length).decode('utf-8') if content_length > 0 else '{}'
            try:
                import json
                payload = json.loads(post_data)
                config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'config.json')
                current_cfg = {}
                if os.path.exists(config_path):
                    try:
                        with open(config_path, 'r') as f:
                            current_cfg = json.load(f)
                    except: pass
                
                if 'window_id' in payload and 'settings' in payload:
                    win_id = str(payload['window_id'])
                    win_settings = payload['settings']
                    per_win = current_cfg.get('per_window_settings', {})
                    if win_id not in per_win:
                        per_win[win_id] = {}
                    per_win[win_id].update(win_settings)
                    current_cfg['per_window_settings'] = per_win
                else:
                    current_cfg.update(payload)
                    
                with open(config_path, 'w') as f:
                    json.dump(current_cfg, f, indent=2)
            except: pass
            self.send_response(200)
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            return

    def do_GET(self):
        if self.path.startswith('/api/config'):
            self.send_response(HTTPStatus.OK)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            from urllib.parse import urlparse, parse_qs
            parsed_url = urlparse(self.path)
            query_params = parse_qs(parsed_url.query)
            win_id = query_params.get('win', ['1'])[0]
            
            config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'config.json')
            res_data = {}
            if os.path.exists(config_path):
                try:
                    with open(config_path, 'r') as f:
                        cfg = json.load(f)
                        per_win = cfg.get('per_window_settings', {})
                        res_data = per_win.get(win_id, {})
                except: pass
            self.wfile.write(json.dumps(res_data).encode('utf-8'))
            return
        elif self.path == '/api/stats':
            self.send_response(HTTPStatus.OK)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            mem = get_mem_info()
            zram, disk_swap = get_swaps()
            cpu_usages = get_cpu_usage()
            cpu_freqs = get_cpu_freqs()
            gpu_freq = get_gpu_freq()
            net = get_network_stats()
            disk = get_disk_stats()
            temp, batt = get_thermal_and_battery()
            top_cpu, top_mem = get_top_processes()
            top_net = nethogs_top if nethogs_top else get_fallback_net_processes()
            
            mem_total = mem.get('MemTotal', 0)
            mem_free = mem.get('MemAvailable', mem.get('MemFree', 0))
            mem_used = mem_total - mem_free
            
            data = {
                'hardware': {
                    'cpu_model': cpu_model_name,
                    'gpu_model': gpu_model_name
                },
                'ram': {'total': mem_total, 'used': mem_used, 'free': mem_free},
                'zram': zram,
                'swap': disk_swap,
                'cpu': {
                    'total_usage': cpu_usages.get('cpu', 0.0),
                    'cores': [cpu_usages.get(f'cpu{i}', 0.0) for i in range(12)],
                    'freqs': cpu_freqs
                },
                'gpu': {'freq': gpu_freq, 'usage': gpu_usage},
                'network': net,
                'disk': disk,
                'sensors': {'temp': temp, 'battery': batt},
                'processes': {
                    'cpu': top_cpu, 
                    'mem': top_mem,
                    'net': top_net,
                    'gpu': gpu_processes
                },
                'details': get_hardware_details()
            }
            
            self.wfile.write(json.dumps(data).encode('utf-8'))

        elif self.path.startswith('/api/disk-detail'):
            from urllib.parse import urlparse, parse_qs
            self.send_response(HTTPStatus.OK)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()

            params = parse_qs(urlparse(self.path).query)
            dev = params.get('dev', ['sda'])[0]
            dev = ''.join(c for c in dev if c.isalnum())

            sections = []

            # 1. Block Device & Partition Layout (Safe, Non-Root)
            try:
                lsblk_out = subprocess.check_output(
                    f'lsblk -o NAME,LABEL,FSTYPE,SIZE,MOUNTPOINT,MODEL,SERIAL,TRAN,TYPE /dev/{dev} 2>/dev/null',
                    shell=True, text=True, timeout=0.5
                ).strip()
                if lsblk_out:
                    sections.append(f"=== Block Device & Partition Structure: /dev/{dev} ===\n{lsblk_out}")
            except:
                pass

            # 2. Hardware Identity & Bus Properties (Safe, Non-Root sysfs/udev)
            try:
                udev_out = subprocess.check_output(
                    f'udevadm info --query=property --name=/dev/{dev} 2>/dev/null | grep -E "ID_MODEL|ID_SERIAL|ID_REVISION|ID_VENDOR|ID_USB|ID_BUS|ID_FS|ID_PATH|ID_TYPE"',
                    shell=True, text=True, timeout=0.5
                ).strip()
                if udev_out:
                    sections.append(f"=== Hardware Identity & Bus Properties ===\n{udev_out}")
            except:
                pass

            # 3. Mounted Filesystem Space (Safe, Non-Root)
            try:
                df_out = subprocess.check_output(
                    f'df -hT | head -1; df -hT | grep "/dev/{dev}"',
                    shell=True, text=True, timeout=0.5
                ).strip()
                if df_out:
                    sections.append(f"=== Filesystem Usage & Mount Points ===\n{df_out}")
            except:
                pass

            # 4. Storage Controller & Queue Attributes (Safe sysfs)
            try:
                sys_attrs = []
                for attr in ['queue/rotational', 'queue/logical_block_size', 'queue/scheduler', 'device/vendor', 'device/model']:
                    path = f'/sys/block/{dev}/{attr}'
                    if os.path.exists(path):
                        with open(path, 'r') as f:
                            val = f.read().strip()
                            sys_attrs.append(f"{attr}: {val}")
                if sys_attrs:
                    sections.append(f"=== Kernel Sysfs Block Properties ===\n" + "\n".join(sys_attrs))
            except:
                pass

            result = '\n\n'.join(sections) if sections else f"No detailed disk data available for /dev/{dev}"
            self.wfile.write(json.dumps({'detail': result}).encode('utf-8'))

        else:
            super().do_GET()

if __name__ == "__main__":
    t_net = threading.Thread(target=update_nethogs_loop, daemon=True)
    t_net.start()
    
    t_gpu = threading.Thread(target=update_gpu_loop, daemon=True)
    t_gpu.start()
    
    print(f"Server berjalan di port {PORT}...")
    with socketserver.TCPServer(("", PORT), APIHandler) as httpd:
        httpd.serve_forever()
