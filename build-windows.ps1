# SmartBook Windows Build Script
# PowerShell script for building on Windows (replaces Makefile)

param(
    [string]$Target = "all",
    [switch]$Run = $false,
    [string]$BuildType = "Release",
    [string]$Platform = "x86_64",
    [string]$QtPath = "",
    [switch]$Clean = $false,
    [switch]$Install = $false,
    [switch]$Help = $false
)

$ErrorActionPreference = "Stop"

# Script directory
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = $ScriptDir
Set-Location $ProjectRoot

# Help message
if ($Help) {
    Write-Host "SmartBook Windows Build Script" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Usage: .\build-windows.ps1 [options]" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -Target <target>     Build target: all, common, reader, creator, test, run-reader, run-creator (default: all)"
    Write-Host "  -BuildType <type>    Build type: Release, Debug (default: Release)"
    Write-Host "  -Platform <arch>     Platform: x86_64 (default: x86_64)"
    Write-Host "  -QtPath <path>       Custom Qt installation path (auto-detected if not specified)"
    Write-Host "  -Clean               Clean build directories before building"
    Write-Host "  -Install             Install after building"
    Write-Host "  -Run                 Run application after building"
    Write-Host "  -Help                Show this help message"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\build-windows.ps1                    # Build everything"
    Write-Host "  .\build-windows.ps1 -Target reader    # Build reader only"
    Write-Host "  .\build-windows.ps1 -Clean -Install    # Clean, build, and install"
    exit 0
}

# Function to find Qt installation
function Find-QtInstallation {
    $qtPaths = @()
    
    # Check %USERPROFILE%\Qt
    if ($env:USERPROFILE) {
        $userQt = Join-Path $env:USERPROFILE "Qt"
        if (Test-Path $userQt) {
            $qtPaths += $userQt
        }
    }
    
    # Check C:\Qt
    $cQt = "C:\Qt"
    if (Test-Path $cQt) {
        $qtPaths += $cQt
    }
    
    # Check QTDIR environment variable
    if ($env:QTDIR) {
        $qtDir = Split-Path -Parent $env:QTDIR
        if (Test-Path $qtDir) {
            $qtPaths += $qtDir
        }
    }
    
    foreach ($qtRoot in $qtPaths) {
        # Find Qt 6.x versions
        $versions = Get-ChildItem -Path $qtRoot -Directory -Filter "6.*" | 
                    Sort-Object { [Version]($_.Name) } -Descending
        
        foreach ($version in $versions) {
            # Try MSVC first (most common on Windows)
            $msvcDirs = Get-ChildItem -Path $version.FullName -Directory -Filter "msvc*_64"
            if ($msvcDirs) {
                $latestMsvc = $msvcDirs | Sort-Object Name -Descending | Select-Object -First 1
                $qtPrefix = $latestMsvc.FullName
                Write-Host "Found Qt at: $qtPrefix" -ForegroundColor Green
                return $qtPrefix
            }
            
            # Fallback to MinGW
            $mingwDirs = Get-ChildItem -Path $version.FullName -Directory -Filter "mingw*_64"
            if ($mingwDirs) {
                $latestMingw = $mingwDirs | Sort-Object Name -Descending | Select-Object -First 1
                $qtPrefix = $latestMingw.FullName
                Write-Host "Found Qt at: $qtPrefix" -ForegroundColor Green
                return $qtPrefix
            }
        }
    }
    
    Write-Host "Warning: Qt not found in standard locations. Using QTDIR or system default." -ForegroundColor Yellow
    return $null
}

# Find Qt
if ([string]::IsNullOrEmpty($QtPath)) {
    $QtPath = Find-QtInstallation
}

# Build directories
$BuildDir = Join-Path $ProjectRoot "build\windows-$Platform"

# Clean if requested
if ($Clean) {
    Write-Host "Cleaning build directories..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item -Path $BuildDir -Recurse -Force
    }
}

# Function to build a component
function Build-Component {
    param(
        [string]$ComponentName,
        [string]$ComponentPath,
        [string]$CmakeTarget
    )
    
    Write-Host "`nBuilding $ComponentName..." -ForegroundColor Cyan
    $ComponentBuildDir = Join-Path $BuildDir $ComponentName.ToLower()
    New-Item -ItemType Directory -Force -Path $ComponentBuildDir | Out-Null
    
    Push-Location $ComponentBuildDir
    
    try {
        # Configure CMake
        $cmakeArgs = @(
            "-DCMAKE_BUILD_TYPE=$BuildType"
        )
        
        if ($QtPath) {
            $cmakeArgs += "-DCMAKE_PREFIX_PATH=$QtPath"
        }
        
        $cmakeArgs += "..\..\..\$ComponentPath"
        
        Write-Host "Configuring CMake..." -ForegroundColor Gray
        & cmake @cmakeArgs
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }
        
        # Build
        Write-Host "Building..." -ForegroundColor Gray
        & cmake --build . --config $BuildType
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }
        
        Write-Host "$ComponentName built successfully!" -ForegroundColor Green
    }
    finally {
        Pop-Location
    }
}

