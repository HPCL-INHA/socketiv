#include <assert.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "socketiv.h"

int open(const char *pathname, int flags, mode_t mode) { // open file
	return orig_open(pathname, flags, mode);
}

int socket(int domain, int type, int protocol) { // create socket
	return orig_socket(domain, type, protocol);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t * addrlen) { // create new socket requested by passive socket
	int new_sockfd = orig_accept(sockfd, addr, addrlen);
	if (new_sockfd >= 0)
		if (socketiv_check_vm_subnet(addr))
			assert(!socketiv_accept(new_sockfd));
	return new_sockfd;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) { // connect to server socket
	int ret = orig_connect(sockfd, addr, addrlen);
	if (ret == 0)
		if (socketiv_check_vm_subnet(addr))
			assert(!socketiv_connect(sockfd));
	return ret;
}

ssize_t read(int fd, void *buf, size_t count) { // read from socket
	if (socketiv_check_ivsock(fd))
		return socketiv_read(fd, buf, count);
	return orig_read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) { // write to socket
	if (socketiv_check_ivsock(fd))
		return socketiv_write(fd, buf, count);
	return orig_write(fd, buf, count);
}

int close(int fd) { // close socket
	if (socketiv_check_ivsock(fd))
		assert(!socketiv_close(fd));
	return orig_close(fd);
}
