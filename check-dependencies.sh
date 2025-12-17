#!/bin/bash
# SmartBook Dependency Checker
# Checks for required dependencies and provides installation guidance

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Platform detection
UNAME_S=$(uname -s)
UNAME_M=$(uname -m)

# Normalize architecture
if [ "$UNAME_M" = "x86_64" ] || [ "$UNAME_M" = "amd64" ]; then
    ARCH="x86_64"
elif [ "$UNAME_M" = "aarch64" ] || [ "$UNAME_M" = "arm64" ]; then
    ARCH="arm64"
else
    ARCH="$UNAME_M"
fi

echo -e "${BLUE}=== SmartBook Dependency Checker ===${NC}\n"

# Function to check command
check_command() {
    if command -v "$1" >/dev/null 2>&1; then
        VERSION=$($1 --version 2>/dev/null | head -1)
        echo -e "${GREEN}✓${NC} $1: $VERSION"
        return 0
    else
        echo -e "${RED}✗${NC} $1: Not found"
        return 1
    fi
}

# Function to check Qt
check_qt() {
    QT_FOUND=false
    QT_PATH=""
    QT_VERSION=""
    
    # Check ~/Qt
    if [ -d "$HOME/Qt" ]; then
        if [ "$UNAME_S" = "Darwin" ]; then
            QT_PATH=$(find "$HOME/Qt" -type d -path "*/6.*/macos" 2>/dev/null | sort -V | tail -1)
        elif [ "$UNAME_S" = "Linux" ]; then
            QT_PATH=$(find "$HOME/Qt" -type d \( -name "gcc_64" -o -name "clang_64" \) 2>/dev/null | head -1)
        fi
        
        if [ -n "$QT_PATH" ] && [ -f "$QT_PATH/bin/qmake" ]; then
            QT_FOUND=true
            QT_VERSION=$("$QT_PATH/bin/qmake" -v 2>/dev/null | grep "Using Qt version" | awk '{print $4}' || echo "Unknown")
        fi
    fi
    
    # Check QTDIR
    if [ -z "$QT_PATH" ] && [ -n "$QTDIR" ] && [ -f "$QTDIR/bin/qmake" ]; then
        QT_FOUND=true
        QT_PATH="$QTDIR"
        QT_VERSION=$("$QTDIR/bin/qmake" -v 2>/dev/null | grep "Using Qt version" | awk '{print $4}' || echo "Unknown")
    fi
    
    # Check system Qt (Linux)
    if [ "$UNAME_S" = "Linux" ] && [ -z "$QT_PATH" ]; then
        if command -v qmake >/dev/null 2>&1; then
            QT_VERSION=$(qmake -v 2>/dev/null | grep "Using Qt version" | awk '{print $4}' || echo "Unknown")
            if echo "$QT_VERSION" | grep -q "^6\."; then
                QT_FOUND=true
                QT_PATH=$(which qmake | sed 's|/bin/qmake||')
            fi
        fi
    fi
    
    if [ "$QT_FOUND" = true ]; then
        echo -e "${GREEN}✓${NC} Qt 6: $QT_VERSION (at $QT_PATH)"
        
        # Check Qt components
        if [ -f "$QT_PATH/lib/cmake/Qt6Core/Qt6CoreConfig.cmake" ]; then
            echo -e "  ${GREEN}✓${NC} Qt6Core"
        else
            echo -e "  ${RED}✗${NC} Qt6Core"
        fi
        
        if [ -f "$QT_PATH/lib/cmake/Qt6Gui/Qt6GuiConfig.cmake" ]; then
            echo -e "  ${GREEN}✓${NC} Qt6Gui"
        else
            echo -e "  ${RED}✗${NC} Qt6Gui"
        fi
        
        if [ -f "$QT_PATH/lib/cmake/Qt6Widgets/Qt6WidgetsConfig.cmake" ]; then
            echo -e "  ${GREEN}✓${NC} Qt6Widgets"
        else
            echo -e "  ${RED}✗${NC} Qt6Widgets"
        fi
        
        if [ -f "$QT_PATH/lib/cmake/Qt6WebEngine/Qt6WebEngineConfig.cmake" ]; then
            echo -e "  ${GREEN}✓${NC} Qt6WebEngine"
        else
            echo -e "  ${YELLOW}⚠${NC} Qt6WebEngine (required for Reader/Creator)"
        fi
        
        if [ -f "$QT_PATH/lib/cmake/Qt6WebChannel/Qt6WebChannelConfig.cmake" ]; then
            echo -e "  ${GREEN}✓${NC} Qt6WebChannel"
        else
            echo -e "  ${YELLOW}⚠${NC} Qt6WebChannel (required for Reader/Creator)"
        fi
        
        if [ -f "$QT_PATH/lib/cmake/Qt6Sql/Qt6SqlConfig.cmake" ]; then
            echo -e "  ${GREEN}✓${NC} Qt6Sql"
        else
            echo -e "  ${RED}✗${NC} Qt6Sql"
        fi
        
        # Check Qt Test
        if [ -f "$QT_PATH/lib/cmake/Qt6Test/Qt6TestConfig.cmake" ] || \
           [ -f "$QT_PATH/lib/cmake/Qt6/Qt6TestConfig.cmake" ]; then
            echo -e "  ${GREEN}✓${NC} Qt6Test"
        else
            echo -e "  ${YELLOW}⚠${NC} Qt6Test (optional, for unit tests)"
        fi
        
        return 0
    else
        echo -e "${RED}✗${NC} Qt 6: Not found"
        return 1
    fi
}

