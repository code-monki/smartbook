# SmartBook Dependency Checker (Windows PowerShell)
# Checks for required dependencies and provides installation guidance

$ErrorActionPreference = "Continue"

# Colors for output
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

Write-ColorOutput "=== SmartBook Dependency Checker ===" "Cyan"
Write-Host ""

# Function to check command
function Test-Command {
    param([string]$Command, [string]$VersionArg = "--version")
    
    try {
        $result = & $Command $VersionArg 2>&1 | Select-Object -First 1
        if ($LASTEXITCODE -eq 0 -or $result) {
            Write-ColorOutput "✓ $Command : $result" "Green"
            return $true
        }
    } catch {
        Write-ColorOutput "✗ $Command : Not found" "Red"
        return $false
    }
    Write-ColorOutput "✗ $Command : Not found" "Red"
    return $false
}

# Function to check Qt
function Test-Qt {
    $QT_FOUND = $false
    $QT_PATH = $null
    $QT_VERSION = $null
    
    # Check %USERPROFILE%\Qt
    $userQt = Join-Path $env:USERPROFILE "Qt"
    if (Test-Path $userQt) {
        $versions = Get-ChildItem -Path $userQt -Directory -Filter "6.*" | 
                    Sort-Object { [Version]($_.Name) } -Descending
        
        foreach ($version in $versions) {
            # Try MSVC first
            $msvcDirs = Get-ChildItem -Path $version.FullName -Directory -Filter "msvc*_64"
            if ($msvcDirs) {
                $qtPath = ($msvcDirs | Sort-Object Name -Descending | Select-Object -First 1).FullName
                if (Test-Path (Join-Path $qtPath "bin\qmake.exe")) {
                    $QT_FOUND = $true
                    $QT_PATH = $qtPath
                    break
                }
            }
            
            # Try MinGW
            $mingwDirs = Get-ChildItem -Path $version.FullName -Directory -Filter "mingw*_64"
            if ($mingwDirs -and -not $QT_FOUND) {
                $qtPath = ($mingwDirs | Sort-Object Name -Descending | Select-Object -First 1).FullName
                if (Test-Path (Join-Path $qtPath "bin\qmake.exe")) {
                    $QT_FOUND = $true
                    $QT_PATH = $qtPath
                    break
                }
            }
        }
    }
    
    # Check C:\Qt
    if (-not $QT_FOUND -and (Test-Path "C:\Qt")) {
        $versions = Get-ChildItem -Path "C:\Qt" -Directory -Filter "6.*" | 
                    Sort-Object { [Version]($_.Name) } -Descending
        
        foreach ($version in $versions) {
            $msvcDirs = Get-ChildItem -Path $version.FullName -Directory -Filter "msvc*_64"
            if ($msvcDirs) {
                $qtPath = ($msvcDirs | Sort-Object Name -Descending | Select-Object -First 1).FullName
                if (Test-Path (Join-Path $qtPath "bin\qmake.exe")) {
                    $QT_FOUND = $true
                    $QT_PATH = $qtPath
                    break
                }
            }
        }
    }
    
    # Check QTDIR
    if (-not $QT_FOUND -and $env:QTDIR) {
        if (Test-Path (Join-Path $env:QTDIR "bin\qmake.exe")) {
            $QT_FOUND = $true
            $QT_PATH = $env:QTDIR
        }
    }
    
    if ($QT_FOUND) {
        try {
            $qmakePath = Join-Path $QT_PATH "bin\qmake.exe"
            $versionOutput = & $qmakePath -v 2>&1
            $QT_VERSION = ($versionOutput | Select-String "Using Qt version (\S+)" | ForEach-Object { $_.Matches.Groups[1].Value })
        } catch {
            $QT_VERSION = "Unknown"
        }
        
        Write-ColorOutput "✓ Qt 6 : $QT_VERSION (at $QT_PATH)" "Green"
        
        # Check Qt components
        $components = @(
            @{Name="Qt6Core"; Path="lib\cmake\Qt6Core\Qt6CoreConfig.cmake"},
            @{Name="Qt6Gui"; Path="lib\cmake\Qt6Gui\Qt6GuiConfig.cmake"},
            @{Name="Qt6Widgets"; Path="lib\cmake\Qt6Widgets\Qt6WidgetsConfig.cmake"},
            @{Name="Qt6WebEngine"; Path="lib\cmake\Qt6WebEngine\Qt6WebEngineConfig.cmake"; Optional=$true},
            @{Name="Qt6WebChannel"; Path="lib\cmake\Qt6WebChannel\Qt6WebChannelConfig.cmake"; Optional=$true},
            @{Name="Qt6Sql"; Path="lib\cmake\Qt6Sql\Qt6SqlConfig.cmake"}
        )
        
        foreach ($comp in $components) {
            $compPath = Join-Path $QT_PATH $comp.Path
            if (Test-Path $compPath) {
                Write-ColorOutput "  ✓ $($comp.Name)" "Green"
            } elseif ($comp.Optional) {
                Write-ColorOutput "  ⚠ $($comp.Name) (required for Reader/Creator)" "Yellow"
            } else {
                Write-ColorOutput "  ✗ $($comp.Name)" "Red"
            }
        }
        
        # Check Qt Test
        $testPaths = @(
            "lib\cmake\Qt6Test\Qt6TestConfig.cmake",
            "lib\cmake\Qt6\Qt6TestConfig.cmake"
        )
        $testFound = $false
        foreach ($testPath in $testPaths) {
            if (Test-Path (Join-Path $QT_PATH $testPath)) {
                $testFound = $true
                break
            }
        }
        if ($testFound) {
            Write-ColorOutput "  ✓ Qt6Test" "Green"
        } else {
            Write-ColorOutput "  ⚠ Qt6Test (optional, for unit tests)" "Yellow"
        }
        
        return $true
    } else {
        Write-ColorOutput "✗ Qt 6 : Not found" "Red"
        return $false
    }
}

