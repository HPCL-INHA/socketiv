#include <stdio.h>
#include <sys/socket.h>

#include "socketiv.h"

#if 0
int open(const char *pathname, int flags, mode_t mode) { // open file
	return orig_open(pathname, flags, mode);
}
#endif

int socket(int domain, int type, int protocol) { // create socket
	int ret = orig_socket(domain, type, protocol);
	if (ret >= 0) {
		if (true) {
			socketiv_socket(ret);
		}
	}
	return ret;
}

int accept2(int sockfd, struct sockaddr *addr, socklen_t * addrlen) { // create new socket requested by passive socket
printf("IVSH: %s++\n", __func__);
	int new_sockfd = orig_accept(sockfd, addr, addrlen);
	if (new_sockfd >= 0) {
		if (socketiv_check_vm_subnet(addr)) {
			printf("IVSH: %s++++\n", __func__);
			socketiv_accept(new_sockfd);
			printf("IVSH: NEW FD: %d\n", new_sockfd);
			printf("IVSH: %s----\n", __func__);
		}
	}
printf("IVSH: %s--\n", __func__);
	return new_sockfd;
}

int connect2(int sockfd, const struct sockaddr *addr, socklen_t addrlen) { // connect to server socket
printf("IVSH: %s++\n", __func__);
	int ret = orig_connect(sockfd, addr, addrlen);
	if (ret == 0) {
		if (socketiv_check_vm_subnet(addr)) {
			printf("IVSH: %s++++\n", __func__);
			socketiv_connect(sockfd);
			printf("IVSH: %s----\n", __func__);
		}
	}
printf("IVSH: %s--\n", __func__);
	return ret;
}

ssize_t read2(int fd, void *buf, size_t count) { // read from socket
printf("IVSH: %s++\n", __func__);
	if (socketiv_check_ivsock(fd)) {
		printf("IVSH: %s++++\n", __func__);
		printf("IVSH: %s----\n", __func__);
		return socketiv_read(fd, buf, count);
	}
printf("IVSH: %s--\n", __func__);
	return orig_read(fd, buf, count);
}

ssize_t write2(int fd, const void *buf, size_t count) { // write to socket
printf("IVSH: %s++\n", __func__);
	if (socketiv_check_ivsock(fd)) {
		printf("IVSH: %s++++\n", __func__);
		printf("IVSH: %s----\n", __func__);
		return socketiv_write(fd, buf, count);
	}
printf("IVSH: %s--\n", __func__);
	return orig_write(fd, buf, count);
}

ssize_t recv2(int sockfd, void *buf, size_t len, int flags) {
printf("IVSH: %s++: %d\n", __func__, sockfd);
	if (socketiv_check_ivsock(sockfd)) {
		printf("IVSH: %s++++\n", __func__);
		printf("IVSH: %s----\n", __func__);
		return socketiv_read(sockfd, buf, len);
	}
printf("IVSH: %s--\n", __func__);
	return orig_recv(sockfd, buf, len, flags);
}

ssize_t recvfrom2(int sockfd, void *buf, size_t len, int flags,
		const struct sockaddr *dest_addr, socklen_t addrlen) {
printf("IVSH: %s++\n", __func__);
	if (socketiv_check_ivsock(sockfd)) {
		printf("IVSH: %s++++\n", __func__);
		printf("IVSH: %s----\n", __func__);
		return socketiv_read(sockfd, buf, len);
	}
printf("IVSH: %s--\n", __func__);
	return orig_recvfrom(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t send2(int sockfd, const void *buf, size_t len, int flags) {
printf("IVSH: %s++\n", __func__);
	if (socketiv_check_ivsock(sockfd)) {
		printf("IVSH: %s++++\n", __func__);
		printf("IVSH: %s----\n", __func__);
		return socketiv_write(sockfd, buf, len);
	}
printf("IVSH: %s--\n", __func__);
	return orig_send(sockfd, buf, len, flags);
}

ssize_t sendto2(int sockfd, const void *buf, size_t len, int flags,
		const struct sockaddr *dest_addr, socklen_t addrlen) {
	printf("IVSH: %s++\n", __func__);
	if (socketiv_check_ivsock(sockfd))
	{
		printf("IVSH: %s++++\n", __func__);
		printf("IVSH: %s----\n", __func__);
		return socketiv_write(sockfd, buf, len);
	}
	printf("IVSH: %s--\n", __func__);
	return orig_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

int close2(int fd) { // close socket
	printf("IVSH: %s++\n", __func__);
	if (socketiv_check_ivsock(fd))
	{
		printf("IVSH: %s++++\n", __func__);
		socketiv_close(fd);
		printf("IVSH: %s----\n", __func__);
	}
	printf("IVSH: %s--\n", __func__);
	return orig_close(fd);
}

int shutdown2(int sockfd, int how) { // close socket
	printf("IVSH: %s++\n", __func__);
	if (socketiv_check_ivsock(sockfd))
	{
		printf("IVSH: %s++++\n", __func__);
		socketiv_close(sockfd);
		printf("IVSH: %s----\n", __func__);
	}
	printf("IVSH: %s--\n", __func__);
	return orig_shutdown(sockfd, how);
}
