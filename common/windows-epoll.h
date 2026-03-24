/*
    Windows epoll emulation header
    
    Provides epoll-compatible API for Windows using WSAPoll.
*/

#ifndef WINDOWS_EPOLL_H
#define WINDOWS_EPOLL_H

#ifdef _WIN32

#include <stdint.h>

// epoll event flags
#define EPOLLIN     0x001
#define EPOLLOUT    0x002
#define EPOLLERR    0x008
#define EPOLLHUP    0x010
#define EPOLLPRI    0x040

// epoll control commands
#define EPOLL_CTL_ADD   1
#define EPOLL_CTL_MOD   2
#define EPOLL_CTL_DEL   3

// epoll_event structure
struct epoll_event {
    uint32_t events;
    union {
        void *ptr;
        int fd;
    } data;
};

// epoll functions
int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
int epoll_close(int epfd);

#endif // _WIN32

#endif // WINDOWS_EPOLL_H