# Function to check SQLite
check_sqlite() {
    if command -v sqlite3 >/dev/null 2>&1; then
        VERSION=$(sqlite3 --version | awk '{print $1}')
        echo -e "${GREEN}✓${NC} SQLite: $VERSION"
        return 0
    else
        echo -e "${YELLOW}⚠${NC} SQLite: Not found (may be bundled with Qt)"
        return 1
    fi
}

# Function to check CMake
check_cmake() {
    if command -v cmake >/dev/null 2>&1; then
        VERSION=$(cmake --version | head -1 | awk '{print $3}')
        # Check version >= 3.20
        MAJOR=$(echo "$VERSION" | cut -d. -f1)
        MINOR=$(echo "$VERSION" | cut -d. -f2)
        if [ "$MAJOR" -gt 3 ] || ([ "$MAJOR" -eq 3 ] && [ "$MINOR" -ge 20 ]); then
            echo -e "${GREEN}✓${NC} CMake: $VERSION"
            return 0
        else
            echo -e "${RED}✗${NC} CMake: $VERSION (requires >= 3.20)"
            return 1
        fi
    else
        echo -e "${RED}✗${NC} CMake: Not found"
        return 1
    fi
}

# Function to check compiler
check_compiler() {
    if [ "$UNAME_S" = "Darwin" ]; then
        if command -v clang >/dev/null 2>&1; then
            VERSION=$(clang --version | head -1)
            echo -e "${GREEN}✓${NC} Clang: $VERSION"
            return 0
        else
            echo -e "${RED}✗${NC} Clang: Not found (install Xcode Command Line Tools)"
            return 1
        fi
    elif [ "$UNAME_S" = "Linux" ]; then
        if command -v g++ >/dev/null 2>&1; then
            VERSION=$(g++ --version | head -1)
            echo -e "${GREEN}✓${NC} G++: $VERSION"
            return 0
        else
            echo -e "${RED}✗${NC} G++: Not found"
            return 1
        fi
    fi
}

# Function to check documentation tools
check_docs() {
    DOCS_OK=true
    
    if command -v asciidoctor >/dev/null 2>&1; then
        VERSION=$(asciidoctor --version | head -1)
        echo -e "${GREEN}✓${NC} Asciidoctor: $VERSION"
    else
        echo -e "${YELLOW}⚠${NC} Asciidoctor: Not found (optional, for documentation)"
        DOCS_OK=false
    fi
    
    if command -v asciidoctor-pdf >/dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} Asciidoctor PDF: Installed"
    else
        echo -e "${YELLOW}⚠${NC} Asciidoctor PDF: Not found (optional, for PDF docs)"
        DOCS_OK=false
    fi
    
    if command -v mmdc >/dev/null 2>&1; then
        VERSION=$(mmdc --version 2>/dev/null || echo "installed")
        echo -e "${GREEN}✓${NC} Mermaid CLI: $VERSION"
    else
        echo -e "${YELLOW}⚠${NC} Mermaid CLI: Not found (optional, for diagram conversion)"
        DOCS_OK=false
    fi
    
    return 0
}

# Main checks
echo -e "${BLUE}Checking build tools...${NC}"
check_cmake
check_compiler
echo ""

echo -e "${BLUE}Checking Qt...${NC}"
QT_OK=false
if check_qt; then
    QT_OK=true
fi
echo ""

echo -e "${BLUE}Checking database...${NC}"
check_sqlite
echo ""

echo -e "${BLUE}Checking documentation tools (optional)...${NC}"
check_docs
echo ""

# Summary and installation guidance
echo -e "${BLUE}=== Summary ===${NC}\n"

MISSING_CRITICAL=false

if ! check_cmake >/dev/null 2>&1; then
    MISSING_CRITICAL=true
    echo -e "${RED}Missing: CMake 3.20+${NC}"
    if [ "$UNAME_S" = "Darwin" ]; then
        echo "  Install: brew install cmake"
    elif [ "$UNAME_S" = "Linux" ]; then
        echo "  Install: sudo apt-get install cmake  # Debian/Ubuntu"
        echo "           sudo dnf install cmake      # Fedora"
        echo "           sudo pacman -S cmake        # Arch"
    fi
    echo ""
fi

if [ "$QT_OK" = false ]; then
    MISSING_CRITICAL=true
    echo -e "${RED}Missing: Qt 6.2+${NC}"
    echo "  Download from: https://www.qt.io/download"
    echo "  Or install via package manager:"
    if [ "$UNAME_S" = "Darwin" ]; then
        echo "    brew install qt@6"
    elif [ "$UNAME_S" = "Linux" ]; then
        echo "    sudo apt-get install qt6-base-dev qt6-webengine-dev qt6-webchannel-dev qt6-sql-dev  # Debian/Ubuntu"
        echo "    sudo dnf install qt6-qtbase-devel qt6-qtwebengine-devel qt6-qtwebchannel-devel qt6-qtsql-devel  # Fedora"
        echo "    sudo pacman -S qt6-base qt6-webengine qt6-webchannel qt6-sql  # Arch"
    fi
    echo ""
fi

if [ "$MISSING_CRITICAL" = false ]; then
    echo -e "${GREEN}All critical dependencies are installed!${NC}"
    echo ""
    echo "You can now build SmartBook:"
    if [ "$UNAME_S" = "Darwin" ]; then
        echo "  make macos"
    elif [ "$UNAME_S" = "Linux" ]; then
        echo "  make linux"
    fi
else
    echo -e "${YELLOW}Please install missing dependencies before building.${NC}"
    exit 1
fi
