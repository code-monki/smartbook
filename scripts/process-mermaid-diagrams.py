#!/usr/bin/env python3
"""
Process Mermaid diagrams in AsciiDoc files.

Extracts Mermaid code blocks, converts them to SVG using mmdc,
and replaces the blocks with image references.
"""

import re
import subprocess
import os
import sys
import shutil
from pathlib import Path

def find_mmdc():
    """Find mmdc executable."""
    mmdc_path = shutil.which('mmdc')
    if not mmdc_path:
        print("Error: mmdc (mermaid-cli) not found. Install with: npm install -g @mermaid-js/mermaid-cli")
        sys.exit(1)
    return mmdc_path

def process_mermaid_blocks(content, output_dir, mmdc_path):
    """Process Mermaid blocks in AsciiDoc content."""
    # Manually parse Mermaid blocks
    lines = content.split('\n')
    processed_lines = []
    i = 0
    converted_count = 0
    
    # Debug: check if we have mermaid blocks
    mermaid_count = sum(1 for line in lines if '[mermaid,' in line and 'svg]' in line)
    if mermaid_count > 0:
        print(f"  Found {mermaid_count} Mermaid diagram block(s) to process")
    
    while i < len(lines):
        line = lines[i]
        # Check if this is a mermaid block start
        if '[mermaid,' in line and 'svg]' in line:
            # Extract name - allow for space before svg
            name_match = re.search(r'\[mermaid,([^,\]]+),\s*svg\]', line)
            if not name_match:
                print(f"  Warning: Could not extract name from line: {line[:50]}")
                processed_lines.append(line)
                i += 1
                continue
            
            name = name_match.group(1).strip()
            print(f"  Found Mermaid block: {name}")
            processed_lines.append(line)  # Keep the original line for now
            i += 1
            
            # Skip the ---- delimiter
            if i < len(lines) and lines[i].strip() == '----':
                i += 1
            
            # Collect mermaid code until we hit the closing ----
            mermaid_lines = []
            while i < len(lines):
                if lines[i].strip() == '----':
                    break
                mermaid_lines.append(lines[i])
                i += 1
            
            # Skip the closing ----
            if i < len(lines):
                i += 1
            
            # Convert mermaid to SVG
            mermaid_code = '\n'.join(mermaid_lines)
            print(f"  Processing diagram: {name} ({len(mermaid_lines)} lines)")
            if convert_mermaid_to_svg(name, mermaid_code, output_dir, mmdc_path):
                # Replace the mermaid block with image reference
                # Remove the original mermaid line we added
                processed_lines.pop()
                image_path = f'images/{name}.svg'
                # Add role="diagram" and use [unbreakable] attribute to prevent page breaks
                processed_lines.append(f'[unbreakable]\nimage::{image_path}[{name} diagram,role="diagram"]')
                converted_count += 1
            else:
                # Keep original if conversion failed - remove the mermaid line we added
                processed_lines.pop()
                processed_lines.append(line)  # Put back original line
                # Also need to put back the ---- and content
                processed_lines.append('----')
                processed_lines.extend(mermaid_lines)
                processed_lines.append('----')
        else:
            processed_lines.append(line)
            i += 1
    
    processed_content = '\n'.join(processed_lines)
    return processed_content, converted_count

def convert_mermaid_to_svg(name, mermaid_code, output_dir, mmdc_path):
    """Convert a single Mermaid diagram to SVG."""
    # Create temporary .mmd file
    temp_mmd = f'/tmp/{name}.mmd'
    try:
        with open(temp_mmd, 'w') as f:
            f.write(mermaid_code)
        
        # Convert to SVG
        svg_file = os.path.join(output_dir, f'{name}.svg')
        os.makedirs(output_dir, exist_ok=True)
        
        try:
            result = subprocess.run(
                [mmdc_path, '-i', temp_mmd, '-o', svg_file, '-b', 'transparent'],
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode == 0 and os.path.exists(svg_file):
                print(f"  ✓ Converted {name}.mmd -> {name}.svg")
                return True
            else:
                print(f"  ✗ Failed to convert {name}.mmd: {result.stderr}")
                return False
        except subprocess.TimeoutExpired:
            print(f"  ✗ Timeout converting {name}.mmd")
            return False
        except Exception as e:
            print(f"  ✗ Error converting {name}: {e}")
            return False
    finally:
        # Clean up temporary file
        if os.path.exists(temp_mmd):
            os.remove(temp_mmd)

def main():
    if len(sys.argv) < 3:
        print("Usage: process-mermaid-diagrams.py <input.adoc> <output_dir>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_dir = sys.argv[2]
    
    if not os.path.exists(input_file):
        print(f"Error: Input file not found: {input_file}")
        sys.exit(1)
    
    mmdc_path = find_mmdc()
    
    # Read input file
    with open(input_file, 'r') as f:
        content = f.read()
    
    # Process Mermaid blocks
    processed_content, count = process_mermaid_blocks(content, output_dir, mmdc_path)
    
    if count > 0:
        # Write processed content to a temporary file
        # We'll use this for HTML generation
        temp_file = input_file + '.processed'
        with open(temp_file, 'w') as f:
            f.write(processed_content)
        print(f"\nProcessed {count} Mermaid diagram(s)")
        print(f"Processed file: {temp_file}")
        sys.exit(0)
    else:
        print("No Mermaid diagrams found or converted")
        sys.exit(0)

if __name__ == '__main__':
    main()

