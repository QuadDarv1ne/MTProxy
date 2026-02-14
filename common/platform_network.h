#ifndef PLATFORM_NETWORK_H
#define PLATFORM_NETWORK_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    
    // Windows socket type compatibility
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_FD INVALID_SOCKET
    #define SOCKET_ERROR_CODE SOCKET_ERROR
    
    // Windows specific definitions
    #define SHUT_RD SD_RECEIVE
    #define SHUT_WR SD_SEND
    #define SHUT_RDWR SD_BOTH
    
    // Error handling
    #define GET_SOCKET_ERROR() WSAGetLastError()
    #define IS_SOCKET_ERROR(err) ((err) == SOCKET_ERROR)
    
#else
    // Unix/Linux includes
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    
    // Unix socket type compatibility
    typedef int socket_t;
    #define INVALID_SOCKET_FD -1
    #define SOCKET_ERROR_CODE -1
    
    // Error handling
    #define GET_SOCKET_ERROR() errno
    #define IS_SOCKET_ERROR(err) ((err) < 0)
    
#endif

// Common network functions with platform abstraction
int platform_socket_init();
int platform_socket_cleanup();
socket_t platform_socket_create(int domain, int type, int protocol);
int platform_socket_close(socket_t sock);
int platform_socket_bind(socket_t sock, const struct sockaddr *addr, socklen_t addrlen);
int platform_socket_connect(socket_t sock, const struct sockaddr *addr, socklen_t addrlen);
int platform_socket_listen(socket_t sock, int backlog);
socket_t platform_socket_accept(socket_t sock, struct sockaddr *addr, socklen_t *addrlen);
ssize_t platform_socket_send(socket_t sock, const void *buf, size_t len, int flags);
ssize_t platform_socket_recv(socket_t sock, void *buf, size_t len, int flags);
int platform_socket_set_nonblocking(socket_t sock, int nonblocking);
int platform_socket_get_error();

// Address resolution
int platform_getaddrinfo(const char *node, const char *service,
                        const struct addrinfo *hints, struct addrinfo **res);
void platform_freeaddrinfo(struct addrinfo *res);

// Utility functions
const char *platform_inet_ntop(int af, const void *src, char *dst, socklen_t size);
int platform_inet_pton(int af, const char *src, void *dst);

#endif // PLATFORM_NETWORK_H