/*
    Fake arpa/inet.h for Windows/MSYS2
    MTProxy uses ws2tcpip.h for inet_pton/inet_ntop functionality
*/
#ifndef _ARPA_INET_H
#define _ARPA_INET_H

#include <winsock2.h>
#include <ws2tcpip.h>

/* inet_pton, inet_ntop, inet_addr, inet_ntoa are in ws2tcpip.h */

/* Additional functions that may be needed */
#ifndef inet_aton
#define inet_aton inet_addr
#endif

/* htons, htonl, ntohs, ntohl are in winsock2.h */

#endif /* _ARPA_INET_H */
