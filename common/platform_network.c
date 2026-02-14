#include "platform_network.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #pragma comment(lib, "ws2_32.lib")
    
    static int winsock_initialized = 0;
    
    int platform_socket_init() {
        if (winsock_initialized) {
            return 0;
        }
        
        WSADATA wsa_data;
        int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0) {
            return -1;
        }
        
        winsock_initialized = 1;
        return 0;
    }
    
    int platform_socket_cleanup() {
        if (winsock_initialized) {
            WSACleanup();
            winsock_initialized = 0;
        }
        return 0;
    }
    
    socket_t platform_socket_create(int domain, int type, int protocol) {
        return socket(domain, type, protocol);
    }
    
    int platform_socket_close(socket_t sock) {
        return closesocket(sock);
    }
    
    int platform_socket_bind(socket_t sock, const struct sockaddr *addr, socklen_t addrlen) {
        return bind(sock, addr, addrlen);
    }
    
    int platform_socket_connect(socket_t sock, const struct sockaddr *addr, socklen_t addrlen) {
        return connect(sock, addr, addrlen);
    }
    
    int platform_socket_listen(socket_t sock, int backlog) {
        return listen(sock, backlog);
    }
    
    socket_t platform_socket_accept(socket_t sock, struct sockaddr *addr, socklen_t *addrlen) {
        return accept(sock, addr, addrlen);
    }
    
    ssize_t platform_socket_send(socket_t sock, const void *buf, size_t len, int flags) {
        return send(sock, (const char*)buf, (int)len, flags);
    }
    
    ssize_t platform_socket_recv(socket_t sock, void *buf, size_t len, int flags) {
        return recv(sock, (char*)buf, (int)len, flags);
    }
    
    int platform_socket_set_nonblocking(socket_t sock, int nonblocking) {
        u_long mode = nonblocking ? 1 : 0;
        return ioctlsocket(sock, FIONBIO, &mode);
    }
    
    int platform_socket_get_error() {
        return WSAGetLastError();
    }
    
    int platform_getaddrinfo(const char *node, const char *service,
                            const struct addrinfo *hints, struct addrinfo **res) {
        return getaddrinfo(node, service, hints, res);
    }
    
    void platform_freeaddrinfo(struct addrinfo *res) {
        freeaddrinfo(res);
    }
    
    const char *platform_inet_ntop(int af, const void *src, char *dst, socklen_t size) {
        struct sockaddr_storage ss;
        memset(&ss, 0, sizeof(ss));
        ss.ss_family = af;
        
        if (af == AF_INET) {
            ((struct sockaddr_in*)&ss)->sin_addr = *(struct in_addr*)src;
        } else if (af == AF_INET6) {
            ((struct sockaddr_in6*)&ss)->sin6_addr = *(struct in6_addr*)src;
        } else {
            return NULL;
        }
        
        DWORD dst_len = size;
        if (WSAAddressToString((struct sockaddr*)&ss, sizeof(ss), NULL, dst, &dst_len) != 0) {
            return NULL;
        }
        return dst;
    }
    
    int platform_inet_pton(int af, const char *src, void *dst) {
        struct sockaddr_storage ss;
        int size = sizeof(ss);
        char src_copy[INET6_ADDRSTRLEN + 1];
        
        strncpy(src_copy, src, INET6_ADDRSTRLEN);
        src_copy[INET6_ADDRSTRLEN] = '\0';
        
        if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr*)&ss, &size) == 0) {
            if (af == AF_INET) {
                *(struct in_addr*)dst = ((struct sockaddr_in*)&ss)->sin_addr;
                return 1;
            } else if (af == AF_INET6) {
                *(struct in6_addr*)dst = ((struct sockaddr_in6*)&ss)->sin6_addr;
                return 1;
            }
        }
        return 0;
    }
    
#else
    // Unix/Linux implementation
    #include <sys/ioctl.h>
    
    int platform_socket_init() {
        return 0; // No initialization needed on Unix
    }
    
    int platform_socket_cleanup() {
        return 0; // No cleanup needed on Unix
    }
    
    socket_t platform_socket_create(int domain, int type, int protocol) {
        return socket(domain, type, protocol);
    }
    
    int platform_socket_close(socket_t sock) {
        return close(sock);
    }
    
    int platform_socket_bind(socket_t sock, const struct sockaddr *addr, socklen_t addrlen) {
        return bind(sock, addr, addrlen);
    }
    
    int platform_socket_connect(socket_t sock, const struct sockaddr *addr, socklen_t addrlen) {
        return connect(sock, addr, addrlen);
    }
    
    int platform_socket_listen(socket_t sock, int backlog) {
        return listen(sock, backlog);
    }
    
    socket_t platform_socket_accept(socket_t sock, struct sockaddr *addr, socklen_t *addrlen) {
        return accept(sock, addr, addrlen);
    }
    
    ssize_t platform_socket_send(socket_t sock, const void *buf, size_t len, int flags) {
        return send(sock, buf, len, flags);
    }
    
    ssize_t platform_socket_recv(socket_t sock, void *buf, size_t len, int flags) {
        return recv(sock, buf, len, flags);
    }
    
    int platform_socket_set_nonblocking(socket_t sock, int nonblocking) {
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags == -1) {
            return -1;
        }
        
        if (nonblocking) {
            flags |= O_NONBLOCK;
        } else {
            flags &= ~O_NONBLOCK;
        }
        
        return fcntl(sock, F_SETFL, flags);
    }
    
    int platform_socket_get_error() {
        return errno;
    }
    
    int platform_getaddrinfo(const char *node, const char *service,
                            const struct addrinfo *hints, struct addrinfo **res) {
        return getaddrinfo(node, service, hints, res);
    }
    
    void platform_freeaddrinfo(struct addrinfo *res) {
        freeaddrinfo(res);
    }
    
    const char *platform_inet_ntop(int af, const void *src, char *dst, socklen_t size) {
        return inet_ntop(af, src, dst, size);
    }
    
    int platform_inet_pton(int af, const char *src, void *dst) {
        return inet_pton(af, src, dst);
    }
    
#endif