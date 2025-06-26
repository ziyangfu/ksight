使用范例

```bash
sudo ./ipcwatcher -p <pid>



```


```bash
fzy@fzy-Lenovo:~/Downloads/04_bcc_ebpf/ksight/build/src/backend/ipc/ipc_watcher$ sudo ./ipcwatcher -u |grep 2278245
286574682620   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286575683126   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286576684392   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286577685683   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286578687164   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286579688227   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286580689499   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>      
```


带payload，原始信息
```bash
fzy@fzy-Lenovo:~/Downloads/04_bcc_ebpf/ksight/build/src/backend/ipc/ipc_watcher$ sudo ./ipcwatcher -u | grep 2294614
288555552381 2294625    /usr/bin/python3                    2294614    /usr/bin/python3                    14         SOCK_STREAM  <none>                         Hello, Server!                                              
288556553661 2294625    /usr/bin/python3                    2294614    /usr/bin/python3                    14         SOCK_STREAM  <none>                         Hello, Server!                                              
288557555000 2294625    /usr/bin/python3                    2294614    /usr/bin/python3                    14         SOCK_STREAM  <none>                         Hello, Server!      
```

带payload hex输出
```bash
fzy@fzy-Lenovo:~/Downloads/04_bcc_ebpf/ksight/build/src/backend/ipc/ipc_watcher$ sudo ./ipcwatcher -u | grep 2295660
289064585472 2295661    /usr/bin/python3                    2295660    /usr/bin/python3                    14         SOCK_STREAM  <none>                         48 65 6c 6c 6f 2c 20 53 65 72 76 65 72 21                   
289065586492 2295661    /usr/bin/python3                    2295660    /usr/bin/python3                    14         SOCK_STREAM  <none>                         48 65 6c 6c 6f 2c 20 53 65 72 76 65 72 21                   
289066587943 2295661    /usr/bin/python3                    2295660    /usr/bin/python3                    14         SOCK_STREAM  <none>                         48 65 6c 6c 6f 2c 20 53 65 72 76 65 72 21                   

```

实例：
```bash
fzy@fzy-Lenovo:<xx>/src/backend/ipc/ipc_watcher$ sudo ./ipcwatcher -u
config_.pid val = 0Tracing UDS send/recv events... Ctrl+C to exit
Timestamp      sendPID    sendComm                       recvPID    recvComm                       Size       Type         Path                          
284596670884   2212981    /snap/clion/353/bin/clion      822        /usr/sbin/rsyslogd             133        SOCK_STREAM  <none>                        
284598877992   1082459    /opt/wechat/RadiumWMPF/runtime 1          /sbin/init                     104        SOCK_STREAM  <none>                        
284609802564   2337       /opt/sogoupinyin/files/bin/sog 822        /usr/sbin/rsyslogd             76         SOCK_STREAM  <none>                        
284611106344   2148       /usr/lib/xorg/Xorg             822        /usr/sbin/rsyslogd             8          SOCK_STREAM                                
284621541717   2148       /usr/lib/xorg/Xorg             822        /usr/sbin/rsyslogd             8          SOCK_STREAM                                
284621652874   2148       /usr/lib/xorg/Xorg             1          /sbin/init                     40         SOCK_STREAM                                
284623333648   2148       /usr/lib/xorg/Xorg             822        /usr/sbin/rsyslogd             8          SOCK_STREAM                                
284623438030   2148       /usr/lib/xorg/Xorg             822        /usr/sbin/rsyslogd             40         SOCK_STREAM                                
284623450421   170957     /usr/share/code/code           822        /usr/sbin/rsyslogd             129        SOCK_STREAM  <none>                        
284628485607   1082459    /opt/wechat/RadiumWMPF/runtime 822        /usr/sbin/rsyslogd             104        SOCK_STREAM  <none>                        
284634717671   1082205    /opt/wechat/RadiumWMPF/runtime 308        /lib/systemd/systemd-journald  104        SOCK_STREAM  <none>                        
284634718567   1082459    /opt/wechat/RadiumWMPF/runtime 822        /usr/sbin/rsyslogd             104        SOCK_STREAM  <none>                        
284634737352   807        /usr/bin/dbus-daemon           308        /lib/systemd/systemd-journald  255        SOCK_STREAM  /run/dbus/system_bus_socket   
284634744743   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16376      SOCK_STREAM  <none>                        
284634745463   2289       /usr/bin/gnome-shell           308        /lib/systemd/systemd-journald  20         SOCK_STREAM  <none>                        
284634745768   2212981    /snap/clion/353/bin/clion      822        /usr/sbin/rsyslogd             16392      SOCK_STREAM  <none>                        
284634746359   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16392      SOCK_STREAM  <none>                        
284634747035   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16384      SOCK_STREAM  <none>                        
284634747731   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16364      SOCK_STREAM  <none>                        
284634748307   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16380      SOCK_STREAM  <none>                        
284634748732   807        /usr/bin/dbus-daemon           308        /lib/systemd/systemd-journald  72         SOCK_STREAM  /run/dbus/system_bus_socket   
284634749021   1          /sbin/init                     822        /usr/sbin/rsyslogd             178        SOCK_STREAM  <none>                        
284634749503   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16372      SOCK_STREAM  <none>                        
284634750052   2148       /usr/lib/xorg/Xorg             308        /lib/systemd/systemd-journald  32         SOCK_STREAM                                
284634752097   1082459    /opt/wechat/RadiumWMPF/runtime 308        /lib/systemd/systemd-journald  104        SOCK_STREAM  <none>                        
284634770809   1          /sbin/init                     308        /lib/systemd/systemd-journald  1092       SOCK_STREAM  <none>                        
284634771000   1          /sbin/init                     822        /usr/sbin/rsyslogd             1092       SOCK_STREAM  <none>                        
284634771164   1          /sbin/init                     1          /sbin/init                     224        SOCK_STREAM  <none>                        
284634780870   2269694    /usr/lib/NetworkManager/nm-dis 308        /lib/systemd/systemd-journald  184        SOCK_STREAM  <none>                        
284634781231   807        /usr/bin/dbus-daemon           308        /lib/systemd/systemd-journald  215        SOCK_STREAM  /run/dbus/system_bus_socket   
284645007260   807        /usr/bin/dbus-daemon           308        /lib/systemd/systemd-journald  189        SOCK_STREAM  /run/dbus/system_bus_socket   
284645007323   807        /usr/bin/dbus-daemon           822        /usr/sbin/rsyslogd             189        SOCK_STREAM  /run/dbus/system_bus_socket   
284621653139   842        /lib/systemd/systemd-logind    940682     /usr/share/code/code           10         SOCK_DGRAM   <none>                        
284621653134   350        /lib/systemd/systemd-udevd     1554       /usr/bin/dockerd               10         SOCK_DGRAM   <none>                        
284665225205   2289       /usr/bin/gnome-shell           308        /lib/systemd/systemd-journald  24         SOCK_STREAM  <none>                        
284665225458   2148       /usr/lib/xorg/Xorg             822        /usr/sbin/rsyslogd             32         SOCK_STREAM                               
```