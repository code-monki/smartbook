# SmartBook Windows Build Instructions

## Overview

Windows builds use **PowerShell scripts** instead of GNU Make, since Make is not available by default on Windows. The build system automatically detects Qt installations and handles MSVC or MinGW compiler variants.

## Prerequisites

1. **Qt 6.2+** installed at:
   - `%USERPROFILE%\Qt\6.x.x\msvc2019_64` (MSVC - recommended)
   - `%USERPROFILE%\Qt\6.x.x\mingw*_64` (MinGW)
   - `C:\Qt\6.x.x\msvc2019_64` (alternative location)

2. **CMake 3.20+** - Download from https://cmake.org/download/

3. **Compiler:**
   - **MSVC:** Visual Studio 2019+ with C++ build tools
   - **MinGW:** MinGW-w64 (if using MinGW Qt)

4. **PowerShell 5.1+** (included with Windows 10/11)

## Building

### Quick Start

```powershell
# Build everything
.\build-windows.ps1

# Build specific component
.\build-windows.ps1 -Target reader
.\build-windows.ps1 -Target creator
.\build-windows.ps1 -Target common

# Clean and rebuild
.\build-windows.ps1 -Clean

# Build and install
.\build-windows.ps1 -Install
```

### Build Options

```powershell
.\build-windows.ps1 [options]
```

**Options:**
- `-Target <target>` - Build target: `all`, `common`, `reader`, `creator`, `test` (default: `all`)
- `-BuildType <type>` - Build type: `Release`, `Debug` (default: `Release`)
- `-Platform <arch>` - Platform: `x86_64` (default: `x86_64`)
- `-QtPath <path>` - Custom Qt installation path (auto-detected if not specified)
- `-Clean` - Clean build directories before building
- `-Install` - Install after building
- `-Help` - Show help message

### Examples

```powershell
# Build everything in Release mode
.\build-windows.ps1

# Build reader only in Debug mode
.\build-windows.ps1 -Target reader -BuildType Debug

# Clean, build, and install
.\build-windows.ps1 -Clean -Install

# Use custom Qt path
.\build-windows.ps1 -QtPath "D:\Qt\6.9.3\msvc2019_64"
```

## Installation

### Using Install Script

```powershell
# Install to default location (Program Files)
.\install-windows.ps1

# Install to custom location
.\install-windows.ps1 -InstallPrefix "C:\MyApps\SmartBook"
```

### Manual Installation

After building, binaries are located in:
- `build\windows-x86_64\common\lib\` - Common library (DLL)
- `build\windows-x86_64\reader\` - Reader executable
- `build\windows-x86_64\creator\` - Creator executable

Copy these to your desired installation directory.

## Using CMake Directly

If you prefer to use CMake directly (e.g., from Visual Studio or Qt Creator):

```powershell
# Configure
cmake -B build\windows-x86_64 -S . -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build\windows-x86_64 --config Release

# Install
cmake --install build\windows-x86_64 --config Release --prefix "C:\Program Files\SmartBook"
```

## Qt Detection

The build scripts automatically detect Qt installations in:
1. `%USERPROFILE%\Qt\6.x.x\msvc*_64` (MSVC - preferred)
2. `%USERPROFILE%\Qt\6.x.x\mingw*_64` (MinGW)
3. `C:\Qt\6.x.x\msvc*_64` (alternative)
4. `C:\Qt\6.x.x\mingw*_64` (alternative)
5. `%QTDIR%` environment variable

The latest Qt 6.x version is automatically selected.

## Troubleshooting

### PowerShell Execution Policy

If you get an execution policy error:

```powershell
# Allow script execution for current session
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process

# Or run with bypass flag
powershell -ExecutionPolicy Bypass -File build-windows.ps1
```

### Qt Not Found

If Qt is not auto-detected:

```powershell
# Specify Qt path explicitly
.\build-windows.ps1 -QtPath "C:\Qt\6.9.3\msvc2019_64"

# Or set QTDIR environment variable
$env:QTDIR = "C:\Qt\6.9.3\msvc2019_64"
.\build-windows.ps1
```

### CMake Not Found

Ensure CMake is in your PATH, or specify full path:

```powershell
& "C:\Program Files\CMake\bin\cmake.exe" -B build -S .
```

### Missing Dependencies

Ensure all Qt components are installed:
- Qt6Core
- Qt6Gui
- Qt6Widgets
- Qt6WebEngine
- Qt6WebChannel
- Qt6Sql

## Differences from Unix Builds

1. **No GNU Make:** Windows uses PowerShell scripts instead
2. **Path Separators:** Use backslashes (`\`) in paths
3. **Executable Extension:** Binaries have `.exe` extension
4. **Library Extension:** DLLs use `.dll` extension
5. **Installation:** May require administrator privileges for Program Files

## Integration with Visual Studio

You can open the project in Visual Studio:

```powershell
# Generate Visual Studio solution
cmake -B build\vs2022 -S . -G "Visual Studio 17 2022"

# Open in Visual Studio
start build\vs2022\SmartBook.sln
```

## Next Steps

- See main [README.md](README.md) for project overview
- See [Documentation/build-process.adoc](Documentation/build-process.adoc) for detailed build documentation
- See [Documentation/srs.adoc](Documentation/srs.adoc) for requirements
