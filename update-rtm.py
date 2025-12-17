#!/usr/bin/env python3
"""
Update Requirements Traceability Matrix with all requirements and test cases.
"""

import re
from collections import defaultdict

def extract_requirements_from_srs():
    """Extract all requirements from SRS."""
    requirements = {}
    
    with open('Documentation/srs.adoc', 'r') as f:
        content = f.read()
        
        # Pattern: * **FR-X.Y.Z (Title):** Description
        pattern = re.compile(r'\*\s+\*\*(FR|NFR|FR-CT|FR-WE|FR-NAV|FR-SERIES|FR-PLAT|FR-ERR|FR-SQL|FR-HELP)-(\d+\.\d+(?:\.\d+)?[a-z]?)\s*\((.*?)\):', re.MULTILINE)
        for match in pattern.finditer(content):
            req_type = match.group(1)
            req_num = match.group(2)
            title = match.group(3)
            req_id = f"{req_type}-{req_num}"
            requirements[req_id] = title
    
    return requirements

def extract_test_cases_from_test_plan():
    """Extract all test cases and their requirements from test plan."""
    test_cases = {}
    req_to_tests = defaultdict(list)
    
    with open('Documentation/test-plan.adoc', 'r') as f:
        content = f.read()
        lines = content.split('\n')
        
        i = 0
        while i < len(lines):
            line = lines[i]
            
            # Look for test case table - format: | **Test Case ID** | **T-XXX**
            if '**Test Case ID**' in line and i+1 < len(lines):
                # Get test case ID from next line
                test_id_line = lines[i+1]
                if '|' in test_id_line:
                    parts = test_id_line.split('|')
                    if len(parts) >= 2:
                        test_id = parts[1].strip()
                        
                        # Get requirement - look for "**Requirement Covered**" in next few lines
                        for j in range(i+2, min(i+10, len(lines))):
                            if '**Requirement Covered**' in lines[j] and j+1 < len(lines):
                                req_line = lines[j+1]
                                if '|' in req_line:
                                    req_parts = req_line.split('|')
                                    if len(req_parts) >= 2:
                                        req_text = req_parts[1].strip()
                                        # Extract requirement IDs from text (handle multiple)
                                        # Pattern: FR-2.3.1 or FR-2.3.2/2.3.3
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
                                                    req_to_tests[base].append(test_id)
                                                    req_to_tests[req_id2].append(test_id)
                                            else:
                                                req_to_tests[req_id].append(test_id)
                                break
            i += 1
    
    return test_cases, req_to_tests

