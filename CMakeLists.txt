cmake_minimum_required(VERSION 3.14)
project(RizkybyMONITOR VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    # -------------------------------------------------------------------------
    # Windows build: src/main_windows.cpp → rizkybymonitor_windows.exe
    # Requires: Microsoft WebView2 SDK (nuget / vcpkg) + Windows SDK 10.0+
    # -------------------------------------------------------------------------
    add_executable(rizkybymonitor_windows WIN32 src/main_windows.cpp)

    target_link_libraries(rizkybymonitor_windows PRIVATE
        ws2_32
        iphlpapi
        pdh
        psapi
        powrprof
        ole32
        oleaut32
        uuid
        shlwapi
        setupapi
        dxgi
        WindowsApp
    )

    target_compile_definitions(rizkybymonitor_windows PRIVATE
        _WIN32_WINNT=0x0A00
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )

    # WebView2 SDK — set WEBVIEW2_DIR to the package root
    # e.g. cmake .. -DWEBVIEW2_DIR="packages\Microsoft.Web.WebView2.1.0.x"
    if(DEFINED WEBVIEW2_DIR)
        target_include_directories(rizkybymonitor_windows PRIVATE
            "${WEBVIEW2_DIR}/build/native/include")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            target_link_directories(rizkybymonitor_windows PRIVATE
                "${WEBVIEW2_DIR}/build/native/x64")
        else()
            target_link_directories(rizkybymonitor_windows PRIVATE
                "${WEBVIEW2_DIR}/build/native/x86")
        endif()
        target_link_libraries(rizkybymonitor_windows PRIVATE WebView2LoaderStatic.lib)
    endif()

else()
    # -------------------------------------------------------------------------
    # Linux build: src/main_linux.cpp → rizkybymonitor_linux
    # Requires: GTK3 + WebKit2GTK 4.1 + pthread
    # -------------------------------------------------------------------------
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3    REQUIRED gtk+-3.0)
    pkg_check_modules(WEBKIT  REQUIRED webkit2gtk-4.1)

    add_executable(rizkybymonitor_linux src/main_linux.cpp)

    target_include_directories(rizkybymonitor_linux PRIVATE
        ${GTK3_INCLUDE_DIRS}
        ${WEBKIT_INCLUDE_DIRS})

    target_link_libraries(rizkybymonitor_linux PRIVATE
        ${GTK3_LIBRARIES}
        ${WEBKIT_LIBRARIES}
        pthread)

    target_compile_options(rizkybymonitor_linux PRIVATE -O3 -Wall -Wextra)
endif()
