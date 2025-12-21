#!/bin/bash
# Convert AsciiDoc files to Markdown
# This script uses pandoc for initial conversion, then applies fixes

set -e

DOCS_DIR="Documentation"
OUTPUT_DIR="${DOCS_DIR}_markdown"
BACKUP_DIR="${DOCS_DIR}_backup_$(date +%Y%m%d_%H%M%S)"

echo "Converting AsciiDoc to Markdown..."
echo ""

# Create backup
echo "Creating backup in ${BACKUP_DIR}..."
mkdir -p "${BACKUP_DIR}"
cp -r "${DOCS_DIR}" "${BACKUP_DIR}/"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

# Count files
TOTAL=$(find "${DOCS_DIR}" -name "*.adoc" | wc -l | tr -d ' ')
COUNT=0

# Convert each file
find "${DOCS_DIR}" -name "*.adoc" | while read -r adoc_file; do
    COUNT=$((COUNT + 1))
    rel_path="${adoc_file#${DOCS_DIR}/}"
    md_file="${OUTPUT_DIR}/${rel_path%.adoc}.md"
    md_dir=$(dirname "${md_file}")
    
    mkdir -p "${md_dir}"
    
    echo "[${COUNT}/${TOTAL}] Converting ${rel_path}..."
    
    # Use pandoc to convert
    if pandoc --from=asciidoc --to=markdown "${adoc_file}" -o "${md_file}" 2>/dev/null; then
        echo "  ✓ Converted successfully"
    else
        echo "  ✗ Conversion failed (pandoc issues - will need manual conversion)"
        # Create placeholder
        echo "# ${rel_path}" > "${md_file}"
        echo "" >> "${md_file}"
        echo "> **Note:** This file needs manual conversion from AsciiDoc to Markdown." >> "${md_file}"
    fi
done

echo ""
echo "Conversion complete!"
echo "  - Original files: ${DOCS_DIR}/ (backed up to ${BACKUP_DIR}/)"
echo "  - Markdown files: ${OUTPUT_DIR}/"
echo ""
echo "Next steps:"
echo "  1. Review converted files in ${OUTPUT_DIR}/"
echo "  2. Fix any conversion issues"
echo "  3. Test HTML/PDF generation from Markdown"
echo "  4. Once satisfied, replace ${DOCS_DIR}/ with ${OUTPUT_DIR}/"
