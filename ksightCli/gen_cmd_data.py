#!/usr/bin/env python3
import os
import json
import argparse
import sys

def generate_commands_data(scan_dir, output_file):
    commands = {}
    
    # Walk through the scan directory to find all *_args.json files
    # Expected structure: scan_dir/module/tool/config/tool_args.json
    for root, dirs, files in os.walk(scan_dir):
        for file in files:
            if file.endswith("_args.json"):
                file_path = os.path.join(root, file)
                try:
                    with open(file_path, 'r') as f:
                        data = json.load(f)
                        tool_name = data.get("tool_name")
                        if tool_name:
                            # root is something like scan_dir/net/netwatcher/config
                            # we want the path to the binary: scan_dir/net/netwatcher/bin/netwatcher
                            
                            # Get the tool directory (parent of config)
                            tool_dir = os.path.dirname(root)
                            
                            # Binary path relative to scan_dir
                            # For own tools: module/tool/bin/tool
                            # For third party (like nettrace): module/tool/bin/tool
                            bin_path_abs = os.path.join(tool_dir, "bin", tool_name)
                            bin_path_rel = os.path.relpath(bin_path_abs, scan_dir)
                            
                            commands[tool_name] = {
                                "bin_path": bin_path_rel,
                                "options": data.get("options", [])
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
    parser = argparse.ArgumentParser(description="Generate commands_data.py from JSON metadata")
    parser.add_argument("--scan-dir", required=True, help="Directory to scan for JSON metadata files")
    parser.add_argument("--output", required=True, help="Output Python file path")
    
    args = parser.parse_args()
    generate_commands_data(args.scan_dir, args.output)
    print(f"Successfully generated {args.output}")
