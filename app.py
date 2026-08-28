import webview
import threading
from server import APIHandler, set_open_window_callback, set_close_window_callback, set_toggle_on_top_callback
import socketserver
import os, time, sys, json

PORT = 8080
active_windows = []

def start_server():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    socketserver.TCPServer.allow_reuse_address = True
    server = None
    for attempt in range(10):
        try:
            server = socketserver.TCPServer(("", PORT), APIHandler)
            break
        except OSError:
            time.sleep(0.5)
    if server:
        server.serve_forever()

def save_window_count(count):
    try:
        config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'config.json')
        cfg = {}
        if os.path.exists(config_path):
            try:
                with open(config_path, 'r') as f:
                    cfg = json.load(f)
            except: pass
        cfg['window_count'] = count
        with open(config_path, 'w') as f:
            json.dump(cfg, f, indent=2)
    except: pass

def save_window_geometry(win_id, width=None, height=None, x=None, y=None):
    try:
        config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'config.json')
        cfg = {}
        if os.path.exists(config_path):
            try:
                with open(config_path, 'r') as f:
                    cfg = json.load(f)
            except: pass
        
        states = cfg.get('window_states', {})
        s_key = str(win_id)
        if s_key not in states:
            states[s_key] = {}
        
        if width is not None and height is not None:
            states[s_key]['width'] = int(width)
            states[s_key]['height'] = int(height)
        if x is not None and y is not None:
            states[s_key]['x'] = int(x)
            states[s_key]['y'] = int(y)
            
        cfg['window_states'] = states
        with open(config_path, 'w') as f:
            json.dump(cfg, f, indent=2)
    except: pass

def update_window_geometry(win_id, w):
    try:
        cur_w = getattr(w, 'width', None)
        cur_h = getattr(w, 'height', None)
        cur_x = getattr(w, 'x', None)
        cur_y = getattr(w, 'y', None)
        save_window_geometry(win_id, cur_w, cur_h, cur_x, cur_y)
    except: pass

def on_single_window_closed(w, win_id):
    global active_windows
    update_window_geometry(win_id, w)
    if w in active_windows:
        active_windows.remove(w)
    count = max(1, len(active_windows))
    save_window_count(count)

def get_window_states():
    try:
        config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'config.json')
        if os.path.exists(config_path):
            with open(config_path, 'r') as f:
                cfg = json.load(f)
                return cfg.get('window_states', {})
    except: pass
    return {}

def bind_window_events(w, win_id):
    def on_moved(*args):
        try:
            cur_w = getattr(w, 'width', None)
            cur_h = getattr(w, 'height', None)
            cur_x = getattr(w, 'x', None)
            cur_y = getattr(w, 'y', None)
            if (cur_x is None or cur_y is None) and len(args) >= 2:
                cur_x, cur_y = args[0], args[1]
            if cur_x is not None and cur_y is not None:
                save_window_geometry(win_id, cur_w, cur_h, cur_x, cur_y)
        except Exception as e: pass

    def on_resized(*args):
        try:
            cur_w = getattr(w, 'width', None)
            cur_h = getattr(w, 'height', None)
            cur_x = getattr(w, 'x', None)
            cur_y = getattr(w, 'y', None)
            if (cur_w is None or cur_h is None) and len(args) >= 2:
                cur_w, cur_h = args[0], args[1]
            if cur_w is not None and cur_h is not None:
                save_window_geometry(win_id, cur_w, cur_h, cur_x, cur_y)
        except Exception as e: pass

    w.events.moved += on_moved
    w.events.resized += on_resized
    w.events.closed += (lambda: on_single_window_closed(w, win_id))

def open_duplicate_window():
    global active_windows
    new_id = len(active_windows) + 1
    
    window_states = get_window_states()
    win_state = window_states.get(str(new_id), {})
    w_width = win_state.get('width', 1100)
    w_height = win_state.get('height', 800)
    w_x = win_state.get('x', None)
    w_y = win_state.get('y', None)

    title = f'RizkybyMONITOR {new_id}'
    kwargs = {
        'title': title,
        'url': f'http://localhost:8080/?win={new_id}',
        'width': w_width,
        'height': w_height,
        'frameless': True,
        'background_color': '#0f172a'
    }
    if w_x is not None and w_y is not None:
        kwargs['x'] = int(w_x)
        kwargs['y'] = int(w_y)

    w = webview.create_window(**kwargs)
    bind_window_events(w, new_id)
    
    if w_x is not None and w_y is not None:
        def _force_move_pos(win_obj, target_x, target_y):
            time.sleep(0.3)
            try:
                win_obj.move(int(target_x), int(target_y))
            except: pass
            try:
                import gi
                gi.require_version('Gtk', '3.0')
                from gi.repository import GLib
                if hasattr(win_obj, 'gui') and hasattr(win_obj.gui, 'window'):
                    GLib.idle_add(win_obj.gui.window.move, int(target_x), int(target_y))
            except: pass
        threading.Thread(target=_force_move_pos, args=(w, w_x, w_y), daemon=True).start()

    active_windows.append(w)
    save_window_count(len(active_windows))

def close_specific_window(win_id):
    global active_windows
    target_win = None
    win_str = str(win_id)
    for w in active_windows:
        if f'RizkybyMONITOR {win_str}' in getattr(w, 'title', '') or (len(active_windows) == 1 and win_str == '1'):
            target_win = w
            break
    if not target_win:
        try:
            idx = int(win_str) - 1
            if 0 <= idx < len(active_windows):
                target_win = active_windows[idx]
        except: pass
        
    if target_win:
        try:
            target_win.destroy()
        except: pass

