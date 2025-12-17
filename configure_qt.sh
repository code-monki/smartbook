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
    # macOS
    QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "clang_64" | head -1)
    if [ -z "$QT_PREFIX" ]; then
        QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "macos" | head -1)
    fi
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "gcc_64" | head -1)
    if [ -z "$QT_PREFIX" ]; then
        QT_PREFIX=$(find "$QT_VERSIONS" -type d -name "linux" | head -1)
    fi
else
    echo "Error: Unsupported platform: $OSTYPE"
    exit 1
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
