@echo off
setlocal EnableDelayedExpansion

net session >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo [INFO] Administrator privileges required - requesting elevation...
    powershell -NoProfile -Command "Start-Process cmd.exe -ArgumentList '/c \"\"%~f0\" %*\"' -WorkingDirectory '%~dp0' -Verb RunAs"
    exit /b
)

if not defined RZKMON_KEEP_OPEN (
    set "RZKMON_KEEP_OPEN=1"
    cmd /k call "%~f0" %*
    exit /b
)

goto :main_start

:ClassifyPmFailure
set "PM_REASON=Unrecognized error - check log at %~1"
findstr /i /c:"could not resolve" /c:"unable to connect" /c:"network" "%~1" >nul 2>nul && set "PM_REASON=No internet connection / repository unreachable."
findstr /i /c:"access is denied" /c:"administrator" "%~1" >nul 2>nul && set "PM_REASON=Needs elevated/administrator privileges."
findstr /i /c:"no space" /c:"disk full" "%~1" >nul 2>nul && set "PM_REASON=Disk is full - free up space and retry."
findstr /i /c:"not found" /c:"no package" /c:"unable to locate" "%~1" >nul 2>nul && set "PM_REASON=Package name not found in this package manager's source."
exit /b

:main_start
cd /d "%~dp0"

REM 1. Tutup aplikasi lama jika sedang berjalan agar file tidak terkunci
taskkill /f /im rizkybymonitor_windows.exe >nul 2>&1
taskkill /f /im rzkmon_sensor.exe >nul 2>&1

REM 2. Otomatis izinkan folder ini di Windows Defender secara relatif (tanpa hardcoded path)
powershell -NoProfile -Command "Add-MpPreference -ExclusionPath '%~dp0' -ErrorAction SilentlyContinue"

echo =======================================================
echo    RizkybyMONITOR v1.1 - Windows 1-Click Builder
echo =======================================================
echo.

REM =======================================================================
REM [PRIORITY 1] Check Computer Specs & System Drive
REM =======================================================================
set "SYS_ARCH=x64"
if "%PROCESSOR_ARCHITECTURE%"=="x86" (
    if not defined PROCESSOR_ARCHITEW6432 set "SYS_ARCH=x86"
)
if "%PROCESSOR_ARCHITECTURE%"=="ARM64" set "SYS_ARCH=ARM64"

set "TARGET_DRIVE=%SystemDrive%"
if "%TARGET_DRIVE%"=="" set "TARGET_DRIVE=C:"

echo [INFO] System Architecture : %SYS_ARCH%
echo [INFO] System Drive        : %TARGET_DRIVE%
echo.

REM =======================================================================
REM [PRIORITY 2] Search for a Compiler Across Active Drives Only
REM =======================================================================
set "COMPILER_TYPE="
set "COMPILER_BIN="
set "VCVARS_BAT="

echo [INFO] Detecting active drives on this system...

REM Dapatkan daftar huruf drive yang benar-benar terpasang (misal: C: D: E:)
set "ACTIVE_DRIVES="

REM 1. Coba fsutil fsinfo drives (Sangat cepat 0 ms, bawaan semua Windows NT, tanpa WMIC/PowerShell)
for /f "tokens=1*" %%a in ('fsutil fsinfo drives 2^>nul') do (
    for %%d in (%%b) do (
        set "drv=%%d"
        set "drv=!drv:\=!"
        if not "!drv!"=="" (
            if exist "!drv!\" set "ACTIVE_DRIVES=!ACTIVE_DRIVES! !drv!"
        )
    )
)

REM 2. Fallback jika fsutil dibatasi: Cek langsung huruf drive via native CMD (100% Tahan Banting, 0 Error)
if "%ACTIVE_DRIVES%"=="" (
    for %%d in (C D E F G H I J K L M N O P Q R S T U V W X Y Z A B) do (
        if exist "%%d:\" set "ACTIVE_DRIVES=!ACTIVE_DRIVES! %%d:"
    )
)

