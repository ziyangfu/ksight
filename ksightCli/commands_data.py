# Generated file. Do not edit manually.
false = False
true = True
null = None

COMMANDS = {
    "ipcwatcher": {
        "bin_path": "ipc/ipcwatcher/bin/ipcwatcher",
        "options": [
            {
                "default": false,
                "flags": [
                    "-u",
                    "--uds"
                ],
                "help": "Trace unix domain socket",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-m",
                    "--mmap"
                ],
                "help": "Trace mmap",
                "required": false,
                "type": "bool"
            },
            {
                "default": 0,
                "flags": [
                    "-p",
                    "--pid"
                ],
                "help": "filter via send pid",
                "required": false,
                "type": "int"
            },
            {
                "default": "",
                "flags": [
                    "--filterPath"
                ],
                "help": "Filter path",
                "required": false,
                "type": "string"
            },
            {
                "default": false,
                "flags": [
                    "--traceNoAnonUds"
                ],
                "help": "only trace no anon uds like /tmp/sample.uds",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--payload"
                ],
                "help": "Print payload",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--force"
                ],
                "help": "Force enable payload printing",
                "required": false,
                "type": "bool"
            },
            {
                "default": "",
                "flags": [
                    "--pcapFile"
                ],
                "help": "Save output to pcap file",
                "required": false,
                "type": "string"
            },
            {
                "default": false,
                "flags": [
                    "--fromJson"
                ],
                "help": "read config args from json file",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--vvv",
                    "--verbose"
                ],
                "help": "Output more information",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--generateConfigJson"
                ],
                "help": "Generate json config file",
                "required": false,
                "type": "bool"
            }
        ]
    },
    "netwatcher": {
        "bin_path": "net/netwatcher/bin/netwatcher",
        "options": [
            {
                "default": false,
                "flags": [
                    "-a",
                    "--all"
                ],
                "help": "set to trace CLOSED connection",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-e",
                    "--err"
                ],
                "help": "set to trace TCP error packets",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-x",
                    "--extra"
                ],
                "help": "set to trace extra conn info",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-r",
                    "--retrans"
                ],
                "help": "set to trace extra retrans info",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-t",
                    "--time"
                ],
                "help": "set to trace layer time of each packet",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-i",
                    "--http"
                ],
                "help": "set to trace http info",
                "required": false,
                "type": "bool"
            },
            {
                "default": 0,
                "flags": [
                    "-s",
                    "--sport"
                ],
                "help": "trace this source port only",
                "required": false,
                "type": "int"
            },
            {
                "default": 0,
                "flags": [
                    "-d",
                    "--dport"
                ],
                "help": "trace this destination port only",
                "required": false,
                "type": "int"
            },
            {
                "default": false,
                "flags": [
                    "-u",
                    "--udp"
                ],
                "help": "trace the udp message",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-n",
                    "--net_filter"
                ],
                "help": "trace ipv4 packget filter",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-k",
                    "--drop_reason"
                ],
                "help": "trace kfree",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-F",
                    "--addr_to_func"
                ],
                "help": "translation addr to func and offset",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-I",
                    "--icmptime"
                ],
                "help": "set to trace layer time of icmp",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-S",
                    "--tcpstate"
                ],
                "help": "set to trace tcpstate",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-L",
                    "--timeload"
                ],
                "help": "analysis time load",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-D",
                    "--dns"
                ],
                "help": "set to trace dns information",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-A",
                    "--stack"
                ],
                "help": "set to trace of stack",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-M",
                    "--mysql"
                ],
                "help": "set to trace mysql information",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-R",
                    "--redis"
                ],
                "help": "set to trace redis information",
                "required": false,
                "type": "bool"
            },
            {
                "default": 0,
                "flags": [
                    "-C",
                    "--count"
                ],
                "help": "specify the time to count the number of requests",
                "required": false,
                "type": "int"
            },
            {
                "default": false,
                "flags": [
                    "-T",
                    "--rtt"
                ],
                "help": "set to trace rtt",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-U",
                    "--rst_counters"
                ],
                "help": "set to trace rst",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--generateConfigJson"
                ],
                "help": "Generate json config file",
                "required": false,
                "type": "bool"
            }
        ]
    },
    "nettrace": {
        "bin_path": "net/nettrace/bin/nettrace",
        "options": [
            {
                "default": "",
                "flags": [
                    "-s",
                    "--saddr"
                ],
                "help": "filter source ip/ipv6 address",
                "required": false,
                "type": "string"
            },
            {
                "default": "",
                "flags": [
                    "-d",
                    "--daddr"
                ],
                "help": "filter dest ip/ipv6 address",
                "required": false,
                "type": "string"
            },
            {
                "default": "",
                "flags": [
                    "--addr"
                ],
                "help": "filter source or dest ip/ipv6 address",
                "required": false,
                "type": "string"
            },
            {
                "default": 0,
                "flags": [
                    "-S",
                    "--sport"
                ],
                "help": "filter source TCP/UDP port",
                "required": false,
                "type": "int"
            },
            {
                "default": 0,
                "flags": [
                    "-D",
                    "--dport"
                ],
                "help": "filter dest TCP/UDP port",
                "required": false,
                "type": "int"
            },
            {
                "default": 0,
                "flags": [
                    "-P",
                    "--port"
                ],
                "help": "filter source or dest TCP/UDP port",
                "required": false,
                "type": "int"
            },
            {
                "default": "",
                "flags": [
                    "-p",
                    "--proto"
                ],
                "help": "filter L3/L4 protocol, such as 'tcp', 'arp'",
                "required": false,
                "type": "string"
            },
            {
                "default": 0,
                "flags": [
                    "--netns"
                ],
                "help": "filter by net namespace inode",
                "required": false,
                "type": "int"
            },
            {
                "default": false,
                "flags": [
                    "--netns-current"
                ],
                "help": "filter by current net namespace",
                "required": false,
                "type": "bool"
            },
            {
                "default": 0,
                "flags": [
                    "--pid"
                ],
                "help": "filter by current process id(pid)",
                "required": false,
                "type": "int"
            },
            {
                "default": 0,
                "flags": [
                    "--min-latency"
                ],
                "help": "filter by the minial time to live of the skb in us",
                "required": false,
                "type": "int"
            },
            {
                "default": 0,
                "flags": [
                    "--pkt-len"
                ],
                "help": "filter by the IP packet length (include header) in byte",
                "required": false,
                "type": "int"
            },
            {
                "default": "",
                "flags": [
                    "--tcp-flags"
                ],
                "help": "filter by TCP flags, such as: SAPR",
                "required": false,
                "type": "string"
            },
            {
                "default": false,
                "flags": [
                    "--basic"
                ],
                "help": "use 'basic' trace mode, don't trace skb's life",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--diag"
                ],
                "help": "enable 'diagnose' mode",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--diag-quiet"
                ],
                "help": "only print abnormal packet",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--diag-keep"
                ],
                "help": "don't quit when abnormal packet found",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--drop"
                ],
                "help": "skb drop monitor mode, for replace of 'droptrace'",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--drop-stack"
                ],
                "help": "print the kernel function call stack of kfree_skb",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--sock"
                ],
                "help": "enable 'sock' mode",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--monitor"
                ],
                "help": "enable 'monitor' mode",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--rtt"
                ],
                "help": "enable 'rtt' in statistics mode",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--rtt-detail"
                ],
                "help": "enable 'rtt' in detail mode",
                "required": false,
                "type": "bool"
            },
            {
                "default": 0,
                "flags": [
                    "--filter-srtt"
                ],
                "help": "filter by the minial first-acked rtt in ms",
                "required": false,
                "type": "int"
            },
            {
                "default": 0,
                "flags": [
                    "--filter-minrtt"
                ],
                "help": "filter by the minial last-acked rtt in ms",
                "required": false,
                "type": "int"
            },
            {
                "default": false,
                "flags": [
                    "--latency-show"
                ],
                "help": "show latency between kernel functions",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--latency-free"
                ],
                "help": "account the latency of skb free",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--latency"
                ],
                "help": "enable 'latency' mode",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--latency-summary"
                ],
                "help": "show latency by statistics",
                "required": false,
                "type": "bool"
            },
            {
                "default": "",
                "flags": [
                    "-t",
                    "--trace"
                ],
                "help": "enable trace group or trace. Some traces are disabled by default, use \"all\" to enable all",
                "required": false,
                "type": "string"
            },
            {
                "default": false,
                "flags": [
                    "--force"
                ],
                "help": "skip some check and force load nettrace",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--ret"
                ],
                "help": "show function return value",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--detail"
                ],
                "help": "show extern packet info, such as pid, ifname, etc",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--date"
                ],
                "help": "print timestamp in date-time format",
                "required": false,
                "type": "bool"
            },
            {
                "default": 0,
                "flags": [
                    "-c",
                    "--count"
                ],
                "help": "exit after receiving count packets",
                "required": false,
                "type": "int"
            },
            {
                "default": false,
                "flags": [
                    "--hooks"
                ],
                "help": "print netfilter hooks if dropping by netfilter",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--tiny-show"
                ],
                "help": "set this option to show less infomation",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--trace-stack"
                ],
                "help": "print call stack for traces or group",
                "required": false,
                "type": "bool"
            },
            {
                "default": "",
                "flags": [
                    "--trace-matcher"
                ],
                "help": "traces that can match packet(default all)",
                "required": false,
                "type": "string"
            },
            {
                "default": "",
                "flags": [
                    "--trace-exclude"
                ],
                "help": "traces that should be disabled",
                "required": false,
                "type": "string"
            },
            {
                "default": false,
                "flags": [
                    "--trace-noclone"
                ],
                "help": "don't trace skb clone",
                "required": false,
                "type": "bool"
            },
            {
                "default": "",
                "flags": [
                    "--trace-free"
                ],
                "help": "custom the free functions",
                "required": false,
                "type": "string"
            },
            {
                "default": false,
                "flags": [
                    "--func-stats"
                ],
                "help": "only do the statistics for function call",
                "required": false,
                "type": "bool"
            },
            {
                "default": 0,
                "flags": [
                    "--rate-limit"
                ],
                "help": "limit the output to N/s, not valid in diag/default mode",
                "required": false,
                "type": "int"
            },
            {
                "default": "",
                "flags": [
                    "--btf-path"
                ],
                "help": "custom the path of BTF info of vmlinux",
                "required": false,
                "type": "string"
            },
            {
                "default": false,
                "flags": [
                    "-v"
                ],
                "help": "show log information",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "--debug"
                ],
                "help": "show debug information",
                "required": false,
                "type": "bool"
            },
            {
                "default": false,
                "flags": [
                    "-V",
                    "--version"
                ],
                "help": "show nettrace version",
                "required": false,
                "type": "bool"
            }
        ]
    }
}
