#!/usr/bin/env python3
"""
Convert test case tables from wide format to individual two-column tables.
Each test case becomes its own table with label/content format.
"""

import re
import sys

def convert_test_case_table(content):
    """Convert test case tables from wide format to individual tables."""
    lines = content.split('\n')
    result = []
    i = 0
    
    while i < len(lines):
        line = lines[i]
        
        # Check if this is a test case table (has "Test Case ID" in header)
        if 'Test Case ID' in line and ('cols=' in line or '|===' in lines[i+1] if i+1 < len(lines) else False):
            # This is a test case table section
            # Find the table boundaries
            table_start = i
            
            # Find the |=== that starts the table
            while i < len(lines) and not lines[i].strip().startswith('|==='):
                i += 1
            
            if i >= len(lines):
                result.append(line)
                break
            
            table_start_marker = i
            i += 1  # Skip |===
            
            # Skip header row (starts with ^.^)
            if i < len(lines) and '^.^' in lines[i]:
                i += 1
            
            # Skip blank line after header
            if i < len(lines) and not lines[i].strip():
                i += 1
            
            # Now collect test case rows until we hit |===
            test_cases = []
            current_case = None
            current_cell_idx = 0
            collecting_cell = None
            
            while i < len(lines):
                line = lines[i]
                stripped = line.strip()
                
                # End of table
                if stripped == '|===':
                    if current_case:
                        test_cases.append(current_case)
                    break
                
                # Table row starts with |
                if stripped.startswith('|'):
                    # Parse the row
                    # Format: | cell1 | cell2 | format| cell3 | cell4 |
                    # The 'a|' indicates AsciiDoc format for that cell
                    
                    # Remove leading | and split
                    parts = stripped[1:].split('|')
                    parts = [p.strip() for p in parts if p.strip()]
                    
                    # Check if this starts a new test case (has **T- in first cell)
                    if parts and '**T-' in parts[0]:
                        # Save previous case if exists
                        if current_case:
                            test_cases.append(current_case)
                        
                        # Start new case
                        current_case = {
                            'id': parts[0] if len(parts) > 0 else '',
                            'requirement': parts[1] if len(parts) > 1 else '',
                            'steps': '',
                            'expected': ''
                        }
                        current_cell_idx = 2
                        
                        # Check if steps start on same line
                        if len(parts) > 2:
                            # Might be 'a|' format specifier
                            if parts[2] == 'a':
                                # Steps start on next lines
                                current_cell_idx = 2
                            else:
                                # Steps might be in this row
                                current_case['steps'] = parts[2] if len(parts) > 2 else ''
                                current_case['expected'] = parts[3] if len(parts) > 3 else ''
                                current_cell_idx = 4
                    else:
                        # Continuation of current case
                        if current_case:
                            # This might be continuation of steps or expected
                            if current_cell_idx == 2:  # Collecting steps
                                # Check if this line has content for steps
                                if parts:
                                    if current_case['steps']:
                                        current_case['steps'] += '\n' + ' '.join(parts)
                                    else:
                                        current_case['steps'] = ' '.join(parts)
                            elif current_cell_idx >= 3:  # Collecting expected
                                if parts:
                                    if current_case['expected']:
                                        current_case['expected'] += ' ' + ' '.join(parts)
                                    else:
                                        current_case['expected'] = ' '.join(parts)
                else:
                    # Continuation line (not starting with |)
                    if current_case:
                        if current_cell_idx == 2:  # Steps
                            current_case['steps'] += '\n' + stripped
                        elif current_cell_idx >= 3:  # Expected
                            current_case['expected'] += ' ' + stripped
                
                i += 1
            
            # Add last case
            if current_case:
                test_cases.append(current_case)
            
            # Convert test cases to individual tables
            for test_case in test_cases:
                result.append('')
                result.append('[cols="1,4"]')
                result.append('|===')
                result.append(f'| Test Case ID | {test_case["id"]}')
                result.append(f'| Requirement Covered | {test_case["requirement"]}')
                result.append('| Test Steps |')
                # Preserve formatting of test steps
                if test_case['steps']:
                    for step_line in test_case['steps'].split('\n'):
                        if step_line.strip():
                            result.append(step_line)
                result.append(f'| Expected Result (Acceptance Criteria) | {test_case["expected"]}')
                result.append('|===')
                result.append('')
            
            i += 1  # Skip closing |===
            continue
        
        # Regular line - keep as is
        result.append(line)
        i += 1
    
    return '\n'.join(result)

def main():
    """Main function."""
    if len(sys.argv) < 2:
        print("Usage: convert-test-tables-v2.py <input.adoc> [output.adoc]")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else input_file + '.new'
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    converted = convert_test_case_table(content)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(converted)
    
    print(f"Converted {input_file}")
    print(f"Output: {output_file}")
    print(f"\nReview the output and if correct, replace original with:")
    print(f"  mv {output_file} {input_file}")

if __name__ == '__main__':
    main()
