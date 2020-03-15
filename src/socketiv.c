#define _GNU_SOURCE
#include <dlfcn.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include "socketiv.h"

typedef struct ivsm {
	void *cts_queue;
	size_t cts_queue_size;
	size_t cts_read_head;
	size_t cts_write_head;
	void *stc_queue;
	size_t stc_queue_size;
	size_t stc_read_head;
	size_t stc_write_head;
} IVSM;
typedef struct ivsock {
	int enabled;

	// QoS 에 필요한 변수들
	size_t blk_size;
	size_t int_dly_time_slot;
	size_t int_strom_thresh;
	size_t poll_intvl;
	size_t poll_tmo;
	// END

	int recv_int_uio;   // file descriptor for receiving interrupt
	void *send_int_uio; // address for sending interrupt

	IVSM *ivsm_addr;
} IVSOCK;

IVSOCK **fd_to_ivsock_map;
size_t fd_to_ivsock_map_reserve;
int fd_to_ivsock_map_prev_size;
int fd_to_ivsock_map_size;

void __attribute__((constructor)) socketiv_init() { // initialize SocketIV
	fd_to_ivsock_map = calloc(4, sizeof(fd_to_ivsock_map));
	fd_to_ivsock_map_reserve = 4;
	fd_to_ivsock_map_prev_size = 2;
	fd_to_ivsock_map_size = 3;

	orig_open = (int (*)(const char *, int, mode_t))dlsym(RTLD_NEXT, "open");
	orig_socket = (int (*)(int, int, int))dlsym(RTLD_NEXT, "socket");
	orig_accept = (int (*)(int, struct sockaddr *, socklen_t *))dlsym(RTLD_NEXT, "accept");
	orig_connect = (int (*)(int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "connect");
	orig_read = (ssize_t (*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
	orig_write = (ssize_t (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
	orig_close = (int (*)(int))dlsym(RTLD_NEXT, "close");
}
static inline int attach_new_ivsock_to_fd(int fd) {
	IVSOCK *ivsock = calloc(1, sizeof(ivsock));
	
	if (fd >= fd_to_ivsock_map_size) {
		if (fd >= fd_to_ivsock_map_reserve) {
			fd_to_ivsock_map = realloc(fd_to_ivsock_map, sizeof(fd_to_ivsock_map) * (fd_to_ivsock_map_reserve * 2));
			memset(fd_to_ivsock_map + fd_to_ivsock_map_reserve, 0, fd_to_ivsock_map_reserve);
			fd_to_ivsock_map_reserve *= 2;
		}
		fd_to_ivsock_map_prev_size = fd_to_ivsock_map_size;
		fd_to_ivsock_map_size = fd + 1;
	}

	// fd number 교환


	// (장기적 수정 필요) construct an IVSOCK structure and IVSM device
	fd_to_ivsock_map[fd] = ivsock;

	return 0;
}
static inline int detach_ivsock_from_fd(int fd) {
	if ((fd + 1) == fd_to_ivsock_map_size)
		if ((fd_to_ivsock_map_prev_size <= fd_to_ivsock_map_reserve / 2) && (fd_to_ivsock_map_reserve > 4)) {
			fd_to_ivsock_map = realloc(fd_to_ivsock_map, fd_to_ivsock_map_reserve /= 2);
			fd_to_ivsock_map_size = fd_to_ivsock_map_prev_size;
		}

	// fd number 교환


	// (장기적 수정 필요) destruct an IVSOCK structure and IVSM device
	free(fd_to_ivsock_map[fd]);
	fd_to_ivsock_map[fd] = NULL;

	return 0;
}

#define VIRT_NET_ADDR_SPACE "192.168.122"
int socketiv_check_vm_subnet(const struct sockaddr *addr) { // (장기적 수정 필요) determine whether this address belongs to a virtual network
	struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
	char *addr_str = inet_ntoa(addr_in->sin_addr);
	if (strstr(addr_str, VIRT_NET_ADDR_SPACE) == NULL)
		return 1;
	return 0;
}
int socketiv_accept(int new_sockfd) {
	return attach_new_ivsock_to_fd(new_sockfd);
}
int socketiv_connect(int sockfd) {
	return attach_new_ivsock_to_fd(sockfd);
}

int socketiv_check_ivsock(int fd) { // determine whether this file descriptor has been paired with an inter-vm socket
	if (fd_to_ivsock_map[fd])
		return 1;
	return 0;
}
ssize_t socketiv_read(int sockfd, void *buf, size_t count) {
	// 채우세요
}
ssize_t socketiv_write(int sockfd, const void *buf, size_t count) {
	// 채우세요
}

int socketiv_close(int sockfd) { // close an inter-vm socket
	return detach_ivsock_from_fd(sockfd);
}