echo [INFO] Active drives found : %ACTIVE_DRIVES%
echo [INFO] Scanning for available compilers...

REM --- 1. Cek MSVC via vswhere ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -prerelease -property installationPath 2^>nul`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" (
            set "VCVARS_BAT=%%i\VC\Auxiliary\Build\vcvars64.bat"
            set "COMPILER_TYPE=MSVC"
        )
    )
)

REM --- 2. Cek System PATH ---
if "!COMPILER_TYPE!"=="" (
    for %%c in (g++.exe clang++.exe) do (
        if "!COMPILER_TYPE!"=="" (
            where %%c >nul 2>nul
            if !ERRORLEVEL! equ 0 (
                %%c --version >nul 2>nul
                if !ERRORLEVEL! equ 0 (
                    set "COMPILER_BIN=%%c"
                    set "COMPILER_TYPE=GCC"
                )
            )
        )
    )
)

REM --- 3. Fast-Path Check (Hanya di Active Drives) ---
if "!COMPILER_TYPE!"=="" (
    for %%d in (%ACTIVE_DRIVES%) do (
        if "!COMPILER_TYPE!"=="" (
            for %%p in (
                "Program Files\CodeBlocks\MinGW\bin\g++.exe"
                "Program Files (x86)\CodeBlocks\MinGW\bin\g++.exe"
                "Program Files\LLVM\bin\clang++.exe"
                "msys64\ucrt64\bin\g++.exe"
                "msys64\mingw64\bin\g++.exe"
                "msys64\clang64\bin\g++.exe"
                "mingw64\bin\g++.exe"
                "mingw32\bin\g++.exe"
                "MinGW\bin\g++.exe"
                "TDM-GCC-64\bin\g++.exe"
                "Strawberry\c\bin\g++.exe"
                "w64devkit\bin\g++.exe"
                "tools\w64devkit\bin\g++.exe"
            ) do (
                if "!COMPILER_TYPE!"=="" (
                    if exist "%%d\%%~p" (
                        set "COMPILER_BIN=%%d\%%~p"
                        set "COMPILER_TYPE=GCC"
                    )
                )
            )

            REM Cek Subfolder 1 Level - WinLibs / Qt Tools
            if "!COMPILER_TYPE!"=="" (
                for /d %%w in ("%%d\winlibs*" "%%d\Qt\Tools\mingw*") do (
                    if exist "%%w\bin\g++.exe" if "!COMPILER_TYPE!"=="" (
                        set "COMPILER_BIN=%%w\bin\g++.exe"
                        set "COMPILER_TYPE=GCC"
                    )
                )
            )
        )
    )
)

REM Jika compiler sudah ditemukan di Fast-Path / MSVC / PATH, langsung lompat ke CompilerFound
if not "!COMPILER_BIN!"=="" goto CompilerFound
if "!COMPILER_TYPE!"=="MSVC" goto CompilerFound

REM --- 3.5 Package Manager Tier (winget / choco / scoop) ---
if "!COMPILER_TYPE!"=="" (
    echo -------------------------------------------------------
    echo    [WARNING] No compiler found via PATH or known install locations.
    echo -------------------------------------------------------
    set /p CONFIRM_INSTALL="[PROMPT] Try installing a compiler automatically via winget/choco/scoop? (y/N): "
    if /i "!CONFIRM_INSTALL!"=="y" (
        set "PKG_MGR="
        where winget >nul 2>nul && set "PKG_MGR=winget"
        if "!PKG_MGR!"=="" (where choco >nul 2>nul && set "PKG_MGR=choco")
        if "!PKG_MGR!"=="" (where scoop >nul 2>nul && set "PKG_MGR=scoop")

        if "!PKG_MGR!"=="" (
            echo [SKIP] No package manager ^(winget/choco/scoop^) detected. Falling back to scan/download.
        ) else (
            echo [INFO] Detected package manager: !PKG_MGR!
            set "PM_LOG=%TEMP%\rzkmon_pm_install.log"
            if "!PKG_MGR!"=="winget" (
                echo [TRY] winget install --id BrechtSanders.WinLibs.POSIX.UCRT
                winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e --silent --accept-source-agreements --accept-package-agreements >"!PM_LOG!" 2>&1
            ) else if "!PKG_MGR!"=="choco" (
                echo [TRY] choco install mingw -y
                choco install mingw -y >"!PM_LOG!" 2>&1
            ) else (
                scoop bucket add extras >nul 2>nul
                echo [TRY] scoop install gcc
                scoop install gcc >"!PM_LOG!" 2>&1
            )

            if !ERRORLEVEL! equ 0 (
                echo [OK] !PKG_MGR! reported success. Re-checking PATH...
                where g++.exe >nul 2>nul && (set "COMPILER_BIN=g++.exe" & set "COMPILER_TYPE=GCC")
                del /f /q "!PM_LOG!" >nul 2>nul
            ) else (
                call :ClassifyPmFailure "!PM_LOG!"
                echo [FAILED] !PKG_MGR! install -^> !PM_REASON!
                del /f /q "!PM_LOG!" >nul 2>nul
            )
        )
    ) else (
        echo [SKIP] User declined automatic package-manager install.
    )
)

if not "!COMPILER_TYPE!"=="" goto CompilerFound

REM --- 4. Deep-Scan Rekursif Brutal (Hanya Menyapu Drive Aktif) ---
echo [INFO] Fast-path/package-manager missed. Executing DEEP RECURSIVE SCAN on active drives...
for /f "usebackq delims=" %%f in (`powershell -NoProfile -Command "Get-PSDrive -PSProvider FileSystem | Where-Object { $_.Used -gt 0 -or $_.Free -gt 0 } | ForEach-Object { Get-ChildItem -Path $_.Root -Filter 'g++.exe' -Recurse -ErrorAction SilentlyContinue } | Select-Object -First 1 -ExpandProperty FullName"`) do (
    if exist "%%f" (
        set "COMPILER_BIN=%%f"
        set "COMPILER_TYPE=GCC"
    )
)

if not "!COMPILER_TYPE!"=="" goto CompilerFound

REM =======================================================================
REM [PRIORITY 3] If No Compiler Is Found, Download the Smallest Portable One
REM =======================================================================
echo [INFO] No compiler found on active drives: %ACTIVE_DRIVES%
echo [INFO] Resolving latest portable compiler release ^(w64devkit^) for %TARGET_DRIVE%...
if not exist "tools" mkdir "tools"

set "DL_FILE=%TARGET_DRIVE%\w64devkit_dl.zip"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ProgressPreference = 'SilentlyContinue'; [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; try { $rel = Invoke-RestMethod -Uri 'https://api.github.com/repos/skeeto/w64devkit/releases/latest' -Headers @{'User-Agent'='rizkybymonitor-builder'}; $asset = $rel.assets | Where-Object { $_.name -like 'w64devkit-*.zip' } | Select-Object -First 1; if (-not $asset) { throw 'No matching release asset found.' }; Write-Host '[DOWNLOADING]' $asset.browser_download_url; Invoke-WebRequest -Uri $asset.browser_download_url -OutFile '%DL_FILE%'; Expand-Archive -Path '%DL_FILE%' -DestinationPath 'tools' -Force; Remove-Item '%DL_FILE%' -ErrorAction SilentlyContinue } catch { Write-Host '[DL-ERROR]' $_.Exception.Message; exit 1 }"

if exist "tools\w64devkit\bin\g++.exe" (
    set "COMPILER_BIN=tools\w64devkit\bin\g++.exe"
    set "COMPILER_TYPE=GCC"
    echo [OK] Portable compiler downloaded and installed at tools\w64devkit
    goto CompilerFound
)

if "!COMPILER_TYPE!"=="" (
    echo.
    echo =======================================================
    echo    [ERROR] Failed to find or install a C++ compiler.
    echo =======================================================
    echo  Try one of these manually, then re-run this script:
    echo    winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e
    echo    choco install mingw -y
    echo    scoop install gcc
    echo    Or download manually: https://github.com/skeeto/w64devkit/releases
    echo =======================================================
    pause
    exit /b 1
)

:CompilerFound
if not "!COMPILER_BIN!"=="" (
    for %%F in ("!COMPILER_BIN!") do set "PATH=%%~dpF;!PATH!"
)
if "!COMPILER_TYPE!"=="MSVC" (
    echo [FOUND] MSVC Environment : "!VCVARS_BAT!"
) else (
    echo [FOUND] !COMPILER_TYPE! Compiler    : "!COMPILER_BIN!"
)
echo.

REM =======================================================================
REM [PRIORITY 2.5] Detect / Setup .NET SDK for CPU Sensor Helper (rzkmon_sensor)
REM =======================================================================
set "DOTNET_BIN="
set "HAS_DOTNET_SDK="

REM Verifikasi apakah .NET SDK benar-benar terpasang (bukan sekadar runtime)
where dotnet.exe >nul 2>nul && (
    for /f "delims=" %%s in ('dotnet --list-sdks 2^>nul') do set "HAS_DOTNET_SDK=1"
)

if defined HAS_DOTNET_SDK (
    set "DOTNET_BIN=dotnet.exe"
)

if "!DOTNET_BIN!"=="" (
    echo [WARNING] .NET SDK not found - needed to build the CPU sensor helper.
    set /p CONFIRM_DOTNET="[PROMPT] Try installing .NET SDK automatically via winget/choco/scoop? (y/N): "
    if /i "!CONFIRM_DOTNET!"=="y" (
        set "PKG_MGR2="
        where winget >nul 2>nul && set "PKG_MGR2=winget"
        if "!PKG_MGR2!"=="" (where choco >nul 2>nul && set "PKG_MGR2=choco")
        if "!PKG_MGR2!"=="" (where scoop >nul 2>nul && set "PKG_MGR2=scoop")

        if "!PKG_MGR2!"=="winget" (
            winget install --id Microsoft.DotNet.SDK.8 -e --silent --accept-source-agreements --accept-package-agreements
        ) else if "!PKG_MGR2!"=="choco" (
            choco install dotnet-8.0-sdk -y
        ) else if "!PKG_MGR2!"=="scoop" (
            scoop install dotnet-sdk
        )
        for /f "delims=" %%s in ('dotnet --list-sdks 2^>nul') do set "DOTNET_BIN=dotnet.exe"
    )
)

if "!DOTNET_BIN!"=="" (
    echo [SKIP] .NET SDK not available - sensor helper rzkmon_sensor.exe will NOT be built.
    echo         CPU temperature will fall back to N/A on this machine.
) else (
    echo [FOUND] .NET SDK          : "!DOTNET_BIN!"
)
echo.

if not "!DOTNET_BIN!"=="" (
    if not exist "sensor\rzkmon_sensor.exe" (
        echo [INFO] Publishing CPU sensor helper - rzkmon_sensor.exe ...
        if not exist "tools\rzkmon_sensor" mkdir "tools\rzkmon_sensor"
        powershell -NoProfile -EncodedCommand WwBTAHkAcwB0AGUAbQAuAEkATwAuAEYAaQBsAGUAXQA6ADoAVwByAGkAdABlAEEAbABsAEIAeQB0AGUAcwAoACcAdABvAG8AbABzAC8AcgB6AGsAbQBvAG4AXwBzAGUAbgBzAG8AcgAvAFAAcgBvAGcAcgBhAG0ALgBjAHMAJwAsACAAWwBTAHkAcwB0AGUAbQAuAEMAbwBuAHYAZQByAHQAXQA6ADoARgByAG8AbQBCAGEAcwBlADYANABTAHQAcgBpAG4AZwAoACcAZABYAE4AcABiAG0AYwBnAFUAMwBsAHoAZABHAFYAdABPAHcAcAAxAGMAMgBsAHUAWgB5AEIATQBhAFcASgB5AFoAVQBoAGgAYwBtAFIAMwBZAFgASgBsAFQAVwA5AHUAYQBYAFIAdgBjAGkANQBJAFkAWABKAGsAZAAyAEYAeQBaAFQAcwBLAEMAbQBOAHMAWQBYAE4AegBJAEYAQgB5AGIAMgBkAHkAWQBXADAASwBlAHcAbwBnAEkAQwBBAGcAYwAzAFIAaABkAEcAbABqAEkASABaAHYAYQBXAFEAZwBUAFcARgBwAGIAaQBnAHAAQwBpAEEAZwBJAEMAQgA3AEMAaQBBAGcASQBDAEEAZwBJAEMAQQBnAGQAbQBGAHkASQBHAE4AdgBiAFgAQgAxAGQARwBWAHkASQBEADAAZwBiAG0AVgAzAEkARQBOAHYAYgBYAEIAMQBkAEcAVgB5AEkASABzAGcAUwBYAE4ARABjAEgAVgBGAGIAbQBGAGkAYgBHAFYAawBJAEQAMABnAGQASABKADEAWgBTAEIAOQBPAHcAbwBnAEkAQwBBAGcASQBDAEEAZwBJAEcATgB2AGIAWABCADEAZABHAFYAeQBMAGsAOQB3AFoAVwA0AG8ASwBUAHMASwBJAEMAQQBnAEkAQwBBAGcASQBDAEIAcABiAG4AUQBnAGMAbQBWAHoAZABXAHgAMABJAEQAMABnAEwAVABrADUATwBUAHMASwBDAGkAQQBnAEkAQwBBAGcASQBDAEEAZwBaAG0AOQB5AFoAVwBGAGoAYQBDAEEAbwBkAG0ARgB5AEkARwBoADMASQBHAGwAdQBJAEcATgB2AGIAWABCADEAZABHAFYAeQBMAGsAaABoAGMAbQBSADMAWQBYAEoAbABLAFEAbwBnAEkAQwBBAGcASQBDAEEAZwBJAEgAcwBLAEkAQwBBAGcASQBDAEEAZwBJAEMAQQBnAEkAQwBBAGcAYQBXAFYAZwBLAEcAaAAzAEwAawBoAGgAYwBtAFIAMwBZAFgASgBsAFYASABsAHcAWgBTAEEAaABQAFMAQgBJAFkAWABKAGsAZAAyAEYAeQBaAFYAUgA1AGMARwBVAHUAUQAzAEIAMQBLAFMAQgBqAGIAMgA1ADAAYQBXADUAMQBaAFQAcwBLAEkAQwBBAGcASQBDAEEAZwBJAEMAQQBnAEkAQwBBAGcAYQBIAGMAdQBWAFgAQgBrAFkAWABSAGwASwBDAGsANwBDAGcAbwBnAEkAQwBBAGcASQBDAEEAZwBJAEMAQQBnAEkAQwBCAG0AYgBHADkAaABkAEQAOABnAGMARwBGAGoAYQAyAEYAbgBaAFMAQQA5AEkARwA1ADEAYgBHAHcAcwBJAEcATgB2AGMAbQBWAE4AWQBYAGcAZwBQAFMAQgB1AGQAVwB4AHMATwB3AG8AZwBJAEMAQQBnAEkAQwBBAGcASQBDAEEAZwBJAEMAQgBtAGIAMwBKAGwAWQBXAE4AbwBJAEMAaAAyAFkAWABJAGcAYwAyAFYAdQBjADIAOQB5AEkARwBsAHUASQBHAGgAMwBMAGwATgBsAGIAbgBOAHYAYwBuAE0AcABDAGkAQQBnAEkAQwBBAGcASQBDAEEAZwBJAEMAQQBnAEkASABzAEsASQBDAEEAZwBJAEMAQQBnAEkAQwBBAGcASQBDAEEAZwBJAEMAQQBnAEkARwBsAG0ASQBDAGgAegBaAFcANQB6AGIAMwBJAHUAVQAyAFYAdQBjADIAOQB5AFYASABsAHcAWgBTAEEAaABQAFMAQgBUAFoAVwA1AHoAYgAzAEoAVQBlAFgAQgBsAEwAbABSAGwAYgBYAEIAbABjAG0ARgAwAGQAWABKAGwASQBIAHgAOABJAEgATgBsAGIAbgBOAHYAYwBpADUAVwBZAFcAeAAxAFoAUwBBADkAUABTAEIAdQBkAFcAeABzAEsAUwBCAGoAYgAyADUAMABhAFcANQAxAFoAVABzAEsASQBDAEEAZwBJAEMAQQBnAEkAQwBBAGcASQBDAEEAZwBJAEMAQQBnAEkARwBsAG0ASQBDAGgAegBaAFcANQB6AGIAMwBJAHUAVABtAEYAdABaAFMANQBEAGIAMgA1ADAAWQBXAGwAdQBjAHkAZwBpAFUARwBGAGoAYQAyAEYAbgBaAFMASQBwAEkASAB4ADgASQBIAE4AbABiAG4ATgB2AGMAaQA1AE8AWQBXADEAbABMAGsATgB2AGIAbgBSAGgAYQBXADUAegBLAEMASgBVAFoARwBsAGwASQBpAGsAZwBmAEgAdwBnAGMAMgBWAHUAYwAyADkAeQBMAGsANQBoAGIAVwBVAHUAUQAyADkAdQBkAEcARgBwAGIAbgBNAG8ASQBsAFIAagBkAEcAdwBpAEsAUwBrAEsASQBDAEEAZwBJAEMAQQBnAEkAQwBBAGcASQBDAEEAZwBJAEMAQQBnAEkAQwBBAGcASQBDAEIAdwBZAFcATgByAFkAVwBkAGwASQBEADAAZwBjADIAVgB1AGMAMgA5AHkATABsAFoAaABiAEgAVgBsAE8AdwBvAGcASQBDAEEAZwBJAEMAQQBnAEkAQwBBAGcASQBDAEEAZwBJAEMAQQBnAFoAVwB4AHoAWgBTAEIAcABaAGkAQQBvAGMAMgBWAHUAYwAyADkAeQBMAGsANQBoAGIAVwBVAHUAUQAyADkAdQBkAEcARgBwAGIAbgBNAG8ASQBrAE4AdgBjAG0AVQBpAEsAUwBBAG0ASgBpAEEAbwBZADIAOQB5AFoAVQAxAGgAZQBDAEEAOQBQAFMAQgB1AGQAVwB4AHMASQBIAHgAOABJAEgATgBsAGIAbgBOAHYAYwBpADUAVwBZAFcAeAAxAFoAUwBBACsASQBHAE4AdgBjAG0AVgBOAFkAWABnAHAASwBRAG8AZwBJAEMAQQBnAEkAQwBBAGcASQBDAEEAZwBJAEMAQQBnAEkAQwBBAGcASQBDAEEAZwBJAEcATgB2AGMAbQBWAE4AWQBYAGcAZwBQAFMAQgB6AFoAVwA1AHoAYgAzAEkAdQBWAG0ARgBzAGQAVwBVADcAQwBpAEEAZwBJAEMAQQBnAEkAQwBBAGcASQBDAEEAZwBJAEgAMABLAEkAQwBBAGcASQBDAEEAZwBJAEMAQQBnAEkAQwBBAGcAWgBtAHgAdgBZAFgAUQAvAEkASABCAHAAWQAyAHMAZwBQAFMAQgB3AFkAVwBOAHIAWQBXAGQAbABJAEQAOAAvAEkARwBOAHYAYwBtAFYATgBZAFgAZwA3AEMAaQBBAGcASQBDAEEAZwBJAEMAQQBnAEkAQwBBAGcASQBHAGwAbQBJAEMAaAB3AGEAVwBOAHIATABrAGgAaABjADEAWgBoAGIASABWAGwASwBTAEIAeQBaAFgATgAxAGIASABRAGcAUABTAEEAbwBhAFcANQAwAEsAWABCAHAAWQAyAHMAdQBWAG0ARgBzAGQAVwBVADcAQwBpAEEAZwBJAEMAQQBnAEkAQwBBAGcASQBDAEEAZwBJAEcASgB5AFoAVwBGAHIATwB3AG8AZwBJAEMAQQBnAEkAQwBBAGcASQBIADAASwBDAGkAQQBnAEkAQwBBAGcASQBDAEEAZwBZADIAOQB0AGMASABWADAAWgBYAEkAdQBRADIAeAB2AGMAMgBVAG8ASwBUAHMASwBJAEMAQQBnAEkAQwBBAGcASQBDAEIARABiADIANQB6AGIAMgB4AGwATABsAGQAeQBhAFgAUgBsAFQARwBsAHUAWgBTAGgAeQBaAFgATgAxAGIASABRAHAATwB3AG8AZwBJAEMABnAGYAUQBwADkAQwBnAD0APQAnACkAKQA=
        "!DOTNET_BIN!" nuget add source https://api.nuget.org/v3/index.json -n nuget.org >nul 2>nul
        pushd "tools\rzkmon_sensor"
        "!DOTNET_BIN!" publish -c Release -o "..\..\sensor"
        popd
    )
    if exist "sensor\rzkmon_sensor.exe" (
        echo [OK] Sensor helper ready at sensor\rzkmon_sensor.exe
    ) else (
        echo [WARNING] Sensor helper build failed - see %TEMP%\rzkmon_dotnet.log
    )
)
echo.

REM =======================================================================
REM Detect / Download Microsoft WebView2 SDK
REM =======================================================================
set "WEBVIEW2_DIR="

if exist "%USERPROFILE%\.nuget\packages\microsoft.web.webview2" (
    for /d %%i in ("%USERPROFILE%\.nuget\packages\microsoft.web.webview2\*") do (
        if exist "%%i\build\native\include\WebView2.h" set "WEBVIEW2_DIR=%%i"
    )
)

if "%WEBVIEW2_DIR%"=="" (
    for /d %%i in (packages\Microsoft.Web.WebView2.*) do (
        if exist "%%i\build\native\include\WebView2.h" set "WEBVIEW2_DIR=%%i"
    )
)

if "%WEBVIEW2_DIR%"=="" (
    for %%d in (%ACTIVE_DRIVES%) do (
        if "%WEBVIEW2_DIR%"=="" (
            if exist "%%d\vcpkg\installed\x64-windows\include\WebView2.h" (
                set "WEBVIEW2_DIR=%%d\vcpkg\installed\x64-windows"
            )
        )
    )
)

if "%WEBVIEW2_DIR%"=="" (
    echo [INFO] Downloading the WebView2 SDK via NuGet into the packages folder
    if not exist "packages" mkdir "packages"
    if exist "nuget.exe" (
        nuget.exe install Microsoft.Web.WebView2 -OutputDirectory packages -Source https://api.nuget.org/v3/index.json
    ) else (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; (New-Object Net.WebClient).DownloadFile('https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2', 'webview2.zip'); Expand-Archive -Path 'webview2.zip' -DestinationPath 'packages\Microsoft.Web.WebView2' -Force; Remove-Item 'webview2.zip' -ErrorAction SilentlyContinue"
    )
    for /d %%i in (packages\Microsoft.Web.WebView2*) do set "WEBVIEW2_DIR=%%i"
)

echo [INFO] WebView2 SDK       : "%WEBVIEW2_DIR%"
set "WEBVIEW2_INCLUDE=%WEBVIEW2_DIR%\build\native\include"
set "WEBVIEW2_LIB=%WEBVIEW2_DIR%\build\native\x64"

if not exist "%WEBVIEW2_INCLUDE%\WebView2.h" (
    set "WEBVIEW2_INCLUDE=%WEBVIEW2_DIR%\include"
    set "WEBVIEW2_LIB=%WEBVIEW2_DIR%\lib"
)

REM =======================================================================
REM Resolve Target Source File First
REM =======================================================================
set "TARGET_SRC="
if exist "main_windows.cpp" set "TARGET_SRC=main_windows.cpp"
if "%TARGET_SRC%"=="" if exist "src\main_windows.cpp" set "TARGET_SRC=src\main_windows.cpp"
if "%TARGET_SRC%"=="" (
    echo [ERROR] Source file not found ^(main_windows.cpp / src\main_windows.cpp^)
    pause
    exit /b 1
)

echo.
echo =======================================================
echo    DEPENDENCY RESOLUTION SUMMARY
echo =======================================================
echo [OK] Compiler   : %COMPILER_TYPE% (%COMPILER_BIN%%VCVARS_BAT%)
echo [OK] WebView2   : %WEBVIEW2_DIR%
echo [OK] Source     : %TARGET_SRC%
echo =======================================================
echo.

echo.
echo [INFO] Compiling %TARGET_SRC% into rizkybymonitor_windows.exe
echo.

set "BUILD_LOG=%TEMP%\rzkmon_build.log"
if "%COMPILER_TYPE%"=="MSVC" (
    call "!VCVARS_BAT!" >nul 2>nul
    cl.exe /nologo /std:c++17 /O2 /W3 /EHsc /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I"%WEBVIEW2_INCLUDE%" %TARGET_SRC% /Fe:rizkybymonitor_windows.exe /link /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup /LIBPATH:"%WEBVIEW2_LIB%" ws2_32.lib iphlpapi.lib pdh.lib psapi.lib powrprof.lib dxgi.lib ole32.lib oleaut32.lib uuid.lib shlwapi.lib setupapi.lib rpcrt4.lib WebView2LoaderStatic.lib >"!BUILD_LOG!" 2>&1
) else (
    "!COMPILER_BIN!" -std=c++17 -O2 -pthread -municode -mwindows -static -static-libgcc -static-libstdc++ -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -DNOMINMAX -DWIN32_LEAN_AND_MEAN -I"%WEBVIEW2_INCLUDE%" %TARGET_SRC% -o rizkybymonitor_windows.exe -L"%WEBVIEW2_LIB%" -lws2_32 -liphlpapi -lpdh -lpsapi -lpowrprof -ldxgi -lole32 -loleaut32 -luuid -lshlwapi -lsetupapi -lwlanapi -lshell32 -lwbemuuid -l:WebView2Loader.dll.lib -lrpcrt4 -ldwmapi >"!BUILD_LOG!" 2>&1
)
set "COMPILE_STATUS=!ERRORLEVEL!"
type "!BUILD_LOG!"

if !COMPILE_STATUS! neq 0 (
    echo.
    echo =======================================================
    echo  [ERROR] Build Failed! Full compiler output above.
    echo =======================================================
    del /f /q "!BUILD_LOG!" >nul 2>nul
    pause
    exit /b 1
)
if not exist "rizkybymonitor_windows.exe" (
    echo.
    echo =======================================================
    echo  [ERROR] Output binary not found! Build failed.
    echo =======================================================
    del /f /q "!BUILD_LOG!" >nul 2>nul
    pause
    exit /b 1
)
del /f /q "!BUILD_LOG!" >nul 2>nul

if exist main_windows.obj del /f /q main_windows.obj >nul 2>nul
if exist "%WEBVIEW2_LIB%\WebView2Loader.dll" copy /y "%WEBVIEW2_LIB%\WebView2Loader.dll" . >nul 2>nul
if exist "packages\Microsoft.Web.WebView2*\build\native\x64\WebView2Loader.dll" (
    for /f "delims=" %%f in ('dir /b /s "packages\Microsoft.Web.WebView2*\build\native\x64\WebView2Loader.dll" 2^>nul') do copy /y "%%f" . >nul 2>nul
)

echo.
echo =======================================================
echo    [SUCCESS] rizkybymonitor_windows.exe is ready!
echo =======================================================
echo.
pause
exit /b 0