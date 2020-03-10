#ifndef SOCKETIV_H
#define SOCKETIV_H

int (*orig_open) (const char *, int, mode_t);
int (*orig_socket) (int, int, int);

int (*orig_accept) (int, struct sockaddr *, socklen_t *);
int (*orig_connect) (int, const struct sockaddr *, socklen_t);

ssize_t(*orig_read) (int, void *, size_t);
ssize_t(*orig_write) (int, void *, size_t);

int (*orig_close) (int);

typedef enum socketiv_fd_type {
	SOCKETIV_FD_TYPE_INV = 0,
	SOCKETIV_FD_TYPE_GEN = 1,
	SOCKETIV_FD_TYPE_IVSOCK = 2
} SOCKETIV_FD_TYPE;

void socketiv_register_fd(int fd, SOCKETIV_FD_TYPE fd_type);
SOCKETIV_FD_TYPE socketiv_check_fd(int fd);
void socketiv_unregister_fd(int fd);

bool socketiv_check_vm_subnet(const struct sockaddr *addr);
int socketiv_accept(int new_sockfd);
int socketiv_connect(int sockfd);

ssize_t socketiv_read(int sockfd, void *buf, size_t count);
ssize_t socketiv_write(int sockfd, void *buf, size_t count);

int socketiv_close(int fd);

#endif
