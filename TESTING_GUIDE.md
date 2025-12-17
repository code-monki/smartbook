# SmartBook Testing Guide

## Getting Started

### Step 1: Check Dependencies

Before building, verify all required dependencies are installed:

**macOS/Linux:**
```bash
./check-dependencies.sh
```

**Windows:**
```powershell
.\check-dependencies.ps1
```

This will check for:
- CMake 3.20+
- C++ compiler (clang/g++ on Unix, MSVC/MinGW on Windows)
- Qt 6.2+ with required components
- SQLite
- Documentation tools (optional)

Install any missing dependencies before proceeding.

### Step 2: Initial Build

Start with building just the common library to verify the build system works:

**macOS:**
```bash
cd common
make
```

**Linux:**
```bash
cd common
make
```

**Windows:**
```powershell
.\build-windows.ps1 -Target common
```

**Expected Result:** Common library should build successfully. If there are errors, fix them one at a time with atomic commits.

### Step 3: Build All Components

Once common library builds successfully, build everything:

**macOS:**
```bash
make macos
```

**Linux:**
```bash
make linux
```

**Windows:**
```powershell
.\build-windows.ps1
```

**Expected Result:** All components (common, reader, creator) should build.

### Step 4: Run Basic Tests

Test the build system and basic functionality:

**macOS/Linux:**
```bash
make test-build
```

**Windows:**
```powershell
.\build-windows.ps1 -Target test
```

**Expected Result:** Test suite should compile (may not pass yet, but should build).

### Step 5: Try Running Applications

Attempt to run the applications (they may not work fully yet, but should launch):

**macOS/Linux:**
```bash
make run-reader
# or
make run-creator
```

**Windows:**
```powershell
.\build-windows.ps1 -Target run-reader
# or
.\build-windows.ps1 -Target run-creator
```

**Expected Result:** Applications should launch (may show empty windows or errors, but should not crash immediately).

## Testing Workflow

### For Each Issue Found:

1. **Identify the Problem**
   - What component? (common, reader, creator)
   - What's the error message?
   - Can you reproduce it consistently?

2. **Create Atomic Fix**
   - Fix ONE thing at a time
   - Test the fix in isolation
   - Commit with descriptive message:
     ```
     fix(<component>): <brief description>
     
     <detailed explanation>
     ```

3. **Verify Fix**
   - Rebuild affected component
   - Test the specific functionality
   - Ensure no regressions

4. **Commit**
   ```bash
   git add <changed-files>
   git commit  # Will use template
   ```

### Example Workflow:

```bash
# 1. Build and find error
make macos
# Error: Missing include in LocalDBManager.cpp

# 2. Fix the issue
# Edit common/src/database/LocalDBManager.cpp
# Add missing #include

# 3. Test the fix
cd common
make clean
make
# Should build successfully

# 4. Commit atomically
git add common/src/database/LocalDBManager.cpp
git commit
# Enter: fix(common): Add missing QSqlQuery include in LocalDBManager
```

## Common Issues to Watch For

### Build Issues:
- Missing includes
- Incorrect CMake configuration
- Qt component not found
- Linker errors
- Platform-specific issues

### Runtime Issues:
- Application crashes on startup
- Missing Qt plugins
- Database initialization failures
- Path resolution problems

### Test Issues:
- Tests don't compile
- Tests fail
- Missing test dependencies

## Testing Priorities

### Phase 1: Build System
1. ✅ Dependencies check
2. ✅ Common library builds
3. ✅ Reader builds
4. ✅ Creator builds
5. ✅ Tests build

### Phase 2: Basic Functionality
1. Applications launch without crashing
2. Database initialization works
3. Basic UI displays
4. No immediate crashes

### Phase 3: Core Features
1. Library management
2. Cartridge loading
3. Content display
4. Form handling

## Atomic Commit Examples

### Good Atomic Commits:
```bash
fix(common): Add missing QSqlQuery include in LocalDBManager.cpp

The LocalDBManager was using QSqlQuery without including
the necessary header, causing compilation errors.

fix(reader): Resolve null pointer in LibraryManager constructor

Added null check before accessing QApplication instance.

feat(common): Implement basic database schema creation

Added SQL schema for Local_Library_Manifest table with
proper indexes and constraints.

test(common): Add unit tests for LocalDBManager

Added test cases for:
- Database initialization
- Schema creation
- Query execution
```

### Avoid:
```bash
# Bad: Multiple unrelated changes
fix(common): Fix includes and update reader UI and add tests

# Good: Separate commits
fix(common): Add missing includes
feat(reader): Update library view UI
test(common): Add LocalDBManager tests
```

## Next Steps After Initial Build

Once you have a successful build:

1. **Document Build Issues**
   - Create issues or notes for any problems encountered
   - Document workarounds

2. **Start Implementing TODOs**
   - The codebase has TODO markers for incomplete features
   - Implement them one at a time with atomic commits

3. **Add Unit Tests**
   - Start with common library components
   - Test database operations
   - Test utility functions

4. **Test Platform-Specific Features**
   - Path resolution
   - Platform utilities
   - Security features

## Getting Help

If you encounter issues:

1. Check the error message carefully
2. Review the relevant source files
3. Check CMake configuration
4. Verify dependencies are correct
5. Look for similar issues in the codebase
6. Commit any fixes atomically

## Quick Reference

```bash
# Check dependencies
./check-dependencies.sh          # Unix/macOS
.\check-dependencies.ps1         # Windows

# Build
make macos                       # macOS
make linux                       # Linux
.\build-windows.ps1              # Windows

# Run
make run-reader                  # Unix/macOS
.\build-windows.ps1 -Target run-reader  # Windows

# Test
make test-build                  # Unix/macOS
.\build-windows.ps1 -Target test # Windows

# Clean
make clean                       # Unix/macOS
# Windows: Delete build\ directory
```

Good luck with testing! Remember: one fix, one commit, one test at a time.
