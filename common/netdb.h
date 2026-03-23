/*
    Fake netdb.h for Windows/MSYS2
    Windows uses Winsock2 for network database functionality
*/
#ifndef _NETDB_H
#define _NETDB_H

#include <winsock2.h>

/* hostent structure is defined in winsock2.h */

/* h_errno - not typically used on Windows */
#ifndef h_errno
extern int h_errno;
#endif

/* Error codes */
#ifndef HOST_NOT_FOUND
#define HOST_NOT_FOUND 1
#endif

#ifndef TRY_AGAIN
#define TRY_AGAIN 2
#endif

#ifndef NO_RECOVERY
#define NO_RECOVERY 3
#endif

#ifndef NO_DATA
#define NO_DATA NO_RECOVERY
#endif

/* gethostbyname, gethostbyaddr are in winsock2.h */
/* getaddrinfo, freeaddrinfo, getnameinfo are in ws2tcpip.h */

#endif /* _NETDB_H */