def toggle_on_top(win_id):
    global active_windows
    win_str = str(win_id)
    target_win = None
    for w in active_windows:
        title = getattr(w, 'title', '')
        if f'MONITOR {win_str}' in title or f'RizkybyMONITOR {win_str}' in title or (len(active_windows) == 1 and win_str == '1'):
            target_win = w
            break
    if not target_win:
        try:
            idx = int(win_str) - 1
            if 0 <= idx < len(active_windows):
                target_win = active_windows[idx]
        except: pass
        
    if not target_win and len(active_windows) > 0:
        target_win = active_windows[0]

    if target_win:
        cur_status = getattr(target_win, '_is_pinned_on_top', False)
        new_status = not cur_status
        setattr(target_win, '_is_pinned_on_top', new_status)

        def _apply_on_top():
            # Method A: PyWebView property
            try:
                target_win.on_top = new_status
            except: pass

            # Method B: Direct GTK Window set_keep_above lookup
            try:
                gtk_win = None
                if hasattr(target_win, 'gui') and hasattr(target_win.gui, 'window'):
                    gtk_win = target_win.gui.window
                elif hasattr(target_win, 'native_window'):
                    gtk_win = target_win.native_window
                elif hasattr(target_win, '_window'):
                    gtk_win = target_win._window

                if gtk_win and hasattr(gtk_win, 'set_keep_above'):
                    gtk_win.set_keep_above(new_status)
            except: pass

            # Method C: PyWebView GTK Platform lookup
            try:
                import pywebview.platforms.gtk as gtk_platform
                if hasattr(gtk_platform, 'windows') and target_win.uid in gtk_platform.windows:
                    v = gtk_platform.windows[target_win.uid]
                    if hasattr(v, 'window') and hasattr(v.window, 'set_keep_above'):
                        v.window.set_keep_above(new_status)
            except: pass

            # Method D: Multi-command X11 / DE tool fallback (wmctrl & xdotool)
            try:
                import subprocess
                win_title = getattr(target_win, 'title', 'RizkybyMONITOR')
                action = "add" if new_status else "remove"
                subprocess.run(["wmctrl", "-r", win_title, "-b", f"{action},above"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                subprocess.run(["wmctrl", "-r", ":ACTIVE:", "-b", f"{action},above"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                if new_status:
                    subprocess.run(["xdotool", "search", "--name", win_title, "windowraise"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except: pass
            return False

        try:
            import gi
            gi.require_version('Gtk', '3.0')
            from gi.repository import GLib
            GLib.idle_add(_apply_on_top)
            GLib.timeout_add(150, _apply_on_top)
        except:
            _apply_on_top()

        return new_status
    return False

if __name__ == '__main__':
    from server import update_gpu_loop, update_nethogs_loop, update_net_loop
    t_net = threading.Thread(target=update_nethogs_loop, daemon=True)
    t_net.start()
    t_gpu = threading.Thread(target=update_gpu_loop, daemon=True)
    t_gpu.start()
    t_net_loop = threading.Thread(target=update_net_loop, daemon=True)
    t_net_loop.start()

    t = threading.Thread(target=start_server)
    t.daemon = True
    t.start()

    set_open_window_callback(open_duplicate_window)
    set_close_window_callback(close_specific_window)
    set_toggle_on_top_callback(toggle_on_top)

    saved_count = 1
    window_states = get_window_states()
    config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'config.json')
    if os.path.exists(config_path):
        try:
            with open(config_path, 'r') as f:
                cfg = json.load(f)
                saved_count = cfg.get('window_count', 1)
        except: pass

    num_windows = saved_count
    for arg in sys.argv[1:]:
        if arg.isdigit():
            num_windows = max(1, int(arg))
        elif arg.startswith('--windows='):
            try:
                num_windows = max(1, int(arg.split('=')[1]))
            except: pass

    for i in range(1, num_windows + 1):
        win_state = window_states.get(str(i), {})
        w_width = win_state.get('width', 1100)
        w_height = win_state.get('height', 800)
        w_x = win_state.get('x', None)
        w_y = win_state.get('y', None)

        title = f'RizkybyMONITOR {i}'
        kwargs = {
            'title': title,
            'url': f'http://localhost:8080/?win={i}',
            'width': w_width,
            'height': w_height,
            'frameless': True,
            'background_color': '#0f172a'
        }
        if w_x is not None and w_y is not None:
            kwargs['x'] = int(w_x)
            kwargs['y'] = int(w_y)

        w = webview.create_window(**kwargs)
        bind_window_events(w, i)

        if w_x is not None and w_y is not None:
            def _force_move_pos_main(win_obj, target_x, target_y):
                time.sleep(0.3)
                try:
                    win_obj.move(int(target_x), int(target_y))
                except: pass
                try:
                    import gi
                    gi.require_version('Gtk', '3.0')
                    from gi.repository import GLib
                    if hasattr(win_obj, 'gui') and hasattr(win_obj.gui, 'window'):
                        GLib.idle_add(win_obj.gui.window.move, int(target_x), int(target_y))
                except: pass
            threading.Thread(target=_force_move_pos_main, args=(w, w_x, w_y), daemon=True).start()

        active_windows.append(w)

    webview.start()
