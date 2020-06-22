#ifndef SOCKETIV_H
#define SOCKETIV_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/socket.h>

#include "intr.h"

#define barrier() __asm__ __volatile__("": : :"memory")
#define TOTAL_SIZE (512 * 1024 * 1024)
#define OFFSET (1024 * 1024)
#define ENDPOINT (TOTAL_SIZE - OFFSET)

#define unlikely(x)     __builtin_expect(!!(x), 0)

int (*orig_open) (const char *, int, mode_t);
int (*orig_socket) (int, int, int);

int (*orig_accept) (int, struct sockaddr *, socklen_t *);
int (*orig_connect) (int, const struct sockaddr *, socklen_t);

ssize_t(*orig_read) (int, void *, size_t);
ssize_t(*orig_write) (int, const void *, size_t);

ssize_t (*orig_recv)(int, void *, size_t, int);
ssize_t (*orig_recvfrom)(int, void *, size_t, int, const struct sockaddr *, socklen_t);

ssize_t (*orig_send)(int, const void *, size_t, int);
ssize_t (*orig_sendto)(int, const void *, size_t, int, const struct sockaddr *, socklen_t);

int (*orig_close) (int);
int (*orig_shutdown) (int, int);

bool socketiv_check_vm_subnet(const struct sockaddr *addr);
int socketiv_accept(int new_sockfd);
int socketiv_connect(int sockfd);

bool socketiv_check_ivsock(int fd);
ssize_t socketiv_read(int sockfd, void *buf, size_t count);
ssize_t socketiv_write(int sockfd, const void *buf, size_t count);

int socketiv_close(int sockfd);

#endif
