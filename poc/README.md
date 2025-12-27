# Content Rendering Architecture Proof of Concept

This directory contains proof-of-concept applications demonstrating the migration from Qt WebEngine to QTextDocument + Qt Quick/QML architecture.

## PoC Applications

### 1. `poc_qtextbrowser_rendering`

**Purpose:** Demonstrate QTextBrowser content rendering with theme changes.

**Features:**
- HTML4 + CSS 2.1 content rendering
- Dynamic theme changes (light, dark, sepia)
- Font size and family controls
- **Key Test:** Verify no rendering flash on theme changes

**Build:**
```bash
cd build
cmake -DBUILD_POC=ON ..
make poc_qtextbrowser_rendering
```

**Run:**
```bash
./poc_qtextbrowser_rendering
```

**What to Test:**
1. Change themes using the dropdown - observe no flash
2. Change font size - verify smooth updates
3. Click "Test Theme Flash" - rapidly cycles themes to verify no flash
4. Verify HTML4 content renders correctly

### 2. `poc_qml_embedding`

**Purpose:** Demonstrate QML embedded applications with C++ bridge.

**Features:**
- QML application embedded in Qt Widgets
- C++/QML communication via properties
- Bridge object (`QmlAppBridge`) exposed to QML
- Form data save/load simulation

**Build:**
```bash
cd build
cmake -DBUILD_POC=ON ..
make poc_qml_embedding
```

**Run:**
```bash
./poc_qml_embedding
```

**What to Test:**
1. Click "Load QML App" - QML app should appear on right side
2. Enter data in form fields
3. Click "Save Data" - verify console output
4. Click "Load Data" - verify data loads into fields
5. Click "Test Bridge" - verify C++ to QML communication
6. Verify QML app renders correctly alongside QTextBrowser content

## Building

### Option 1: Build with main project

```bash
cd /path/to/SmartBook
mkdir -p build && cd build
cmake -DBUILD_POC=ON ..
make poc_qtextbrowser_rendering poc_qml_embedding
```

### Option 2: Build standalone

```bash
cd poc
mkdir -p build && cd build
cmake ..
make
```

## Dependencies

- Qt 6.x (Core, Widgets, Quick, QuickWidgets)
- C++17 compiler
- CMake 3.20+

## Success Criteria

### QTextBrowser PoC
- ✅ Content renders correctly (HTML4 + CSS 2.1)
- ✅ Theme changes apply instantly
- ✅ **No visible rendering flash on theme changes**
- ✅ Font changes apply correctly

### QML Embedding PoC
- ✅ QML app loads and renders correctly
- ✅ C++ bridge exposed to QML
- ✅ QML can call C++ methods
- ✅ C++ signals received in QML
- ✅ QML app integrates with Qt Widgets layout

## Next Steps

After verifying both PoCs work correctly:

1. Proceed with Phase 1: Reader View Migration
2. Integrate QTextBrowser into `ReaderView`
3. Integrate QML embedding into content rendering
4. Replace WebChannelBridge with QmlAppBridge

## Notes

- These PoCs are simplified versions for demonstration
- Production implementation will be more robust
- Error handling and edge cases will be addressed in full implementation