def get_ddd_section_for_requirement(req_id):
    """Map requirement to DDD section."""
    ddd_mapping = {
        'FR-2.1.1': 'Sec 8.1 (HLD), Sec 10.2 (Classes)',
        'FR-2.1.2': 'Sec 8.1 (HLD)',
        'FR-2.1.3': 'Sec 19.3 (Window State)',
        'FR-2.1.4': 'Sec 19.3 (Window State)',
        'FR-2.1.5': 'Sec 19.3 (Window State)',
        'FR-2.2.1': 'Sec 5 (Qt WebEngine)',
        'FR-2.2.2': 'Sec 5 (Qt WebEngine), Sec 8 (Embedded Apps)',
        'FR-2.2.3': 'Sec 3.5 (Settings Table)',
        'FR-2.2.4': 'Sec 3.5 (Settings Table)',
        'FR-2.3.1': 'Sec 6.2 (Verification Phase 2)',
        'FR-2.3.2': 'Sec 6.4 (Verification Phase 4), Sec 11.2 (UI)',
        'FR-2.3.3': 'Sec 6.4 (Verification Phase 4), Sec 11.2 (UI)',
        'FR-2.4.1': 'Sec 4.2 (Schema), Sec 6.3 (Verification Phase 3)',
        'FR-2.4.3': 'Sec 4.2 (Schema), Sec 12.1 (Deletion Process)',
        'FR-2.4.4': 'Sec 6.3 (Verification Phase 3)',
        'FR-2.5.1': 'Sec 4.1 (Schema), Sec 7.1 (Import Algorithm)',
        'FR-2.5.5': 'Sec 4.1 (Schema), Sec 12.1 (Deletion Process)',
        'FR-2.7.1': 'Sec 6.2 (Form Rendering)',
        'FR-2.7.2': 'Sec 6.1 (Form Embedding)',
        'FR-2.7.3': 'Sec 6.3 (Form Validation)',
        'FR-2.7.4': 'Sec 6.4 (Form Data Persistence)',
        'FR-2.7.5': 'Sec 6.4 (Form Data Persistence)',
        'FR-2.7.6': 'Sec 6.4 (Form Data Loading)',
        'FR-2.7.7': 'Sec 6.5 (Form Printing)',
        'FR-2.7.8': 'Sec 6.4 (Form Auto-Save)',
        'FR-2.7.9': 'Sec 6.4 (Form Reset)',
        'FR-2.7.10': 'Sec 6.1 (Form Embedding)',
        'FR-2.7.11': 'Sec 6.4 (Form Field Undo/Redo)',
        'FR-2.8.1': 'Sec 7.3 (Forward/Backward Navigation)',
        'FR-2.8.2': 'Sec 7.4 (TOC Generation)',
        'FR-2.8.3': 'Sec 7.4 (TOC Display)',
        'FR-2.8.4': 'Sec 7.5 (Link Processing)',
        'FR-2.8.5': 'Sec 7.6 (Bookmarking)',
        'FR-2.8.6': 'Sec 7.7 (Reading Position)',
        'FR-2.8.7': 'Sec 7.8 (Navigation History)',
        'FR-2.9.1': 'Sec 7.5 (Document Search)',
        'FR-2.9.2': 'Sec 7.5 (Normal Search)',
        'FR-2.9.3': 'Sec 7.5 (Boolean Search)',
        'FR-2.9.4': 'Sec 7.5 (Fuzzy Search)',
        'FR-2.9.5': 'Sec 7.5 (Regex Search)',
        'FR-2.9.6': 'Sec 7.5 (Search Results)',
        'FR-2.9.7': 'Sec 7.5 (Search Scope)',
        'FR-2.9.8': 'Sec 7.5 (Search Performance)',
        'FR-2.10.1': 'Sec 7.6 (Find-in-Page)',
        'FR-2.10.2': 'Sec 7.6 (Term Highlighting)',
        'FR-2.10.3': 'Sec 7.6 (Match Navigation)',
        'FR-2.10.4': 'Sec 7.6 (Result Count)',
        'FR-2.10.5': 'Sec 7.6 (Case Sensitivity)',
        'FR-2.10.6': 'Sec 7.6 (Auto-Scroll)',
        'FR-2.10.7': 'Sec 7.6 (Search Persistence)',
        'FR-2.11.1': 'Sec 9 (Print Functionality)',
        'FR-2.11.2': 'Sec 9.1 (System Print Dialog)',
        'FR-2.11.3': 'Sec 9.2 (Print Media CSS)',
        'FR-2.11.4': 'Sec 9.2 (Print CSS Application)',
        'FR-2.11.5': 'Sec 9.3 (Offscreen Rendering)',
        'FR-2.11.6': 'Sec 9.4 (Print Preview)',
        'FR-2.11.7': 'Sec 9.5 (Print Quality)',
        'FR-2.11.8': 'Sec 9.1 (Print Keyboard Shortcut)',
        'FR-2.12.1': 'Sec 10 (Text Selection)',
        'FR-2.12.2': 'Sec 10.1 (Copy to Clipboard)',
        'FR-2.12.3': 'Sec 10.1 (Background Removal)',
        'FR-2.12.4': 'Sec 10.2 (Paste into Forms)',
        'FR-2.12.5': 'Sec 10.1 (Paste into External)',
        'FR-2.12.6': 'Sec 10.1 (Copy Context Menu)',
        'FR-2.12.7': 'Sec 10.3 (Keyboard Shortcuts)',
        'NFR-3.1': 'Sec 7.1 (Import Algorithm), Sec 10.1 (Classes)',
        'NFR-3.2': 'Sec 4.1 (Schema)',
        'NFR-3.3': 'Sec 6.3 (Verification Phase 3)',
    }
    
    # Check exact match first
    if req_id in ddd_mapping:
        return ddd_mapping[req_id]
    
    # Check prefix matches for ranges
    base_id = req_id.split('/')[0]  # Handle FR-2.3.2/2.3.3
    if base_id in ddd_mapping:
        return ddd_mapping[base_id]
    
    # Infer from category
    if req_id.startswith('FR-2.1'):
        return 'Sec 8.1 (HLD), Sec 10.2 (Classes)'
    elif req_id.startswith('FR-2.2'):
        return 'Sec 5 (Qt WebEngine)'
    elif req_id.startswith('FR-2.3'):
        return 'Sec 6.2-6.4 (Security Verification)'
    elif req_id.startswith('FR-2.4'):
        return 'Sec 4.2 (Schema), Sec 6.3 (Verification)'
    elif req_id.startswith('FR-2.5'):
        return 'Sec 4.1 (Schema), Sec 7.1 (Import)'
    elif req_id.startswith('FR-2.7'):
        return 'Sec 6 (Form Rendering)'
    elif req_id.startswith('FR-2.8'):
        return 'Sec 7 (Navigation)'
    elif req_id.startswith('FR-2.9'):
        return 'Sec 7.5 (Document Search)'
    elif req_id.startswith('FR-2.10'):
        return 'Sec 7.6 (In-Page Search)'
    elif req_id.startswith('FR-2.11'):
        return 'Sec 9 (Print Functionality)'
    elif req_id.startswith('FR-2.12'):
        return 'Sec 10 (Text Selection and Copy)'
    elif req_id.startswith('FR-CT'):
        return 'Sec 13 (Creator Tool)'
    elif req_id.startswith('FR-WE'):
        return 'Sec 5 (Qt WebEngine Configuration)'
    elif req_id.startswith('FR-NAV'):
        return 'Sec 7.1 (Navigation Structure)'
    elif req_id.startswith('FR-SERIES'):
        return 'Sec 14 (Series and Edition Grouping)'
    elif req_id.startswith('FR-PLAT'):
        return 'Sec 15 (Platform-Specific Considerations)'
    elif req_id.startswith('FR-ERR'):
        return 'Sec 4 (Error Handling)'
    elif req_id.startswith('FR-SQL'):
        return 'Sec 16 (SQLite Configuration)'
    elif req_id.startswith('FR-HELP'):
        return 'Sec 9 (Help System)'
    
    return 'See DDD'

