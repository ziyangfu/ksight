#!/usr/bin/env python3
import os
import json
import re
import argparse

def generate_commands_data(scan_dir, output_file):
    commands = {}
    
    # Walk through the scan directory to find all bash-complete.sh files
    # Expected structure: scan_dir/module/tool/scripts/bash-complete.sh
    #                     scan_dir/module/tool/config/brief.json
    for root, dirs, files in os.walk(scan_dir):
        for file in files:
            if file == "bash-complete.sh":
                file_path = os.path.join(root, file)
                try:
                    with open(file_path, 'r') as f:
                        content = f.read()
                        # Match complete -W 'options' tool_name
                        # Support multi-line options
                        match = re.search(r"complete\s+-W\s+['\"](.*?)['\"]\s+(\S+)", content, re.DOTALL)
                        if match:
                            options_str = match.group(1)
                            tool_name = match.group(2)
                            
                            # Split options and filter out empty strings
                            options = [opt.strip() for opt in options_str.split() if opt.strip()]
                            
                            # root is something like scan_dir/net/netwatcher/scripts
                            # Get the tool directory (parent of scripts)
                            tool_dir = os.path.dirname(root)
                            
                            # Binary path relative to scan_dir
                            bin_path_abs = os.path.join(tool_dir, "bin", tool_name)
                            bin_path_rel = os.path.relpath(bin_path_abs, scan_dir)
                            
                            # Try to read description from brief.json
                            description = "No description available"
                            brief_json_path = os.path.join(tool_dir, "config", "brief.json")
                            if os.path.exists(brief_json_path):
                                try:
                                    with open(brief_json_path, 'r') as brief_f:
                                        brief_data = json.load(brief_f)
                                        description = brief_data.get("description", description)
                                except Exception as e:
                                    print(f"Warning: Could not read {brief_json_path}: {e}")
                            
                            commands[tool_name] = {
                                "bin_path": bin_path_rel,
                                "description": description,
                                "options": options
                            }
                except Exception as e:
                    print(f"Error processing {file_path}: {e}")

    # Write to output file
    with open(output_file, 'w') as f:
        f.write("# Generated file. Do not edit manually.\n")
        f.write("false = False\n")
        f.write("true = True\n")
        f.write("null = None\n\n")
        f.write("COMMANDS = ")
        f.write(json.dumps(commands, indent=4))
        f.write("\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate commands_data.py from bash-complete.sh files")
    parser.add_argument("--scan-dir", required=True, help="Directory to scan for bash-complete.sh files")
    parser.add_argument("--output", required=True, help="Output Python file path")
    
    args = parser.parse_args()
    generate_commands_data(args.scan_dir, args.output)
    print(f"Successfully generated {args.output}")
