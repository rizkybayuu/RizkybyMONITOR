#!/bin/bash
# Matikan instance yang sedang berjalan jika ada
pkill -f "python3 app.py"
pkill -f "python3 server.py"

cd /run/media/rizkybayuu_/RizkybySSD/.ai/RizkybyMONITOR

# Jalankan aplikasi GUI native!
./venv/bin/python3 app.py
