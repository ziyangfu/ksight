#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import json
import argparse
import subprocess
import time
import re
from datetime import datetime

class NetstackProbe:
    def __init__(self, config_path=None, interface=None, target=None):
        self.config_path = config_path
        self.interface = interface
        self.target = target
        self.config = {}
        if config_path and os.path.exists(config_path):
            with open(config_path, 'r') as f:
                self.config = json.load(f)

    def log_info(self, msg):
        print(f"[*] {msg}")

    def log_success(self, msg):
        print(f"[\033[32mOK\033[0m] {msg}")

    def log_fail(self, msg):
        print(f"[\033[31mFAIL\033[0m] {msg}")

    def log_warn(self, msg):
        print(f"[\033[33mWARN\033[0m] {msg}")

    # --- Dimension A: Sysctl Auditor ---
    def audit_sysctl(self):
        print("\n--- [Dimension A] Sysctl Configuration Audit ---")
        sysctl_rules = self.config.get("sysctl", {})
        if not sysctl_rules:
            self.log_warn("No sysctl rules defined in config.")
            return {}

        results = {}
        for key, expected in sysctl_rules.items():
            # Convert net.ipv4... to /proc/sys/net/ipv4/...
            rel_path = key.replace(".", "/")
            proc_path = os.path.join("/proc/sys", rel_path)
            
            if not os.path.exists(proc_path):
                self.log_fail(f"{key}: Path {proc_path} does not exist.")
                results[key] = {"status": "MISSING", "expected": expected}
                continue

            try:
                with open(proc_path, 'r') as f:
                    actual = f.read().strip()
                if str(actual) == str(expected):
                    self.log_success(f"{key} is {actual}")
                    results[key] = {"status": "PASS", "actual": actual}
                else:
                    self.log_fail(f"{key}: expected {expected}, got {actual}")
                    results[key] = {"status": "FAIL", "expected": expected, "actual": actual}
            except Exception as e:
                self.log_fail(f"{key}: Error reading {proc_path}: {e}")
                results[key] = {"status": "ERROR", "msg": str(e)}
        return results

    # --- Dimension B: Quality Prober ---
    def probe_connectivity(self):
        if not self.target:
            return None
        
        print(f"\n--- [Dimension B] Quality Probing (Target: {self.target}) ---")
        
        results = {"target": self.target}
        
        # 1. ICMP Ping
        cmd = ["ping", "-c", "5", "-i", "0.2", "-q", self.target]
        try:
            start_time = time.time()
            process = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            duration = time.time() - start_time
            
            if process.returncode == 0:
                self.log_success(f"Ping {self.target} successful.")
                # Parse summary: rtt min/avg/max/mdev = 0.051/0.101/0.151/0.050 ms
                last_line = process.stdout.strip().split('\n')[-1]
                if '/' in last_line:
                    parts = last_line.split('=')[1].strip().split(' ')[0].split('/')
                    results["rtt_min"] = parts[0]
                    results["rtt_avg"] = parts[1]
                    results["rtt_max"] = parts[2]
                    results["rtt_mdev"] = parts[3]
                    self.log_info(f"RTT Avg: {parts[1]}ms, Jitter(mdev): {parts[3]}ms")
                
                # Parse loss
                loss_line = process.stdout.strip().split('\n')[-2]
                loss_match = re.search(r'(\d+)% packet loss', loss_line)
                if loss_match:
                    results["packet_loss"] = loss_match.group(1)
                    self.log_info(f"Packet Loss: {results['packet_loss']}%")
            else:
                self.log_fail(f"Ping {self.target} failed (Return code: {process.returncode})")
                results["status"] = "UNREACHABLE"
        except subprocess.TimeoutExpired:
            self.log_fail(f"Ping {self.target} timed out.")
            results["status"] = "TIMEOUT"
        except Exception as e:
            self.log_fail(f"Error during ping: {e}")
            results["status"] = "ERROR"

        return results

    # --- Dimension C: RT & System Monitor ---
    def monitor_system(self):
        print("\n--- [Dimension C] Real-time & Hardware Audit ---")
        results = {}
        
        # 1. Interface Stats from /proc/net/dev
        if self.interface:
            try:
                with open("/proc/net/dev", "r") as f:
                    lines = f.readlines()
                for line in lines:
                    if self.interface in line:
                        parts = line.split(":")[1].split()
                        # bytes, packets, errs, drop ...
                        results["rx_bytes"] = parts[0]
                        results["rx_errs"] = parts[2]
                        results["rx_drop"] = parts[3]
                        results["tx_bytes"] = parts[8]
                        results["tx_errs"] = parts[10]
                        results["tx_drop"] = parts[11]
                        
                        if int(results["rx_errs"]) > 0 or int(results["rx_drop"]) > 0:
                            self.log_warn(f"{self.interface} RX errors: {results['rx_errs']}, drops: {results['rx_drop']}")
                        else:
                            self.log_success(f"{self.interface} is clean (No RX errors/drops)")
            except Exception as e:
                self.log_fail(f"Error reading /proc/net/dev: {e}")

        # 2. SoftIRQ Balance
        try:
            with open("/proc/softirqs", "r") as f:
                lines = f.readlines()
            for line in lines:
                if "NET_RX" in line or "NET_TX" in line:
                    parts = line.split()
                    irq_type = parts[0].replace(":", "")
                    counts = [int(p) for p in parts[1:]]
                    avg = sum(counts) / len(counts) if counts else 0
                    results[irq_type] = counts
                    
                    # Check imbalance
                    imbalance = False
                    if avg > 1000: # Only care if there's actual load
                        for c in counts:
                            if c > avg * 2: imbalance = True
                    
                    if imbalance:
                        self.log_warn(f"{irq_type} is imbalanced across CPUs.")
                    else:
                        self.log_success(f"{irq_type} distribution is acceptable.")
        except Exception as e:
            self.log_fail(f"Error reading /proc/softirqs: {e}")

        return results

    def run_all(self):
        report = {
            "timestamp": datetime.now().isoformat(),
            "dimensions": {}
        }
        report["dimensions"]["A"] = self.audit_sysctl()
        if self.target:
            report["dimensions"]["B"] = self.probe_connectivity()
        report["dimensions"]["C"] = self.monitor_system()
        
        return report

