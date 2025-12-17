#!/usr/bin/env python3
"""
Comprehensive audit of all lists in AsciiDoc files to find missing blank lines.
Table cells are excluded as they handle lists differently.
"""

import re
import glob
from pathlib import Path

def is_in_table_cell(line_num, content_lines):
    """Check if a line is within a table cell (between |==| markers)."""
    # Look backwards to find if we're in a table
    in_table = False
    for i in range(line_num - 1, -1, -1):
        line = content_lines[i].strip()
        if line.startswith('|==='):
            in_table = True
            break
        elif line.startswith('|===') or (line.startswith('|') and '|===' in line):
            break
    
    # Look forwards to see if we're still in table
    if in_table:
        for i in range(line_num, len(content_lines)):
            line = content_lines[i].strip()
            if line.startswith('|==='):
                return True
            if not line.startswith('|') and line and not line.startswith('a|'):
                return False
    
    return False

def audit_lists(file_path):
    """Audit a single file for list issues."""
    issues = []
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            content = ''.join(lines)
        
        # Pattern 1: List immediately after non-blank line (not in table)
        # Check for bullet lists
        pattern1 = re.compile(r'^(.+)\n(\*[^*\s])', re.MULTILINE)
        for match in pattern1.finditer(content):
            start_pos = match.start()
            line_num = content[:start_pos].count('\n') + 1
            prev_line = match.group(1).strip()
            list_line = match.group(2)
            
            # Skip if previous line is blank or is a list item itself
            if not prev_line or prev_line.startswith('*') or prev_line.startswith('.') or prev_line.startswith('|'):
                continue
            
            # Skip if in table cell
            if is_in_table_cell(line_num - 1, lines):
                continue
            
            # Skip if previous line ends with continuation marker
            if prev_line.endswith('+'):
                continue
            
            issues.append({
                'line': line_num,
                'type': 'bullet',
                'context': prev_line[:60] + '...' if len(prev_line) > 60 else prev_line,
                'list_start': list_line[:40]
            })
        
        # Pattern 2: Numbered list (starting with .) immediately after non-blank line
        pattern2 = re.compile(r'^(.+)\n(\.\s)', re.MULTILINE)
        for match in pattern2.finditer(content):
            start_pos = match.start()
            line_num = content[:start_pos].count('\n') + 1
            prev_line = match.group(1).strip()
            list_line = match.group(2)
            
            # Skip if previous line is blank or is a list item itself
            if not prev_line or prev_line.startswith('*') or prev_line.startswith('.') or prev_line.startswith('|'):
                continue
            
            # Skip if in table cell
            if is_in_table_cell(line_num - 1, lines):
                continue
            
            # Skip if previous line ends with continuation marker
            if prev_line.endswith('+'):
                continue
            
            issues.append({
                'line': line_num,
                'type': 'numbered',
                'context': prev_line[:60] + '...' if len(prev_line) > 60 else prev_line,
                'list_start': list_line[:40]
            })
        
        # Pattern 3: Numbered list (starting with digit) immediately after non-blank line
        pattern3 = re.compile(r'^(.+)\n(\d+\.\s)', re.MULTILINE)
        for match in pattern3.finditer(content):
            start_pos = match.start()
            line_num = content[:start_pos].count('\n') + 1
            prev_line = match.group(1).strip()
            list_line = match.group(2)
            
            # Skip if previous line is blank or is a list item itself
            if not prev_line or prev_line.startswith('*') or prev_line.startswith('.') or prev_line.startswith('|') or prev_line[0].isdigit():
                continue
            
            # Skip if in table cell
            if is_in_table_cell(line_num - 1, lines):
                continue
            
            # Skip if previous line ends with continuation marker
            if prev_line.endswith('+'):
                continue
            
            issues.append({
                'line': line_num,
                'type': 'numbered_digit',
                'context': prev_line[:60] + '...' if len(prev_line) > 60 else prev_line,
                'list_start': list_line[:40]
            })
        
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
    
    return issues

def main():
    """Main function."""
    files = glob.glob("Documentation/**/*.adoc", recursive=True)
    
    total_issues = 0
    files_with_issues = []
    
    print("Auditing all lists in documentation files...")
    print("(Table cells are excluded as they handle lists correctly)\n")
    
    for file_path in sorted(files):
        issues = audit_lists(file_path)
        if issues:
            files_with_issues.append((file_path, issues))
            total_issues += len(issues)
            print(f"{file_path}: {len(issues)} issue(s)")
            for issue in issues[:5]:  # Show first 5
                print(f"  Line {issue['line']}: {issue['type']} list after '{issue['context']}'")
            if len(issues) > 5:
                print(f"  ... and {len(issues) - 5} more")
            print()
    
    print(f"\nSUMMARY:")
    print(f"  Files with issues: {len(files_with_issues)}")
    print(f"  Total issues: {total_issues}")
    
    if files_with_issues:
        print(f"\nRun 'python3 fix-asciidoc-lists.py' to fix all issues automatically")

if __name__ == '__main__':
    main()
