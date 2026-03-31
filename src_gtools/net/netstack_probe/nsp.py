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
    def __init__(self, config_path=None, interface=None, target=None, use_iperf=False):
        self.config_path = config_path
        self.interface = interface
        self.target = target
        self.use_iperf = use_iperf
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
            rel_path = key.replace(".", "/")
            proc_path = os.path.join("/proc/sys", rel_path)
            
            if not os.path.exists(proc_path):
                self.log_fail(f"{key}: Path {proc_path} does not exist.")
                results[key] = {"status": "MISSING", "expected": expected}
                continue

            try:
                with open(proc_path, 'r') as f:
                    actual = " ".join(f.read().strip().split())
                    expected_normalized = " ".join(str(expected).split())
                
                if actual == expected_normalized:
                    self.log_success(f"{key} is '{actual}'")
                    results[key] = {"status": "PASS", "actual": actual}
                else:
                    self.log_fail(f"{key}: expected '{expected_normalized}', got '{actual}'")
                    results[key] = {"status": "FAIL", "expected": expected_normalized, "actual": actual}
            except Exception as e:
                self.log_fail(f"{key}: Error reading {proc_path}: {e}")
                results[key] = {"status": "ERROR", "msg": str(e)}
        return results

    # --- Dimension B: Quality Prober (Ping + iperf3 + arping) ---
    def probe_l2(self):
        """L2 connectivity check via arping"""
        if not self.target:
            return None
        
        print(f"\n--- [Dimension B] L2 Probing (arping - Target: {self.target}) ---")
        if os.geteuid() != 0:
            self.log_warn("arping requires root privileges. Skipping L2 probe.")
            return {"status": "SKIP", "reason": "permission_denied"}

        if not self.interface:
            self.log_warn("arping requires an interface. Use -i. Skipping L2 probe.")
            return {"status": "SKIP", "reason": "no_interface"}

        results = {"target": self.target, "interface": self.interface}
        cmd = ["arping", "-c", "2", "-I", self.interface, self.target]
        try:
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
            if proc.returncode == 0:
                # Output example: Received 1 response(s)
                self.log_success(f"arping {self.target} successful.")
                results["status"] = "PASS"
                # Looking for MAC: Unicast reply from 127.0.0.1 [00:00:00:00:00:00]
                mac_match = re.search(r"\[([0-9a-fA-F:]+)\]", proc.stdout)
                if mac_match:
                    results["mac"] = mac_match.group(1)
                    self.log_info(f"Target MAC: {results['mac']}")
            else:
                self.log_fail(f"arping {self.target} failed (Host down or firewall).")
                results["status"] = "FAIL"
        except Exception as e:
            self.log_fail(f"Error during arping: {e}")
            results["status"] = "ERROR"
        
        return results

    def probe_connectivity(self):
        if not self.target:
            return None
        
        print(f"\n--- [Dimension B] L3 Probing (ping - Target: {self.target}) ---")
        results = {"target": self.target}
        
        # 1. ICMP Ping
        cmd = ["ping", "-c", "5", "-i", "0.2", "-q", self.target]
        try:
            process = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            if process.returncode == 0:
                self.log_success(f"Ping {self.target} successful.")
                last_lines = process.stdout.strip().split('\n')
                rtt_line = last_lines[-1]
                if '=' in rtt_line:
                    parts = rtt_line.split('=')[1].strip().split(' ')[0].split('/')
                    results["rtt_min"] = parts[0]; results["rtt_avg"] = parts[1]
                    results["rtt_max"] = parts[2]; results["rtt_mdev"] = parts[3]
                    self.log_info(f"RTT Avg: {parts[1]}ms, Jitter(mdev): {parts[3]}ms")
                
                loss_line = last_lines[-2]
                loss_match = re.search(r'(\d+)% packet loss', loss_line)
                if loss_match:
                    results["packet_loss"] = int(loss_match.group(1))
                    self.log_info(f"Packet Loss: {results['packet_loss']}%")
            else:
                self.log_fail(f"Ping {self.target} failed.")
                results["status"] = "UNREACHABLE"
        except Exception as e:
            self.log_fail(f"Error during ping: {e}")

        # 2. iperf3 Active Test
        if self.use_iperf:
            self.log_info("Running iperf3 test (5s duration)...")
            iperf_cmd = ["iperf3", "-c", self.target, "-J", "-t", "5"]
            try:
                proc = subprocess.run(iperf_cmd, capture_output=True, text=True, timeout=15)
                if proc.returncode == 0:
                    data = json.loads(proc.stdout)
                    thru = data['end']['sum_received']['bits_per_second'] / 1e6
                    retrans = data['end']['sum_sent'].get('retransmits', 0)
                    results["iperf_throughput_mbps"] = round(thru, 2)
                    results["iperf_retrans"] = retrans
                    self.log_success(f"iperf3 Throughput: {results['iperf_throughput_mbps']} Mbps, Retransmits: {retrans}")
                else:
                    self.log_warn("iperf3 test failed (is server running?)")
            except Exception as e:
                self.log_warn(f"iperf3 integration error: {e}")

        return results

    # --- Dimension C: RT & System Monitor ---
    def _read_proc_net_dev(self, iface):
        try:
            with open("/proc/net/dev", "r") as f:
                for line in f:
                    if iface + ":" in line:
                        parts = line.split(":")[1].split()
                        return {"rx": int(parts[0]), "tx": int(parts[8])}
        except: pass
        return None

    def monitor_system(self):
        print("\n--- [Dimension C] Real-time & Hardware Audit ---")
        results = {}
        
        # 1. Real-time Bandwidth (1s delta)
        if self.interface:
            print(f"[*] Calculating 1s bandwidth delta for {self.interface}...")
            s1 = self._read_proc_net_dev(self.interface)
            if s1:
                time.sleep(1.0)
                s2 = self._read_proc_net_dev(self.interface)
                if s2:
                    rx_bps = (s2["rx"] - s1["rx"]) * 8
                    tx_bps = (s2["tx"] - s1["tx"]) * 8
                    results["rx_rate_kbps"] = round(rx_bps / 1024, 2)
                    results["tx_rate_kbps"] = round(tx_bps / 1024, 2)
                    self.log_info(f"Current Traffic: RX {results['rx_rate_kbps']} Kbps, TX {results['tx_rate_kbps']} Kbps")

        # 2. Interface Requirement Audit (Hardware)
        if self.interface and "interface" in self.config:
            iface_rules = self.config["interface"].get(self.interface, {})
            if iface_rules:
                try:
                    out = subprocess.check_output(["ethtool", self.interface], text=True, stderr=subprocess.STDOUT)
                    speed_match = re.search(r"Speed: (\d+)Mb/s", out)
                    duplex_match = re.search(r"Duplex: (\w+)", out)
                    
                    actual_speed = int(speed_match.group(1)) if speed_match else 0
                    actual_duplex = duplex_match.group(1).lower() if duplex_match else "unknown"
                    
                    min_speed = iface_rules.get("min_speed", 0)
                    if actual_speed >= min_speed:
                        self.log_success(f"{self.interface} Speed: {actual_speed}Mb/s (>= {min_speed})")
                    else:
                        self.log_fail(f"{self.interface} Speed: {actual_speed}Mb/s (Expected >= {min_speed})")
                    
                    req_duplex = iface_rules.get("duplex", "").lower()
                    if req_duplex and actual_duplex == req_duplex:
                        self.log_success(f"{self.interface} Duplex: {actual_duplex}")
                    elif req_duplex:
                        self.log_fail(f"{self.interface} Duplex: {actual_duplex} (Expected: {req_duplex})")
                    results["hardware"] = {"speed": actual_speed, "duplex": actual_duplex}
                except Exception:
                    self.log_warn(f"Could not run ethtool for {self.interface}. Skipping hardware speed audit.")

        # 3. SoftIRQ Balance
        try:
            with open("/proc/softirqs", "r") as f:
                for line in f:
                    if "NET_RX" in line or "NET_TX" in line:
                        parts = line.split()
                        irq_type = parts[0].replace(":", "")
                        counts = [int(p) for p in parts[1:]]
                        avg = sum(counts) / len(counts) if counts else 0
                        results[irq_type] = counts
                        if any(c > avg * 1.5 for c in counts) and avg > 1000:
                            self.log_warn(f"{irq_type} is imbalanced across CPUs.")
                        else:
                            self.log_success(f"{irq_type} distribution is balanced.")
        except Exception: pass

        return results

    def run_all(self):
        report = {"timestamp": datetime.now().isoformat(), "dimensions": {}}
        report["dimensions"]["A"] = self.audit_sysctl()
        if self.target:
            report["dimensions"]["B"] = {
                "l3": self.probe_connectivity(),
                "l2": self.probe_l2()
            }
        report["dimensions"]["C"] = self.monitor_system()
        return report

