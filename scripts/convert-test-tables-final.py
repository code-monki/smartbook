#!/usr/bin/env python3
# ============================================================================
# convert-test-tables-final.py - Test Case Table Format Converter
# ============================================================================
#
# Purpose:
#   Converts test case tables from wide multi-column format to individual
#   two-column tables (label/content format). Each test case becomes its
#   own separate table for better readability in documentation.
#
# Usage:
#   ./convert-test-tables-final.py <input.adoc> [output.adoc]
#
#   Arguments:
#     input.adoc:   AsciiDoc file containing test case tables
#     output.adoc:  Output file (default: overwrites input file)
#
# Requirements:
#   - Python 3.x
#
# Process:
#   1. Scans AsciiDoc file for test case tables (identified by "Test Case ID" header)
#   2. Converts each wide table to individual two-column tables
#   3. Each test case becomes: | Label | Content |
#   4. Preserves all table content and formatting
#
# Input Format (Wide Table):
#   [cols="1,3,2"]
#   |===
#   | **Test Case ID** | **Description** | **Requirement Covered**
#   | T-001 | Test description | FR-1.2.3
#   |===
#
# Output Format (Individual Tables):
#   |===
#   | **Test Case ID**
#   | T-001
#   |===
#   |===
#   | **Description**
#   | Test description
#   |===
#   |===
#   | **Requirement Covered**
#   | FR-1.2.3
#   |===
#
# Exit Codes:
#   0: Success
#   1: Error (file not found, parsing error, etc.)
#
# See Documentation/test-plan.adoc for example usage.
# ============================================================================

import re
import sys

def convert_test_case_table(content):
    """Convert test case tables from wide format to individual tables."""
    lines = content.split('\n')
    result = []
    i = 0
    
    while i < len(lines):
        line = lines[i]
        
        # Check if this is a test case table
        # Look for the cols= line that precedes test case tables
        if 'cols=' in line and 'Test Case ID' in (lines[i+2] if i+2 < len(lines) else ''):
            # Find the table start marker |===
            while i < len(lines) and not lines[i].strip().startswith('|==='):
                i += 1
            
            if i >= len(lines):
                result.append(line)
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
                    break
                
                # New test case row starts with | **T-
                if stripped.startswith('|') and '**T-' in stripped:
                    # Parse: | **T-XXX** | requirement | a|
                    # Remove leading | and split
                    cell_content = stripped[1:].strip()
                    parts = [p.strip() for p in cell_content.split('|') if p.strip()]
                    
                    test_id = parts[0] if len(parts) > 0 else ''
                    requirement = parts[1] if len(parts) > 1 else ''
                    # Remove trailing 'a' format specifier if present
                    requirement = requirement.rstrip(' a').strip()
                    
                    # Steps start on next lines (after 'a|') - collect until we hit a line with | that has expected result
                    i += 1
                    steps_lines = []
                    expected = ''
                    
                    while i < len(lines):
                        next_line = lines[i]
                        next_stripped = next_line.strip()
                        
                        # Check if this line contains both steps and expected result (has | in middle)
                        # Format: ". last step | Expected result"
                        # But be careful - | might be in citations, so we need to find the LAST |
                        if '|' in next_stripped and not next_stripped.startswith('|'):
                            # Find the last | that's not part of a citation or code
                            # Simple heuristic: split by | and take the last part as expected result
                            # But we need to be smarter - look for pattern: " | **AC:" or " | AC:"
                            if ' | **AC:' in next_stripped or ' | AC:' in next_stripped or ' | [cite_start]**AC:' in next_stripped:
                                # This is the line with expected result
                                # Split on the pattern that indicates start of expected result
                                if ' | **AC:' in next_stripped:
                                    parts = next_stripped.split(' | **AC:', 1)
                                    last_step = parts[0].strip()
                                    expected = '**AC:' + parts[1].strip() if len(parts) > 1 else ''
                                elif ' | [cite_start]**AC:' in next_stripped:
                                    parts = next_stripped.split(' | [cite_start]**AC:', 1)
                                    last_step = parts[0].strip()
                                    expected = '[cite_start]**AC:' + parts[1].strip() if len(parts) > 1 else ''
                                elif ' | AC:' in next_stripped:
                                    parts = next_stripped.split(' | AC:', 1)
                                    last_step = parts[0].strip()
                                    expected = 'AC:' + parts[1].strip() if len(parts) > 1 else ''
                                else:
                                    # Fallback: split on last |
                                    parts = next_stripped.rsplit('|', 1)
                                    last_step = parts[0].strip()
                                    expected = parts[1].strip() if len(parts) > 1 else ''
                                
                                if last_step:
                                    steps_lines.append(last_step)
                                if expected:
                                    break
                        
                        # If line starts with | and has **T-, it's the next test case
                        if next_stripped.startswith('|') and '**T-' in next_stripped:
                            # No expected result found - might be missing
                            break
                        
                        # If line starts with | and is just |===, end of table
                        if next_stripped == '|===':
                            break
                        
                        # Otherwise, it's part of steps
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
                
                i += 1
            
            # Convert to individual tables
            for test_case in test_cases:
                result.append('')
                result.append('[cols="1,4"]')
                result.append('|===')
                result.append(f'| Test Case ID | {test_case["id"]}')
                result.append(f'| Requirement Covered | {test_case["requirement"]}')
                result.append('| Test Steps |')
                if test_case['steps']:
                    # Preserve formatting
                    for step_line in test_case['steps'].split('\n'):
                        result.append(step_line)
                result.append(f'| Expected Result (Acceptance Criteria) | {test_case["expected"]}')
                result.append('|===')
                result.append('')
            
            i += 1  # Skip closing |===
            continue
        
        # Regular line
        result.append(line)
        i += 1
    
    return '\n'.join(result)

def main():
    """Main function."""
    if len(sys.argv) < 2:
        print("Usage: convert-test-tables-final.py <input.adoc> [output.adoc]")
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
        print(f"✓ Converted {input_file}")
        print(f"✓ Output: {output_file}")
        print(f"\nReview the output. If correct, replace original:")
        print(f"  mv {output_file} {input_file}")
    except Exception as e:
        print(f"Error writing {output_file}: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
