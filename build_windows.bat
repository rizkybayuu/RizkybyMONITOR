@echo off
REM =======================================================================
REM RizkybyMONITOR - Windows Build Script
REM =======================================================================
REM Requirements:
REM   - Visual Studio 2019+ with C++ workload (or VS Build Tools)
REM   - Windows SDK 10.0.19041.0+
REM   - Microsoft WebView2 SDK
REM
REM Install WebView2 SDK via NuGet:
REM   nuget install Microsoft.Web.WebView2 -OutputDirectory packages
REM
REM Or via vcpkg:
REM   vcpkg install webview2
REM =======================================================================

setlocal EnableDelayedExpansion

echo ========================================
echo  RizkybyMONITOR Windows Build
echo ========================================

REM --- Locate WebView2 SDK ---
set WEBVIEW2_DIR=
for /d %%i in (packages\Microsoft.Web.WebView2.*) do set WEBVIEW2_DIR=%%i
if "%WEBVIEW2_DIR%"=="" (
    REM Try vcpkg installed location
    if exist vcpkg_installed\x64-windows\include\WebView2.h (
        set WEBVIEW2_DIR=vcpkg_installed\x64-windows
    )
)

if "%WEBVIEW2_DIR%"=="" (
    echo [ERROR] Microsoft WebView2 SDK not found!
    echo.
    echo Please install it with:
    echo   nuget install Microsoft.Web.WebView2 -OutputDirectory packages
    echo   OR
    echo   vcpkg install webview2
    echo.
    pause
    exit /b 1
)

echo [INFO] Found WebView2 SDK at: %WEBVIEW2_DIR%

REM --- Setup MSVC Environment ---
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% set VSWHERE="%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist %VSWHERE% (
    echo [ERROR] Visual Studio not found. Please install Visual Studio 2019 or later.
    pause
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -prerelease -property installationPath`) do set VS_PATH=%%i
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"

REM --- Compile ---
echo.
echo [INFO] Compiling src\main_windows.cpp...
echo.

set WEBVIEW2_INCLUDE=%WEBVIEW2_DIR%\build\native\include
set WEBVIEW2_LIB=%WEBVIEW2_DIR%\build\native\x64

cl.exe /std:c++17 /O2 /W3 /EHsc /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /DNOMINMAX /DWIN32_LEAN_AND_MEAN ^
    /I"%WEBVIEW2_INCLUDE%" ^
    src\main_windows.cpp ^
    /Fe:rizkybymonitor_windows.exe ^
    /link ^
    /SUBSYSTEM:WINDOWS ^
    /LIBPATH:"%WEBVIEW2_LIB%" ^
    ws2_32.lib iphlpapi.lib pdh.lib psapi.lib powrprof.lib ^
    dxgi.lib ole32.lib oleaut32.lib uuid.lib shlwapi.lib ^
    setupapi.lib WebView2LoaderStatic.lib

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed! Check error messages above.
    pause
    exit /b 1
)

REM --- Copy WebView2Loader.dll ---
if exist "%WEBVIEW2_LIB%\WebView2Loader.dll" (
    copy /y "%WEBVIEW2_LIB%\WebView2Loader.dll" . >nul
    echo [INFO] Copied WebView2Loader.dll
)

echo.
echo ========================================
echo  BUILD SUCCESSFUL!
echo ========================================
echo  Output: rizkybymonitor_windows.exe
echo.
echo  Make sure these files are in the same folder:
echo    - rizkybymonitor_windows.exe
echo    - index.html
echo    - WebView2Loader.dll  (if not statically linked)
echo.
echo  Run: rizkybymonitor_windows.exe
echo ========================================
pause
