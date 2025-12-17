#!/bin/bash
# Qt Configuration Helper Script
# Detects Qt installation and sets up environment

QT_HOME="$HOME/Qt"

if [ ! -d "$QT_HOME" ]; then
    echo "Error: Qt not found at $QT_HOME"
    exit 1
fi

# Find Qt version directories
QT_VERSIONS=$(find "$QT_HOME" -maxdepth 1 -type d -name "6.*" | sort -V | tail -1)

if [ -z "$QT_VERSIONS" ]; then
    echo "Error: No Qt 6.x version found in $QT_HOME"
    exit 1
fi

# Detect platform and find appropriate Qt installation
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS: ~/Qt/6.x.x/macos
    QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "macos" | head -1)
    if [ -z "$QT_PREFIX" ]; then
        QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "clang_64" | head -1)
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux: ~/Qt/6.x.x/gcc_64 or ~/Qt/6.x.x/clang_64
    QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "gcc_64" | head -1)
    if [ -z "$QT_PREFIX" ]; then
        QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "clang_64" | head -1)
    fi
    if [ -z "$QT_PREFIX" ]; then
        QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "linux" | head -1)
    fi
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]] || [[ -n "$WINDIR" ]]; then
    # Windows (Git Bash, Cygwin, or native)
    # Try %USERPROFILE%\Qt first, then C:\Qt
    if [ -n "$USERPROFILE" ]; then
        WIN_QT_HOME=$(cygpath -u "$USERPROFILE/Qt" 2>/dev/null || echo "$USERPROFILE/Qt")
    else
        WIN_QT_HOME="C:/Qt"
    fi
    
    if [ -d "$WIN_QT_HOME" ]; then
        QT_VERSIONS=$(find "$WIN_QT_HOME" -maxdepth 1 -type d -name "6.*" | sort -V | tail -1)
    fi
    
    if [ -n "$QT_VERSIONS" ]; then
        # Windows: C:/Qt/6.x.x/msvc2019_64 or C:/Qt/6.x.x/mingw*_64
        QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "msvc*_64" | head -1)
        if [ -z "$QT_PREFIX" ]; then
            QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "mingw*_64" | head -1)
        fi
    fi
else
    echo "Warning: Unsupported platform: $OSTYPE"
    echo "Attempting generic detection..."
    QT_PREFIX=$(find "$QT_VERSIONS" -type d -maxdepth 1 | grep -E "(macos|gcc_64|clang_64|msvc|mingw)" | head -1)
fi

if [ -z "$QT_PREFIX" ]; then
    echo "Error: Could not find Qt installation directory"
    exit 1
fi

echo "Found Qt at: $QT_PREFIX"
echo ""
echo "To use this Qt installation, set:"
echo "  export QTDIR=\"$QT_PREFIX\""
echo "  export CMAKE_PREFIX_PATH=\"$QT_PREFIX\""
echo "  export PATH=\"$QT_PREFIX/bin:\$PATH\""
echo ""
echo "Or source this script:"
echo "  source configure_qt.sh"

export QTDIR="$QT_PREFIX"
export CMAKE_PREFIX_PATH="$QT_PREFIX"
export PATH="$QT_PREFIX/bin:$PATH"

# Check for Qt Test
if [ -f "$QT_PREFIX/lib/cmake/Qt6/Qt6TestConfig.cmake" ] || [ -f "$QT_PREFIX/lib/cmake/Qt6Test/Qt6TestConfig.cmake" ]; then
    echo "✓ Qt6 Test found"
else
    echo "⚠ Qt6 Test not found (tests may not work)"
fi
