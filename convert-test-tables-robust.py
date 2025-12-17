#!/usr/bin/env python3
"""
Robust converter for test case tables from wide format to individual two-column tables.
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
        
        # Check if this is a test case table (cols= line with Test Case ID in next few lines)
        if 'cols=' in line and i+2 < len(lines) and 'Test Case ID' in lines[i+2]:
            # This is a test case table section
            result.append(line)  # Keep the cols= line for now, we'll replace the table
            i += 1
            
            # Find and skip the |=== start
            while i < len(lines) and not lines[i].strip().startswith('|==='):
                i += 1
            
            if i >= len(lines):
                break
            
            # Skip |=== and header row
            i += 1  # Skip |===
            if i < len(lines) and '^.^' in lines[i]:
                i += 1  # Skip header
            if i < len(lines) and not lines[i].strip():
                i += 1  # Skip blank line
            
            # Collect test cases
            test_cases = []
            
            while i < len(lines):
                line = lines[i]
                stripped = line.strip()
                
                # End of table
                if stripped == '|===':
                    i += 1
                    break
                
                # New test case row starts with | **T-
                if stripped.startswith('|') and '**T-' in stripped:
                    # Parse: | **T-XXX** | requirement | a|
                    cell_content = stripped[1:].strip()
                    # Remove trailing | if present
                    if cell_content.endswith('|'):
                        cell_content = cell_content[:-1].strip()
                    
                    parts = [p.strip() for p in cell_content.split('|') if p.strip()]
                    
                    test_id = parts[0] if len(parts) > 0 else ''
                    requirement = parts[1] if len(parts) > 1 else ''
                    # Remove trailing 'a' format specifier
                    requirement = requirement.rstrip(' a').strip()
                    
                    # Steps start on next lines - collect until we hit a line with expected result
                    i += 1
                    steps_lines = []
                    expected = ''
                    
                    while i < len(lines):
                        next_line = lines[i]
                        next_stripped = next_line.strip()
                        
                        # End of table
                        if next_stripped == '|===':
                            break
                        
                        # Next test case
                        if next_stripped.startswith('|') and '**T-' in next_stripped:
                            break
                        
                        # Check if this line has expected result (contains | and AC:)
                        if '|' in next_stripped and ('AC:' in next_stripped or '**AC:' in next_stripped):
                            # Split on the | that precedes AC:
                            # Find the position of AC: to split correctly
                            ac_pos = next_stripped.find('AC:')
                            if ac_pos > 0:
                                # Find the | before AC:
                                pipe_pos = next_stripped.rfind('|', 0, ac_pos)
                                if pipe_pos >= 0:
                                    last_step = next_stripped[:pipe_pos].strip()
                                    expected = next_stripped[pipe_pos+1:].strip()
                                    if last_step:
                                        steps_lines.append(last_step)
                                    break
                            
                            # Fallback: split on last |
                            parts = next_stripped.rsplit('|', 1)
                            if len(parts) == 2:
                                last_step = parts[0].strip()
                                expected = parts[1].strip()
                                if last_step:
                                    steps_lines.append(last_step)
                                break
                        
                        # Regular step line
                        steps_lines.append(next_line.rstrip())
                        i += 1
                    
                    steps = '\n'.join(steps_lines).strip()
                    
                    # Create test case
                    test_cases.append({
                        'id': test_id,
                        'requirement': requirement,
                        'steps': steps,
                        'expected': expected
                    })
                    continue
                
                i += 1
            
            # Replace the old table with individual tables
            # Remove the cols= line we added earlier
            result.pop()
            
            # Add individual tables
            for test_case in test_cases:
                result.append('')
                result.append('[cols="1,4"]')
                result.append('|===')
                result.append(f'| Test Case ID | {test_case["id"]}')
                result.append(f'| Requirement Covered | {test_case["requirement"]}')
                result.append('| Test Steps |')
                result.append('a|')  # AsciiDoc format for steps (allows lists)
                if test_case['steps']:
                    for step_line in test_case['steps'].split('\n'):
                        if step_line.strip():
                            result.append(step_line)
                result.append('| Expected Result (Acceptance Criteria) |')
                result.append('a|')  # AsciiDoc format for expected result
                if test_case['expected']:
                    result.append(test_case['expected'])
                result.append('|===')
                result.append('')
            
            continue
        
        # Regular line
        result.append(line)
        i += 1
    
    return '\n'.join(result)

def main():
    """Main function."""
    if len(sys.argv) < 2:
        print("Usage: convert-test-tables-robust.py <input.adoc> [output.adoc]")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else input_file + '.new'
    
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {input_file}: {e}")
        sys.exit(1)
    
    converted = convert_test_case_table(content)
    
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(converted)
        print(f"✓ Converted {input_file} -> {output_file}")
    except Exception as e:
        print(f"Error writing {output_file}: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
