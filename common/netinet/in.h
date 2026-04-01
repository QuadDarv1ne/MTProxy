/*
    Fake netinet/in.h for Windows/MSYS2
    MTProxy uses winsock2.h for socket functionality
*/
#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#include <winsock2.h>
#include <ws2tcpip.h>

/* POSIX compatibility - already provided by winsock2.h */
#ifndef in_port_t
typedef uint16_t in_port_t;
#endif

#ifndef in_addr_t
typedef uint32_t in_addr_t;
#endif

/* These are already defined in winsock2.h and ws2tcpip.h */
/* struct in_addr - defined in winsock2.h */
/* struct in6_addr - defined in ws2tcpip.h */
/* struct sockaddr_in - defined in winsock2.h */
/* struct sockaddr_in6 - defined in ws2tcpip.h */

/* Additional POSIX definitions */
#ifndef IPPROTO_IP
#define IPPROTO_IP 0
#endif

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif

#ifndef IPPROTO_UDP
#define IPPROTO_UDP 17
#endif

#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 41
#endif

#endif /* _NETINET_IN_H */
