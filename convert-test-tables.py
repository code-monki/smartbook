#!/usr/bin/env python3
"""
Convert test case tables from wide format to individual two-column tables.
Each test case becomes its own table with label/content format.
"""

import re
import sys

def parse_table_row(line):
    """Parse a table row, handling multi-line cells."""
    # Remove leading/trailing pipes and split
    line = line.strip()
    if not line.startswith('|'):
        return None
    
    # Remove leading | and trailing |
    line = line[1:].rstrip('|').strip()
    
    # Split by | but be careful with escaped pipes
    # Simple split for now - AsciiDoc uses | as separator
    cells = [cell.strip() for cell in line.split('|')]
    
    # Filter out empty cells from leading/trailing separators
    cells = [c for c in cells if c]
    
    return cells

def is_test_case_table_start(line):
    """Check if this is the start of a test case table."""
    # Look for table with Test Case ID header
    return 'Test Case ID' in line and 'cols=' in line

def convert_test_case_table(content):
    """Convert a test case table from wide format to individual tables."""
    lines = content.split('\n')
    result = []
    i = 0
    
    while i < len(lines):
        line = lines[i]
        
        # Check if this is a test case table
        if is_test_case_table_start(line):
            # Find the table start marker
            table_start_idx = i
            cols_line = line
            
            # Find the header row (next line with ^.^)
            i += 1
            header_line = lines[i] if i < len(lines) else ""
            
            # Find the |=== start
            while i < len(lines) and not lines[i].strip().startswith('|==='):
                i += 1
            
            if i >= len(lines):
                result.append(line)
                break
            
            # Now collect all test case rows until |===
            test_cases = []
            i += 1  # Skip the |=== line
            
            current_row = []
            in_cell = False
            
            while i < len(lines):
                line = lines[i]
                stripped = line.strip()
                
                # End of table
                if stripped == '|===':
                    if current_row:
                        test_cases.append(current_row)
                    break
                
                # Empty line - might be part of cell content
                if not stripped:
                    if current_row:
                        # Add blank line to last cell
                        if len(current_row) > 0:
                            current_row[-1] += '\n'
                    i += 1
                    continue
                
                # Table row
                if stripped.startswith('|'):
                    # Parse the row
                    cells = parse_table_row(line)
                    if cells and len(cells) >= 4:
                        # This is a test case row
                        if current_row:
                            test_cases.append(current_row)
                        current_row = cells
                    elif cells:
                        # Might be continuation of previous row
                        if current_row and len(current_row) < 4:
                            # Merge with previous
                            for j, cell in enumerate(cells):
                                if j < len(current_row):
                                    current_row[j] += ' ' + cell
                                else:
                                    current_row.append(cell)
                        else:
                            current_row = cells
                else:
                    # Continuation line - add to last cell
                    if current_row:
                        current_row[-1] += ' ' + stripped
                
                i += 1
            
            # Convert test cases to individual tables
            for test_case in test_cases:
                if len(test_case) >= 4:
                    test_id = test_case[0].strip()
                    requirement = test_case[1].strip()
                    test_steps = test_case[2].strip()
                    expected_result = test_case[3].strip()
                    
                    # Create two-column table
                    result.append('')
                    result.append('[cols="1,4"]')
                    result.append('|===')
                    result.append(f'| Test Case ID | {test_id}')
                    result.append(f'| Requirement Covered | {requirement}')
                    result.append('| Test Steps |')
                    # Preserve test steps formatting (may have lists)
                    for step_line in test_steps.split('\n'):
                        if step_line.strip():
                            result.append(step_line)
                    result.append(f'| Expected Result (Acceptance Criteria) | {expected_result}')
                    result.append('|===')
                    result.append('')
            
            i += 1  # Skip the closing |===
            continue
        
        # Not a test case table - keep as is
        result.append(line)
        i += 1
    
    return '\n'.join(result)

def main():
    """Main function."""
    if len(sys.argv) < 2:
        print("Usage: convert-test-tables.py <input.adoc> [output.adoc]")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else input_file
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    converted = convert_test_case_table(content)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(converted)
    
    print(f"Converted {input_file} -> {output_file}")

if __name__ == '__main__':
    main()
