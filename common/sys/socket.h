/*
    Fake sys/socket.h for Windows/MSYS2
    MTProxy uses winsock2.h for socket functionality
*/
#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

/* winsock2.h provides all necessary socket definitions */
#include <winsock2.h>

#endif /* _SYS_SOCKET_H */