def main():
    parser = argparse.ArgumentParser(description="netstack_probe (nsp): Network Diagnostic Tool (Level 1)")
    parser.add_argument("-C", "--check-config", action="store_true", help="Audit sysctl configuration based on JSON")
    parser.add_argument("-P", "--probe", action="store_true", help="Probe connectivity, latency, and loss to target")
    parser.add_argument("-S", "--stat", action="store_true", help="Monitor real-time stats and hardware (IRQs, RX/TX)")
    parser.add_argument("-A", "--all", action="store_true", help="Run all dimensions of probe")
    parser.add_argument("-t", "--target", type=str, help="Target IP address for probing")
    parser.add_argument("-i", "--interface", type=str, help="Interface name to monitor (e.g. eth0)")
    parser.add_argument("-c", "--config", type=str, default="desired_state.json", help="Path to desired_state JSON")
    parser.add_argument("-j", "--json", action="store_true", help="Output in structure JSON format")

    args = parser.parse_args()

    # Find the config file in the same directory as the script if not absolutely provided
    config_path = args.config
    if not os.path.isabs(config_path):
        script_dir = os.path.dirname(os.path.realpath(__file__))
        config_path = os.path.join(script_dir, args.config)

    nsp = NetstackProbe(config_path=config_path, interface=args.interface, target=args.target)

    if args.json:
        # Redirect stdout to null if we want pure JSON? 
        # Better: handle reporting differently.
        # For now, just collect and print at end.
        import contextlib
        with contextlib.redirect_stdout(None):
            report = nsp.run_all()
        print(json.dumps(report, indent=2))
        return

    if args.all:
        nsp.run_all()
    else:
        if args.check_config:
            nsp.audit_sysctl()
        if args.probe:
            if not args.target:
                print("Error: --target is required for --probe")
            else:
                nsp.probe_connectivity()
        if args.stat:
            nsp.monitor_system()
        
        if not (args.check_config or args.probe or args.stat):
            parser.print_help()

if __name__ == "__main__":
    main()