def main():
    parser = argparse.ArgumentParser(description="netstack_probe (nsp): Professional Network Stack Diagnostic Tool (Level 1)")
    parser.add_argument("-C", "--check-config", action="store_true", help="Audit sysctl config (Dimension A)")
    parser.add_argument("-P", "--probe", action="store_true", help="Probe connectivity/quality (Dimension B)")
    parser.add_argument("-r", "--arping", action="store_true", help="Include L2 arping probe (Dimension B)")
    parser.add_argument("-S", "--stat", action="store_true", help="Monitor real-time stats & hardware (Dimension C)")
    parser.add_argument("-A", "--all", action="store_true", help="Run all dimensions (A, B+L2, C)")
    parser.add_argument("-t", "--target", type=str, help="Target IP for probing")
    parser.add_argument("-i", "--interface", type=str, help="Interface name (required for arping and some stat modules)")
    parser.add_argument("-I", "--iperf", action="store_true", help="Include active iperf3 test in Dimension B")
    parser.add_argument("-c", "--config", type=str, default="desired_state.json", help="Path to JSON config")
    parser.add_argument("-j", "--json", action="store_true", help="Output results in JSON format")

    args = parser.parse_args()
    config_path = args.config if os.path.isabs(args.config) else os.path.join(os.path.dirname(os.path.realpath(__file__)), args.config)
    nsp = NetstackProbe(config_path=config_path, interface=args.interface, target=args.target, use_iperf=args.iperf)

    if args.json:
        import contextlib
        with contextlib.redirect_stdout(None): report = nsp.run_all()
        print(json.dumps(report, indent=2))
        return

    if args.all:
        nsp.run_all()
    else:
        if args.check_config: nsp.audit_sysctl()
        if args.probe or args.arping:
            if args.target:
                if args.probe: nsp.probe_connectivity()
                if args.arping: nsp.probe_l2()
            else: print("Error: --target required for probing.")
        if args.stat: nsp.monitor_system()
        if not any([args.check_config, args.probe, args.stat, args.arping]):
            parser.print_help()

if __name__ == "__main__":
    main()
