/*
    Windows epoll emulation using WSAPoll
    
    This module provides epoll-compatible API for Windows using WSAPoll.
    WSAPoll is available on Windows Vista+ and provides better performance
    than select() for large numbers of sockets.
    
    Features:
    - epoll_create emulation
    - epoll_ctl emulation (ADD/MOD/DEL)
    - epoll_wait emulation via WSAPoll
    - Event flag conversion (EPOLLIN/EPOLLOUT/EPOLLERR)
*/

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Maximum number of file descriptors
#define MAX_EPOLL_FDS   1024

// epoll_event structure
struct epoll_event {
    uint32_t events;
    union {
        void *ptr;
        int fd;
    } data;
};

// epoll file descriptor set
typedef struct {
    SOCKET sockets[MAX_EPOLL_FDS];
    struct epoll_event events[MAX_EPOLL_FDS];
    int fd_map[MAX_EPOLL_FDS];  // Maps index to fd
    int count;
    int epoll_fd;
} epoll_fd_set_t;

// Global epoll sets
static epoll_fd_set_t epoll_sets[64];
static int epoll_sets_count = 0;

// Convert epoll events to WSAPoll events
static short epoll_to_wsapoll_events(uint32_t epoll_events) {
    short wsapoll_events = 0;
    
    if (epoll_events & EPOLLIN) {
        wsapoll_events |= POLLRDNORM | POLLIN;
    }
    if (epoll_events & EPOLLOUT) {
        wsapoll_events |= POLLWRNORM | POLLOUT;
    }
    if (epoll_events & EPOLLERR) {
        wsapoll_events |= POLLERR;
    }
    if (epoll_events & EPOLLHUP) {
        wsapoll_events |= POLLHUP;
    }
    if (epoll_events & EPOLLPRI) {
        wsapoll_events |= POLLPRI;
    }
    
    return wsapoll_events;
}

// Convert WSAPoll events to epoll events
static uint32_t wsapoll_to_epoll_events(short wsapoll_events) {
    uint32_t epoll_events = 0;
    
    if (wsapoll_events & (POLLRDNORM | POLLIN | POLLMSG)) {
        epoll_events |= EPOLLIN;
    }
    if (wsapoll_events & (POLLWRNORM | POLLOUT | POLLWRBAND)) {
        epoll_events |= EPOLLOUT;
    }
    if (wsapoll_events & POLLERR) {
        epoll_events |= EPOLLERR;
    }
    if (wsapoll_events & POLLHUP) {
        epoll_events |= EPOLLHUP;
    }
    if (wsapoll_events & POLLPRI) {
        epoll_events |= EPOLLPRI;
    }
    if (wsapoll_events & POLLNVAL) {
        epoll_events |= EPOLLERR;
    }
    
    return epoll_events;
}

// Find epoll set by fd
static epoll_fd_set_t* find_epoll_set(int fd) {
    for (int i = 0; i < epoll_sets_count; i++) {
        for (int j = 0; j < epoll_sets[i].count; j++) {
            if (epoll_sets[i].fd_map[j] == fd) {
                return &epoll_sets[i];
            }
        }
    }
    return NULL;
}

// Find free index in epoll set
static int find_free_index(epoll_fd_set_t *set) {
    for (int i = 0; i < set->count; i++) {
        if (set->sockets[i] == INVALID_SOCKET) {
            return i;
        }
    }
    return -1;
}

// Create epoll instance
int epoll_create(int size) {
    if (epoll_sets_count >= 64) {
        errno = EMFILE;
        return -1;
    }
    
    epoll_fd_set_t *set = &epoll_sets[epoll_sets_count];
    memset(set, 0, sizeof(epoll_fd_set_t));
    
    for (int i = 0; i < MAX_EPOLL_FDS; i++) {
        set->sockets[i] = INVALID_SOCKET;
        set->fd_map[i] = -1;
    }
    
    set->epoll_fd = epoll_sets_count + 100;  // Unique fd
    set->count = 0;
    epoll_sets_count++;
    
    return set->epoll_fd;
}