# Function to install a component
function Install-Component {
    param(
        [string]$ComponentName
    )
    
    Write-Host "`nInstalling $ComponentName..." -ForegroundColor Cyan
    $ComponentBuildDir = Join-Path $BuildDir $ComponentName.ToLower()
    
    if (-not (Test-Path $ComponentBuildDir)) {
        Write-Host "Error: $ComponentName not built. Run build first." -ForegroundColor Red
        return
    }
    
    Push-Location $ComponentBuildDir
    
    try {
        & cmake --install . --config $BuildType
        if ($LASTEXITCODE -ne 0) {
            throw "Install failed"
        }
        Write-Host "$ComponentName installed successfully!" -ForegroundColor Green
    }
    finally {
        Pop-Location
    }
}

# Handle run targets (don't build, just run)
if ($Target -match "^run-") {
    $RunTarget = $Target -replace "run-", ""
    $BuildPath = Join-Path $BuildDir $RunTarget
    $ExePath = $null
    
    if ($RunTarget -eq "reader") {
        $ExePath = Join-Path $BuildPath "smartbook-reader.exe"
        if (-not (Test-Path $ExePath)) {
            $ExePath = Join-Path (Join-Path $BuildDir "bin") "smartbook-reader.exe"
        }
    } elseif ($RunTarget -eq "creator") {
        $ExePath = Join-Path $BuildPath "smartbook-creator.exe"
        if (-not (Test-Path $ExePath)) {
            $ExePath = Join-Path (Join-Path $BuildDir "bin") "smartbook-creator.exe"
        }
    } elseif ($RunTarget -eq "server") {
        Write-Host "Server application not yet implemented (Phase 2)" -ForegroundColor Yellow
        exit 1
    }
    
    if ($ExePath -and (Test-Path $ExePath)) {
        Write-Host "Running $RunTarget..." -ForegroundColor Cyan
        & $ExePath
        exit 0
    } else {
        Write-Host "Error: $RunTarget not built. Run build first." -ForegroundColor Red
        Write-Host "  .\build-windows.ps1 -Target $RunTarget" -ForegroundColor Yellow
        exit 1
    }
}

# Main build logic
Write-Host "`n=== SmartBook Windows Build ===" -ForegroundColor Cyan
Write-Host "Target: $Target" -ForegroundColor Yellow
Write-Host "Build Type: $BuildType" -ForegroundColor Yellow
Write-Host "Platform: $Platform" -ForegroundColor Yellow
if ($QtPath) {
    Write-Host "Qt Path: $QtPath" -ForegroundColor Yellow
}

# Build targets
switch ($Target.ToLower()) {
    "all" {
        Build-Component -ComponentName "common" -ComponentPath "common"
        Build-Component -ComponentName "reader" -ComponentPath "reader"
        Build-Component -ComponentName "creator" -ComponentPath "creator"
    }
    "common" {
        Build-Component -ComponentName "common" -ComponentPath "common"
    }
    "reader" {
        Build-Component -ComponentName "common" -ComponentPath "common"
        Build-Component -ComponentName "reader" -ComponentPath "reader"
    }
    "creator" {
        Build-Component -ComponentName "common" -ComponentPath "common"
        Build-Component -ComponentName "creator" -ComponentPath "creator"
    }
    "test" {
        Build-Component -ComponentName "common" -ComponentPath "common"
        Build-Component -ComponentName "test" -ComponentPath "test"
    }
    default {
        Write-Host "Error: Unknown target '$Target'" -ForegroundColor Red
        Write-Host "Use -Help to see available targets" -ForegroundColor Yellow
        exit 1
    }
}

# Install if requested
if ($Install) {
    switch ($Target.ToLower()) {
        "all" {
            Install-Component -ComponentName "common"
            Install-Component -ComponentName "reader"
            Install-Component -ComponentName "creator"
        }
        "common" {
            Install-Component -ComponentName "common"
        }
        "reader" {
            Install-Component -ComponentName "reader"
        }
        "creator" {
            Install-Component -ComponentName "creator"
        }
    }
}

Write-Host "`n=== Build Complete ===" -ForegroundColor Green

# Run after build if requested
if ($Run) {
    $RunTarget = if ($Target -eq "reader") { "reader" } elseif ($Target -eq "creator") { "creator" } else { "reader" }
    
    $BuildPath = Join-Path $BuildDir $RunTarget
    $ExePath = $null
    
    if ($RunTarget -eq "reader") {
        $ExePath = Join-Path $BuildPath "smartbook-reader.exe"
        if (-not (Test-Path $ExePath)) {
            $ExePath = Join-Path (Join-Path $BuildDir "bin") "smartbook-reader.exe"
        }
    } elseif ($RunTarget -eq "creator") {
        $ExePath = Join-Path $BuildPath "smartbook-creator.exe"
        if (-not (Test-Path $ExePath)) {
            $ExePath = Join-Path (Join-Path $BuildDir "bin") "smartbook-creator.exe"
        }
    }
    
    if ($ExePath -and (Test-Path $ExePath)) {
        Write-Host "`nRunning $RunTarget..." -ForegroundColor Cyan
        & $ExePath
    }
}
