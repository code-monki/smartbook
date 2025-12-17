# SmartBook Windows Install Script
# Installs built binaries to a target directory

param(
    [string]$InstallPrefix = "$env:ProgramFiles\SmartBook",
    [string]$BuildType = "Release",
    [switch]$Help = $false
)

$ErrorActionPreference = "Stop"

# Script directory
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = $ScriptDir
Set-Location $ProjectRoot

# Help message
if ($Help) {
    Write-Host "SmartBook Windows Install Script" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Usage: .\install-windows.ps1 [options]" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -InstallPrefix <path>  Installation directory (default: `$env:ProgramFiles\SmartBook)"
    Write-Host "  -BuildType <type>      Build type: Release, Debug (default: Release)"
    Write-Host "  -Help                  Show this help message"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\install-windows.ps1"
    Write-Host "  .\install-windows.ps1 -InstallPrefix `"C:\MyApps\SmartBook`""
    exit 0
}

$BuildDir = Join-Path $ProjectRoot "build\windows-x86_64"

Write-Host "`n=== SmartBook Windows Install ===" -ForegroundColor Cyan
Write-Host "Install Prefix: $InstallPrefix" -ForegroundColor Yellow
Write-Host "Build Type: $BuildType" -ForegroundColor Yellow

# Check if build exists
if (-not (Test-Path $BuildDir)) {
    Write-Host "Error: Build directory not found. Run build-windows.ps1 first." -ForegroundColor Red
    exit 1
}

# Create install directory
New-Item -ItemType Directory -Force -Path $InstallPrefix | Out-Null

# Install components
$components = @("common", "reader", "creator")

foreach ($component in $components) {
    $ComponentBuildDir = Join-Path $BuildDir $component
    
    if (-not (Test-Path $ComponentBuildDir)) {
        Write-Host "Warning: $component not built, skipping..." -ForegroundColor Yellow
        continue
    }
    
    Write-Host "`nInstalling $component..." -ForegroundColor Cyan
    Push-Location $ComponentBuildDir
    
    try {
        # Use CMake install with custom prefix
        & cmake --install . --config $BuildType --prefix $InstallPrefix
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Warning: Install of $component failed" -ForegroundColor Yellow
        } else {
            Write-Host "$component installed successfully!" -ForegroundColor Green
        }
    }
    finally {
        Pop-Location
    }
}

Write-Host "`n=== Install Complete ===" -ForegroundColor Green
Write-Host "SmartBook installed to: $InstallPrefix" -ForegroundColor Cyan