// Control epoll (add/modify/delete)
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    if (epfd < 100 || epfd >= 100 + epoll_sets_count) {
        errno = EBADF;
        return -1;
    }
    
    epoll_fd_set_t *set = &epoll_sets[epfd - 100];
    
    // Find existing fd
    int idx = -1;
    for (int i = 0; i < set->count; i++) {
        if (set->fd_map[i] == fd) {
            idx = i;
            break;
        }
    }
    
    switch (op) {
        case EPOLL_CTL_ADD:
            if (idx >= 0) {
                errno = EEXIST;
                return -1;
            }
            
            // Find free slot
            int free_idx = find_free_index(set);
            if (free_idx < 0 && set->count >= MAX_EPOLL_FDS) {
                errno = ENOMEM;
                return -1;
            }
            
            // Add new entry
            if (free_idx < 0) {
                free_idx = set->count++;
            }
            
            set->sockets[free_idx] = (SOCKET)fd;
            set->fd_map[free_idx] = fd;
            if (event) {
                set->events[free_idx] = *event;
            }
            break;
            
        case EPOLL_CTL_MOD:
            if (idx < 0) {
                errno = ENOENT;
                return -1;
            }
            
            if (event) {
                set->events[idx] = *event;
            }
            break;
            
        case EPOLL_CTL_DEL:
            if (idx < 0) {
                errno = ENOENT;
                return -1;
            }
            
            set->sockets[idx] = INVALID_SOCKET;
            set->fd_map[idx] = -1;
            memset(&set->events[idx], 0, sizeof(struct epoll_event));
            break;
            
        default:
            errno = EINVAL;
            return -1;
    }
    
    return 0;
}

// Wait for events
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    if (epfd < 100 || epfd >= 100 + epoll_sets_count) {
        errno = EBADF;
        return -1;
    }
    
    epoll_fd_set_t *set = &epoll_sets[epfd - 100];
    
    if (set->count == 0) {
        if (timeout > 0) {
            Sleep(timeout);
        }
        return 0;
    }
    
    // Build WSAPoll fds
    struct WSAPOLLFD *wsapoll_fds = (struct WSAPOLLFD*)malloc(set->count * sizeof(struct WSAPOLLFD));
    if (!wsapoll_fds) {
        errno = ENOMEM;
        return -1;
    }
    
    int active_count = 0;
    for (int i = 0; i < set->count; i++) {
        if (set->sockets[i] != INVALID_SOCKET) {
            wsapoll_fds[active_count].fd = set->sockets[i];
            wsapoll_fds[active_count].events = epoll_to_wsapoll_events(set->events[i].events);
            wsapoll_fds[active_count].revents = 0;
            active_count++;
        }
    }
    
    if (active_count == 0) {
        free(wsapoll_fds);
        if (timeout > 0) {
            Sleep(timeout);
        }
        return 0;
    }
    
    // Call WSAPoll
    int ret = WSAPoll(wsapoll_fds, active_count, timeout);
    
    if (ret == SOCKET_ERROR) {
        free(wsapoll_fds);
        errno = WSAGetLastError();
        return -1;
    }
    
    // Convert results
    int event_count = 0;
    for (int i = 0; i < active_count && event_count < maxevents; i++) {
        if (wsapoll_fds[i].revents != 0) {
            events[event_count].events = wsapoll_to_epoll_events(wsapoll_fds[i].revents);
            events[event_count].data.fd = set->fd_map[i];
            event_count++;
        }
    }
    
    free(wsapoll_fds);
    return event_count;
}

// Close epoll instance
int epoll_close(int epfd) {
    if (epfd < 100 || epfd >= 100 + epoll_sets_count) {
        errno = EBADF;
        return -1;
    }
    
    epoll_fd_set_t *set = &epoll_sets[epfd - 100];
    memset(set, 0, sizeof(epoll_fd_set_t));
    
    // Shift remaining sets
    for (int i = epfd - 100; i < epoll_sets_count - 1; i++) {
        epoll_sets[i] = epoll_sets[i + 1];
        epoll_sets[i].epoll_fd = i + 100;
    }
    
    epoll_sets_count--;
    return 0;
}

#endif // _WIN32
