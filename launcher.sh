#!/bin/bash
# Matikan instance yang sedang berjalan jika ada
pkill -f "python3 app.py"
pkill -f "python3 server.py"

cd /run/media/rizkybayuu_/RizkybySSD/.ai/RizkybyMONITOR

# Jika folder .venv belum ada, buat baru menggunakan uv
if [ ! -d ".venv" ]; then
    echo "Creating ultra-fast virtual environment using uv..."
    uv venv --system-site-packages .venv
fi

# Pastikan dependensi selalu terinstal (hanya makan waktu sangat singkat dengan uv)
uv pip install pywebview psutil requests

# Jalankan aplikasi GUI native menggunakan uv run!
uv run app.py
