# ipcwatcher - 进程间通信观测工具
## 一、工具介绍
- **简介**
  unix domian socket（uds）作为一种IPC方式，与TCP/IP采用相同的socket接口，在本机通信中广泛应用。本机环境下由于不用经过网络协议栈，所以在性能上比TCP更有优势。比如Linux的X11，就是通过uds通信的。
  同时由于uds具有传递文件描述符的能力，且共享内存作为最快的IPC方式，没有自己的同步机制。因此，一种常规做法是采用uds作为共享内存的同步机制，并通过uds在进程间传递memfd。这种方式在通信中间件的设计实现上很常见，例如字节跳动的采用go语言实现的shmipc。
  **目前的观测手段**
  目前查看下来，观测手段较少。

  - 在系统调用跟踪方面，可以用strace跟踪到。
  - 在信息展示方面，可以用ss观测。
  - 而在数据包跟踪方面，没看到原生的跟踪工具，除了使用bpftrace可以跟踪一下之外，一种常规做法是，借助socat将uds的数据包转发到一个TCP连接上，然后使用wireshark监控TCP数据，间接观测uds的数据。

  不知道是否还有其他的观测工具，大家知道可以说下。
  **结论**
  在经过必要性论证通过后，我们的net工具，增加uds（以及其他IPC）方面的观测。


```c
SEC("kprobe/unix_stream_sendmsg")
int BPF_KPROBE(unix_stream_sendmsg, struct socket *sock, struct msghdr *msg,
			       size_t len) {
    struct uds_event *event;
    u64 current_pid = bpf_get_current_pid_tgid() >> 32;
    struct sock *sk = BPF_CORE_READ(sock, sk);
    // 从 ringbuffer 分配事件内存
    event = bpf_ringbuf_reserve(&uds_events, sizeof(struct uds_event), 0);
    if (!event)
        return 0;
    struct unix_sock *unix_sk = (struct unix_sock*)sk;
    const struct unix_address *addr = BPF_CORE_READ(unix_sk, addr);
    /** 存在显性路径 */
    if (addr) {
        const char *path = BPF_CORE_READ(addr, name->sun_path);
        bpf_probe_read_kernel_str(event->path, sizeof(event->path), path);
    }
    else {
        //if (filter_is_exist_path) {
        //    return 0;
        //}
        //else {
            bpf_probe_read_kernel_str(event->path, 7, "<none>");
       // }

    }

    // 提取 payload 数据（从 msghdr 的 iovec）
    struct iovec *iov = BPF_CORE_READ(msg, msg_iter.iov);
    u32 iov_len = BPF_CORE_READ(msg, msg_iter.nr_segs);
    u32 copied = 0;

    // 使用循环读取 iovec 数据
    if (iov_len > 0) {
        u32 i = 0;
        while (i < iov_len && copied < MAX_PAYLOAD_LEN) {
            struct iovec iov_elem;
            if (bpf_probe_read_kernel_str(&iov_elem, sizeof(iov_elem), &iov[i])) {
                bpf_ringbuf_discard(event, 0);
                return 0;
            }
            u32 iov_len_i = iov_elem.iov_len;
            if ((MAX_PAYLOAD_LEN - copied) < iov_len_i )
                iov_len_i = MAX_PAYLOAD_LEN - copied;

            // 安全读取 iovec 中的数据
            if (bpf_probe_read_kernel_str(event->payload + copied, iov_len_i, (void *)iov_elem.iov_base)) {
                bpf_ringbuf_discard(event, 0);
                return 0;
            }
            copied += iov_len_i;
            i++;
        }
    }


    // 记录 PID
    event->send_pid = current_pid;
    event->recv_pid = 0;
    event->timestamp = 0;
    event->direction = 0;
    event->size = len;
    event->payload[0] = '\0';

    // 提交事件到用户态
    bpf_ringbuf_submit(event, 0);
    return 0;
}
```