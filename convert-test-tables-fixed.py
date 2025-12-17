#!/usr/bin/env python3
"""
Fixed converter for test case tables.
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
        
        # Check if this is a test case table
        is_test_table = False
        if 'cols=' in line:
            for j in range(i+1, min(i+6, len(lines))):
                if 'Test Case ID' in lines[j] and '^.^' in lines[j]:
                    is_test_table = True
                    break
        
        if is_test_table:
            # Skip the cols= line and find |===
            i += 1
            while i < len(lines) and not lines[i].strip().startswith('|==='):
                i += 1
            
            if i >= len(lines):
                result.append(line)
                break
            
            # Skip |=== and header
            i += 1  # Skip |===
            if i < len(lines) and '^.^' in lines[i]:
                i += 1  # Skip header
            if i < len(lines) and not lines[i].strip():
                i += 1  # Skip blank
            
            # Parse test cases
            test_cases = []
            
            while i < len(lines):
                line = lines[i]
                stripped = line.strip()
                
                if stripped == '|===':
                    i += 1
                    break
                
                # New test case: | **T-XXX** | requirement | a|
                if stripped.startswith('|') and '**T-' in stripped:
                    # Parse first row: | **T-XXX** | requirement | a|
                    # Remove leading | and trailing |
                    cell_content = stripped[1:].strip()
                    if cell_content.endswith('|'):
                        cell_content = cell_content[:-1].strip()
                    
                    # Split by | - should give us: [test_id, requirement, 'a']
                    parts = [p.strip() for p in cell_content.split('|') if p.strip()]
                    
                    test_id = parts[0] if len(parts) > 0 else ''
                    # Requirement might have trailing 'a' from format specifier
                    requirement_raw = parts[1] if len(parts) > 1 else ''
                    requirement = requirement_raw.rstrip(' a').strip()
                    
                    # Collect steps
                    i += 1
                    steps = []
                    expected = ''
                    
                    while i < len(lines):
                        next_line = lines[i]
                        next_stripped = next_line.strip()
                        
                        if next_stripped == '|===':
                            break
                        if next_stripped.startswith('|') and '**T-' in next_stripped:
                            break
                        
                        # Check if this line has expected result
                        if '|' in next_stripped and ('AC:' in next_stripped or '**AC:' in next_stripped):
                            # Find the | before AC:
                            ac_idx = next_stripped.find('AC:')
                            pipe_idx = next_stripped.rfind('|', 0, ac_idx)
                            if pipe_idx >= 0:
                                last_step = next_stripped[:pipe_idx].strip()
                                expected = next_stripped[pipe_idx+1:].strip()
                                if last_step:
                                    steps.append(last_step)
                            break
                        
                        steps.append(next_line.rstrip())
                        i += 1
                    
                    test_cases.append({
                        'id': test_id,
                        'requirement': requirement,
                        'steps': '\n'.join(steps).strip(),
                        'expected': expected
                    })
                    continue
                
                i += 1
            
            # Generate individual tables
            for tc in test_cases:
                result.append('')
                result.append('[cols="1,4"]')
                result.append('|===')
                result.append(f'| Test Case ID | {tc["id"]}')
                result.append(f'| Requirement Covered | {tc["requirement"]}')
                
                # Test Steps - a| must be on same line as first content
                result.append('| Test Steps |')
                if tc['steps']:
                    steps_lines = [s for s in tc['steps'].split('\n') if s.strip()]
                    if steps_lines:
                        # First line: a| followed immediately by first step (no space after |)
                        result.append(f'a|{steps_lines[0]}')
                        # Remaining steps continue in same cell
                        for step in steps_lines[1:]:
                            result.append(step)
                    else:
                        result.append('a|')
                else:
                    result.append('a|')
                
                # Expected Result - a| must be on same line as content
                result.append('| Expected Result (Acceptance Criteria) |')
                if tc['expected']:
                    # Put a| and content on same line
                    result.append(f'a|{tc["expected"]}')
                else:
                    result.append('a|')
                
                result.append('|===')
                result.append('')
            
            continue
        
        result.append(line)
        i += 1
    
    return '\n'.join(result)

def main():
    input_file = sys.argv[1] if len(sys.argv) > 1 else 'Documentation/test-plan.adoc'
    output_file = sys.argv[2] if len(sys.argv) > 2 else input_file
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    converted = convert_test_case_table(content)
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(converted)
    
    print(f"✓ Converted {input_file}")

if __name__ == '__main__':
    main()
