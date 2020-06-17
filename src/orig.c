#include <stdio.h>
#include <sys/socket.h>

#include "socketiv.h"

#if 0
int open(const char *pathname, int flags, mode_t mode) { // open file
	return orig_open(pathname, flags, mode);
}

int socket(int domain, int type, int protocol) { // create socket
	return orig_socket(domain, type, protocol);
}
#endif

int accept2(int sockfd, struct sockaddr *addr, socklen_t * addrlen) { // create new socket requested by passive socket
	return orig_accept(sockfd, addr, addrlen);
}

int connect2(int sockfd, const struct sockaddr *addr, socklen_t addrlen) { // connect to server socket
	return orig_connect(sockfd, addr, addrlen);
}

ssize_t read2(int fd, void *buf, size_t count) { // read from socket
	return orig_read(fd, buf, count);
}

ssize_t write2(int fd, const void *buf, size_t count) { // write to socket
	return orig_write(fd, buf, count);
}

ssize_t recv2(int sockfd, void *buf, size_t len, int flags) {
	return orig_recv(sockfd, buf, len, flags);
}

ssize_t recvfrom2(int sockfd, void *buf, size_t len, int flags,
		const struct sockaddr *dest_addr, socklen_t addrlen) {
	return orig_recvfrom(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t send2(int sockfd, const void *buf, size_t len, int flags) {
	return orig_send(sockfd, buf, len, flags);
}

ssize_t sendto2(int sockfd, const void *buf, size_t len, int flags,
		const struct sockaddr *dest_addr, socklen_t addrlen) {
	return orig_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

int close2(int fd) { // close socket
	return orig_close(fd);
}
