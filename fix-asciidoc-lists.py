#!/usr/bin/env python3
"""
Fix AsciiDoc list rendering issues by ensuring blank lines before lists
that follow bold text or other block elements.
"""

import re
import glob
import sys
from pathlib import Path

def fix_list_spacing(content):
    """Fix list spacing issues in AsciiDoc content."""
    fixes_applied = 0
    
    # Pattern 1: Bold text immediately followed by list (most common)
    # **text:**\n* becomes **text:**\n\n*
    pattern1 = re.compile(r'(\*\*[^*]+\*\*)\n(\*)', re.MULTILINE)
    def replacer1(match):
        nonlocal fixes_applied
        fixes_applied += 1
        return f"{match.group(1)}\n\n{match.group(2)}"
    content = pattern1.sub(replacer1, content)
    
    # Pattern 2: Regular text ending with colon followed by list
    # text:\n* becomes text:\n\n*
    pattern2 = re.compile(r'^([A-Z][^:\n]+:)\n(\*)', re.MULTILINE)
    def replacer2(match):
        nonlocal fixes_applied
        # Only fix if it's not already part of a list or code block
        if not match.group(0).strip().startswith('*'):
            fixes_applied += 1
            return f"{match.group(1)}\n\n{match.group(2)}"
        return match.group(0)
    content = pattern2.sub(replacer2, content)
    
    # Pattern 3: Numbered list (starting with .) after bold
    pattern3 = re.compile(r'(\*\*[^*]+\*\*)\n(\.\s)', re.MULTILINE)
    def replacer3(match):
        nonlocal fixes_applied
        fixes_applied += 1
        return f"{match.group(1)}\n\n{match.group(2)}"
    content = pattern3.sub(replacer3, content)
    
    # Pattern 4: Numbered list (starting with digit) after bold (less common)
    pattern4 = re.compile(r'(\*\*[^*]+\*\*)\n(\d+\.)', re.MULTILINE)
    def replacer4(match):
        nonlocal fixes_applied
        fixes_applied += 1
        return f"{match.group(1)}\n\n{match.group(2)}"
    content = pattern4.sub(replacer4, content)
    
    return content, fixes_applied

def process_file(file_path, dry_run=False):
    """Process a single AsciiDoc file."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            original = f.read()
        
        fixed, count = fix_list_spacing(original)
        
        if count > 0:
            if not dry_run:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(fixed)
            return count
        return 0
    except Exception as e:
        print(f"  Error processing {file_path}: {e}", file=sys.stderr)
        return 0

def main():
    """Main function."""
    import argparse
    
    parser = argparse.ArgumentParser(description='Fix AsciiDoc list rendering issues')
    parser.add_argument('--dry-run', action='store_true', 
                       help='Show what would be fixed without making changes')
    parser.add_argument('--file', type=str,
                       help='Fix a specific file instead of all files')
    args = parser.parse_args()
    
    if args.file:
        files = [args.file]
    else:
        files = glob.glob("Documentation/**/*.adoc", recursive=True)
    
    total_fixes = 0
    files_modified = 0
    
    print(f"Processing {len(files)} file(s)...")
    print("")
    
    for file_path in sorted(files):
        count = process_file(file_path, dry_run=args.dry_run)
        if count > 0:
            files_modified += 1
            total_fixes += count
            action = "Would fix" if args.dry_run else "Fixed"
            print(f"  {action} {count} issue(s) in {file_path}")
    
    print("")
    if args.dry_run:
        print(f"DRY RUN: Would fix {total_fixes} issue(s) in {files_modified} file(s)")
        print("Run without --dry-run to apply fixes")
    else:
        print(f"Fixed {total_fixes} issue(s) in {files_modified} file(s)")
        print("")
        print("Next step: Run 'make docs-html' to regenerate HTML and verify fixes")

if __name__ == '__main__':
    main()