# Main checks
Write-ColorOutput "Checking build tools..." "Cyan"
$cmakeOk = Test-Command "cmake" "-version"
if ($cmakeOk) {
    $cmakeVersion = (cmake --version | Select-Object -First 1) -replace 'cmake version ', ''
    $versionParts = $cmakeVersion -split '\.'
    if ([int]$versionParts[0] -lt 3 -or ([int]$versionParts[0] -eq 3 -and [int]$versionParts[1] -lt 20)) {
        Write-ColorOutput "✗ CMake : $cmakeVersion (requires >= 3.20)" "Red"
        $cmakeOk = $false
    }
}

# Check for Visual Studio or MinGW
$compilerFound = $false
if (Test-Path "C:\Program Files\Microsoft Visual Studio") {
    Write-ColorOutput "✓ Visual Studio : Found" "Green"
    $compilerFound = $true
} elseif (Test-Path "C:\Program Files (x86)\Microsoft Visual Studio") {
    Write-ColorOutput "✓ Visual Studio : Found" "Green"
    $compilerFound = $true
} elseif (Get-Command g++ -ErrorAction SilentlyContinue) {
    Write-ColorOutput "✓ MinGW/G++ : Found" "Green"
    $compilerFound = $true
} else {
    Write-ColorOutput "✗ Compiler : Not found (Visual Studio or MinGW required)" "Red"
}
Write-Host ""

Write-ColorOutput "Checking Qt..." "Cyan"
$qtOk = Test-Qt
Write-Host ""

Write-ColorOutput "Checking database..." "Cyan"
$sqliteOk = Test-Command "sqlite3" "-version"
if (-not $sqliteOk) {
    Write-ColorOutput "⚠ SQLite : Not found (may be bundled with Qt)" "Yellow"
}
Write-Host ""

# Summary
Write-ColorOutput "=== Summary ===" "Cyan"
Write-Host ""

$missingCritical = $false

if (-not $cmakeOk) {
    $missingCritical = $true
    Write-ColorOutput "Missing: CMake 3.20+" "Red"
    Write-Host "  Download from: https://cmake.org/download/"
    Write-Host "  Or install via Chocolatey: choco install cmake"
    Write-Host ""
}

if (-not $qtOk) {
    $missingCritical = $true
    Write-ColorOutput "Missing: Qt 6.2+" "Red"
    Write-Host "  Download from: https://www.qt.io/download"
    Write-Host "  Or install via Chocolatey: choco install qtcreator"
    Write-Host "  Note: You need Qt 6.2+ with MSVC or MinGW compiler"
    Write-Host ""
}

if (-not $compilerFound) {
    $missingCritical = $true
    Write-ColorOutput "Missing: C++ Compiler" "Red"
    Write-Host "  Install Visual Studio 2019+ with C++ build tools"
    Write-Host "  Or install MinGW-w64"
    Write-Host ""
}

if (-not $missingCritical) {
    Write-ColorOutput "All critical dependencies are installed!" "Green"
    Write-Host ""
    Write-Host "You can now build SmartBook:"
    Write-Host "  .\build-windows.ps1"
} else {
    Write-ColorOutput "Please install missing dependencies before building." "Yellow"
    exit 1
}