def generate_rtm():
    """Generate complete RTM."""
    requirements = extract_requirements_from_srs()
    test_cases, req_to_tests = extract_test_cases_from_test_plan()
    
    print(f"Found {len(requirements)} requirements")
    print(f"Found {len(test_cases)} test cases")
    print(f"Found {len(req_to_tests)} requirements with test cases")
    
    # Generate RTM content
    rtm_lines = []
    rtm_lines.append('== Requirements Traceability Matrix')
    rtm_lines.append('')
    rtm_lines.append('[cols="1, 3, 2, 1", options="header", grid="all"]')
    rtm_lines.append('|===')
    rtm_lines.append('^.^| SRS ID ^.^| Requirement Summary ^.^| Design Artifact (DDD Section) ^.^| Test Case IDs')
    rtm_lines.append('')
    
    # Sort requirements
    sorted_reqs = sorted(requirements.keys())
    
    for req_id in sorted_reqs:
        req_title = requirements[req_id]
        ddd_section = get_ddd_section_for_requirement(req_id)
        test_ids = sorted(req_to_tests.get(req_id, []))
        test_ids_str = ', '.join(test_ids) if test_ids else ''
        
        rtm_lines.append(f'| **{req_id}** | {req_title} | {ddd_section} | {test_ids_str}')
    
    rtm_lines.append('|===')
    
    return '\n'.join(rtm_lines)

if __name__ == '__main__':
    rtm_content = generate_rtm()
    print("\n" + "="*80)
    print("GENERATED RTM:")
    print("="*80)
    print(rtm_content)
