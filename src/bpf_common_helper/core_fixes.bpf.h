/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
#ifndef __CORE_FIXES_BPF_H
#define __CORE_FIXES_BPF_H

#include <vmlinux.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>

/*
 * The inet_sock bitfields has been moved to a new inet_flags field in 6.6
 * kernel, see 159987ef7260 ("inet: move the 1st group of inet_sock bitfields into a single field")
 */

struct inet_sock___o {
	__u8 freebind: 1;
	__u8 transparent: 1;
	__u8 bind_address_no_port: 1;
};

enum {
	INET_FLAGS_FREEBIND___x = 11,
	INET_FLAGS_TRANSPARENT___x = 15,
	INET_FLAGS_BIND_ADDRESS_NO_PORT___x = 18,
};

struct inet_sock___x {
	unsigned long inet_flags;
};

static __always_inline __u8 get_inet_sock_freebind(void *inet_sock)
{
	unsigned long inet_flags;

	if (bpf_core_field_exists(struct inet_sock___o, freebind))
		return BPF_CORE_READ_BITFIELD_PROBED((struct inet_sock___o *)inet_sock, freebind);

	inet_flags = BPF_CORE_READ((struct inet_sock___x *)inet_sock, inet_flags);
	return (1 << INET_FLAGS_FREEBIND___x) & inet_flags ? 1 : 0;
}

static __always_inline __u8 get_inet_sock_transparent(void *inet_sock)
{
	unsigned long inet_flags;

	if (bpf_core_field_exists(struct inet_sock___o, transparent))
		return BPF_CORE_READ_BITFIELD_PROBED((struct inet_sock___o *)inet_sock, transparent);

	inet_flags = BPF_CORE_READ((struct inet_sock___x *)inet_sock, inet_flags);
	return (1 << INET_FLAGS_TRANSPARENT___x) & inet_flags ? 1 : 0;
}

static __always_inline __u8 get_inet_sock_bind_address_no_port(void *inet_sock)
{
	unsigned long inet_flags;

	if (bpf_core_field_exists(struct inet_sock___o, bind_address_no_port))
		return BPF_CORE_READ_BITFIELD_PROBED((struct inet_sock___o *)inet_sock, bind_address_no_port);

	inet_flags = BPF_CORE_READ((struct inet_sock___x *)inet_sock, inet_flags);
	return (1 << INET_FLAGS_BIND_ADDRESS_NO_PORT___x) & inet_flags ? 1 : 0;
}

#endif /* __CORE_FIXES_BPF_H */
