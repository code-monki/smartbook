#!/bin/bash
# inspect-build.sh - Comprehensive build output inspection
# Usage: ./inspect-build.sh [build-output.log]

BUILD_LOG="${1:-build-output.log}"

if [ ! -f "$BUILD_LOG" ]; then
    echo "Error: Build log file '$BUILD_LOG' not found"
    echo "Usage: $0 [build-output.log]"
    exit 1
fi

echo "=== Build Inspection Report ==="
echo "File: $BUILD_LOG"
echo ""

# Count errors
ERROR_COUNT=$(grep -icE "error|failed|fatal" "$BUILD_LOG" 2>/dev/null || echo "0")
echo "=== ERRORS ==="
echo "Total errors found: $ERROR_COUNT"
if [ "$ERROR_COUNT" -gt 0 ]; then
    echo ""
    echo "Error details (first 30):"
    grep -iE "error|failed|fatal" "$BUILD_LOG" | head -30
    echo ""
fi

# Count warnings
WARNING_COUNT=$(grep -ic "warning" "$BUILD_LOG" 2>/dev/null || echo "0")
echo "=== WARNINGS ==="
echo "Total warnings found: $WARNING_COUNT"
if [ "$WARNING_COUNT" -gt 0 ]; then
    echo ""
    echo "Warning details (first 30):"
    grep -i "warning" "$BUILD_LOG" | head -30
    echo ""
fi

# Check for specific warning types
echo "=== WARNING CATEGORIES ==="
UNUSED_COUNT=$(grep -ic "unused" "$BUILD_LOG" 2>/dev/null || echo "0")
if [ "$UNUSED_COUNT" -gt 0 ]; then
    echo "  - Unused variables/parameters: $UNUSED_COUNT"
fi

DEPRECATED_COUNT=$(grep -ic "deprecated" "$BUILD_LOG" 2>/dev/null || echo "0")
if [ "$DEPRECATED_COUNT" -gt 0 ]; then
    echo "  - Deprecated API usage: $DEPRECATED_COUNT"
fi

IMPLICIT_COUNT=$(grep -ic "implicit" "$BUILD_LOG" 2>/dev/null || echo "0")
if [ "$IMPLICIT_COUNT" -gt 0 ]; then
    echo "  - Implicit conversions: $IMPLICIT_COUNT"
fi

CAST_COUNT=$(grep -icE "cast|conversion" "$BUILD_LOG" 2>/dev/null || echo "0")
if [ "$CAST_COUNT" -gt 0 ]; then
    echo "  - Type casts/conversions: $CAST_COUNT"
fi

MOC_COUNT=$(grep -ic "moc" "$BUILD_LOG" 2>/dev/null || echo "0")
if [ "$MOC_COUNT" -gt 0 ]; then
    echo "  - Qt MOC issues: $MOC_COUNT"
fi

echo ""

# Summary
echo "=== SUMMARY ==="
if [ "$ERROR_COUNT" -eq 0 ] && [ "$WARNING_COUNT" -eq 0 ]; then
    echo "✓ Build clean - no errors or warnings"
    exit 0
elif [ "$ERROR_COUNT" -gt 0 ]; then
    echo "✗ Build FAILED - $ERROR_COUNT error(s) found"
    exit 1
else
    echo "⚠ Build has $WARNING_COUNT warning(s) - these should be fixed"
    echo "  (Warnings are code smells indicating potential defects)"
    exit 1
fi
