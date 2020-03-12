#ifndef SOCKETIV_H
#define SOCKETIV_H

#include <sys/socket.h>
#include <sys/types.h>

int (*orig_open) (const char *, int, mode_t);
int (*orig_socket) (int, int, int);

int (*orig_accept) (int, struct sockaddr *, socklen_t *);
int (*orig_connect) (int, const struct sockaddr *, socklen_t);

ssize_t(*orig_read) (int, void *, size_t);
ssize_t(*orig_write) (int, void *, size_t);

int (*orig_close) (int);

int socketiv_check_vm_subnet(const struct sockaddr *addr);
int socketiv_accept(int new_sockfd);
int socketiv_connect(int sockfd);

int socketiv_check_ivsock(int fd);

ssize_t socketiv_read(int sockfd, void *buf, size_t count);
ssize_t socketiv_write(int sockfd, void *buf, size_t count);

int socketiv_close(int fd);

#endif
