#ifndef JUICE_XSDK_COMPAT_H
#define JUICE_XSDK_COMPAT_H

// xSDK's sockets are IPv4 only. These let libjuice's IPv6 paths compile; AF_INET6 sockets fail and it falls back.
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define AF_INET6 23
#define IPPROTO_IPV6 41
#define IPV6_V6ONLY 26
#define INET6_ADDRSTRLEN 46
#define NI_NUMERICHOST 1
#define NI_NUMERICSERV 2
#define NI_DGRAM 16
#define NI_MAXHOST 1025
#define NI_MAXSERV 32

struct in6_addr {
	uint8_t s6_addr[16];
};

struct sockaddr_in6 {
	sa_family_t sin6_family;
	in_port_t sin6_port;
	uint32_t sin6_flowinfo;
	struct in6_addr sin6_addr;
	uint32_t sin6_scope_id;
};

static inline int getnameinfo(const struct sockaddr *address, socklen_t length, char *host, socklen_t host_length,
                              char *service, socklen_t service_length, int flags) {
	(void)length;
	(void)flags;
	if (address->sa_family != AF_INET)
		return -1;
	const struct sockaddr_in *ipv4 = (const struct sockaddr_in *)address;
	if (host && !inet_ntop(AF_INET, &ipv4->sin_addr, host, host_length))
		return -1;
	if (service)
		snprintf(service, service_length, "%u", (unsigned)ntohs(ipv4->sin_port));
	return 0;
}

#endif
