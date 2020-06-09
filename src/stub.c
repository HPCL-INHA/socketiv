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
	int new_sockfd = orig_accept(sockfd, addr, addrlen);
	if (new_sockfd >= 0)
		if (socketiv_check_vm_subnet(addr))
			socketiv_accept(new_sockfd);
	return new_sockfd;
}

int connect2(int sockfd, const struct sockaddr *addr, socklen_t addrlen) { // connect to server socket
	int ret = orig_connect(sockfd, addr, addrlen);
	if (ret == 0)
		if (socketiv_check_vm_subnet(addr))
			socketiv_connect(sockfd);
	return ret;
}

ssize_t read2(int fd, void *buf, size_t count) { // read from socket
	if (socketiv_check_ivsock(fd))
		return socketiv_read(fd, buf, count);
	return orig_read(fd, buf, count);
}

ssize_t write2(int fd, const void *buf, size_t count) { // write to socket
	if (socketiv_check_ivsock(fd))
		return socketiv_write(fd, buf, count);
	return orig_write(fd, buf, count);
}

ssize_t recv2(int sockfd, void *buf, size_t len, int flags) {
	if (socketiv_check_ivsock(sockfd))
		return socketiv_read(sockfd, buf, len);
	return orig_recv(sockfd, buf, len, flags);
}

ssize_t recvfrom2(int sockfd, void *buf, size_t len, int flags,
		const struct sockaddr *dest_addr, socklen_t addrlen) {
	if (socketiv_check_ivsock(sockfd))
		return socketiv_read(sockfd, buf, len);
	return orig_recvfrom(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t send2(int sockfd, const void *buf, size_t len, int flags) {
	if (socketiv_check_ivsock(sockfd))
		return socketiv_write(sockfd, buf, len);
	return orig_send(sockfd, buf, len, flags);
}

ssize_t sendto2(int sockfd, const void *buf, size_t len, int flags,
		const struct sockaddr *dest_addr, socklen_t addrlen) {
	if (socketiv_check_ivsock(sockfd))
		return socketiv_write(sockfd, buf, len);
	return orig_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

int close2(int fd) { // close socket
	if (socketiv_check_ivsock(fd))
		socketiv_close(fd);
	return orig_close(fd);
}
