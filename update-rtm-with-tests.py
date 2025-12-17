#!/usr/bin/env python3
"""
Update Requirements Traceability Matrix with test case IDs.
"""

import re
from collections import defaultdict

def extract_test_cases_from_test_plan():
    """Extract all test cases and their requirements from test plan."""
    req_to_tests = defaultdict(list)
    
    with open('Documentation/test-plan.adoc', 'r') as f:
        lines = f.readlines()
        
        i = 0
        while i < len(lines):
            line = lines[i]
            
            # Look for test case table - format: | **Test Case ID** | **T-XXX**
            if '**Test Case ID**' in line and '|' in line:
                # Test ID is on the same line: | **Test Case ID** | **T-XXX**
                parts = line.split('|')
                if len(parts) >= 3:
                    test_id = parts[2].strip()
                    # Remove any bold markers
                    test_id = test_id.replace('**', '').strip()
                    
                    if not test_id:
                        continue
                    
                    # Get requirement - look for "**Requirement Covered**" in next line
                    if i+1 < len(lines):
                        req_line = lines[i+1]
                        if '**Requirement Covered**' in req_line and '|' in req_line:
                            req_parts = req_line.split('|')
                            if len(req_parts) >= 3:
                                req_text = req_parts[2].strip()  # Requirement is in column 2 (after label)
                                # Extract requirement IDs from text
                                req_matches = re.findall(r'((?:FR|NFR|FR-CT|FR-WE|FR-NAV|FR-SERIES|DDD)-[\d\.]+(?:/[0-9\.]+)?)', req_text)
                                for req_match in req_matches:
                                    req_id = req_match
                                    # Handle combined requirements like FR-2.3.2/2.3.3
                                    if '/' in req_id:
                                        # Split into individual requirements
                                        base = req_id.split('/')[0]
                                        second = req_id.split('/')[1]
                                        # Reconstruct second requirement ID
                                        base_parts = base.split('-')
                                        if len(base_parts) >= 2:
                                            req_type = base_parts[0]
                                            req_id2 = f"{req_type}-{second}"
                                            if test_id not in req_to_tests[base]:
                                                req_to_tests[base].append(test_id)
                                            if test_id not in req_to_tests[req_id2]:
                                                req_to_tests[req_id2].append(test_id)
                                    else:
                                        if test_id not in req_to_tests[req_id]:
                                            req_to_tests[req_id].append(test_id)
            i += 1
    
    return req_to_tests

def update_rtm_with_test_cases():
    """Update RTM file with test case IDs."""
    req_to_tests = extract_test_cases_from_test_plan()
    
    print(f"Found {len(req_to_tests)} requirements with test cases")
    print(f"Total test case mappings: {sum(len(v) for v in req_to_tests.values())}")
    
    # Read current RTM
    with open('Documentation/requirements_traceability_matrix.adoc', 'r') as f:
        lines = f.readlines()
    
    # Update lines with test case IDs
    updated_lines = []
    for line in lines:
        # Check if this is a requirement row
        match = re.match(r'^\| \*\*([A-Z0-9\./-]+)\*\* \| (.+?) \| (.+?) \| (.+?)$', line.strip())
        if match:
            req_id = match.group(1)
            req_summary = match.group(2)
            ddd_section = match.group(3)
            existing_tests = match.group(4).strip()
            
            # Get test cases for this requirement
            test_ids = sorted(req_to_tests.get(req_id, []))
            test_ids_str = ', '.join(test_ids) if test_ids else ''
            
            # Update the line
            updated_line = f'| **{req_id}** | {req_summary} | {ddd_section} | {test_ids_str}\n'
            updated_lines.append(updated_line)
        else:
            updated_lines.append(line)
    
    # Write updated RTM
    with open('Documentation/requirements_traceability_matrix.adoc', 'w') as f:
        f.writelines(updated_lines)
    
    print(f"Updated RTM with test case mappings")

if __name__ == '__main__':
    update_rtm_with_test_cases()
