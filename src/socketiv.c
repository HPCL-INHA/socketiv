#define _GNU_SOURCE
#include <dlfcn.h>

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <time.h>

#include "socketiv.h"

#include "intr.c"

#define ALIGN 64

struct ivsm {
	size_t rptr;
	size_t wptr;
	int fulled;
	int enabled;
} __attribute__((aligned(ALIGN), packed));

typedef struct ivsm IVSM;

#define TIMESTAMP_ENTRIES 20
#define STORM_RATE_MS 50
#define POLL_US 500
struct ivsock {
	int enabled;

	// QoS 에 필요한 변수들
	size_t blk_size;
	size_t int_dly_time_slot;
	size_t int_strom_thresh;
	size_t poll_intvl;
	size_t poll_tmo;
	int64_t timestamp[TIMESTAMP_ENTRIES];
	int timestamp_index;
	// END

	int recv_int_uio;   // file descriptor for receiving interrupt
	void *send_int_uio; // address for sending interrupt

	IVSM *ivsm_addr_read;
	IVSM *ivsm_addr_write;
} __attribute__((aligned(ALIGN), packed));

typedef struct ivsock IVSOCK;

IVSOCK **fd_to_ivsock_map;
size_t fd_to_ivsock_map_reserve;
int fd_to_ivsock_map_prev_size;
int fd_to_ivsock_map_size;

static void __attribute__((constructor)) socketiv_init() { // initialize SocketIV
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
	orig_recv = (ssize_t (*)(int, void *, size_t, int))dlsym(RTLD_NEXT, "recv");
	orig_recvfrom = (ssize_t (*)(int, void *, size_t, int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "recvfrom");
	orig_send = (ssize_t (*)(int, const void *, size_t, int))dlsym(RTLD_NEXT, "send");
	orig_sendto = (ssize_t (*)(int, const void *, size_t, int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "sendto");
	orig_close = (int (*)(int))dlsym(RTLD_NEXT, "close");
	orig_shutdown = (int (*)(int, int))dlsym(RTLD_NEXT, "shutdown");
}

int attach_new_ivsock_to_fd(int fd) {
	if (fd >= fd_to_ivsock_map_size) {
		if (fd >= fd_to_ivsock_map_reserve) {
			fd_to_ivsock_map = realloc(fd_to_ivsock_map, sizeof(fd_to_ivsock_map) * (fd_to_ivsock_map_reserve * 2));
			memset(fd_to_ivsock_map + fd_to_ivsock_map_reserve, 0, fd_to_ivsock_map_reserve);
			fd_to_ivsock_map_reserve *= 2;
		}
		fd_to_ivsock_map_prev_size = fd_to_ivsock_map_size;
		fd_to_ivsock_map_size = fd + 1;
	}
	else if (fd >= fd_to_ivsock_map_prev_size)
		fd_to_ivsock_map_prev_size = fd + 1;

	// fd number 교환

	// (지금 삽입 필요) construct an IVSOCK structure and IVSM device
	// 여기에
	IVSOCK *ivsock = calloc(1, sizeof(IVSOCK));
	fd_to_ivsock_map[fd] = ivsock;

	intr_init();

	ivsock->blk_size = 256 * 1024;
#ifdef CLIENT
	ivsock->ivsm_addr_read = plain_mmap_read; //(void*)PHYS_ADDR;
	ivsock->ivsm_addr_write = plain_mmap_write; //(void*)PHYS_ADDR;
#else
	ivsock->ivsm_addr_read = plain_mmap_write; //(void*)PHYS_ADDR;
	ivsock->ivsm_addr_write = plain_mmap_read; //(void*)PHYS_ADDR;
#endif
	memset(ivsock->ivsm_addr_write, 0, sizeof(IVSOCK));

	// 자기가 write하는 파트의 enabled 비트 끄기
	fd_to_ivsock_map[fd]->ivsm_addr_write->enabled = 1;
	fd_to_ivsock_map[fd]->enabled = 1;


	return 0;
}
static inline int detach_ivsock_from_fd(int fd) {
	// 모든 파트의 enabled 비트 끄기
	fd_to_ivsock_map[fd]->enabled = 0;
	fd_to_ivsock_map[fd]->ivsm_addr_write->enabled = 0;
	fd_to_ivsock_map[fd]->ivsm_addr_read->enabled = 0;

	free(fd_to_ivsock_map[fd]);
	fd_to_ivsock_map[fd] = NULL;

	return 0; // NOOP ATM

	if ((fd + 1) == fd_to_ivsock_map_size)
		if ((fd_to_ivsock_map_prev_size <= fd_to_ivsock_map_reserve / 2) && (fd_to_ivsock_map_reserve > 4)) {
			fd_to_ivsock_map = realloc(fd_to_ivsock_map, fd_to_ivsock_map_reserve /= 2);
			fd_to_ivsock_map_size = fd_to_ivsock_map_prev_size;
		}

	// fd number 교환


	// (지금 삽입 필요) destruct an IVSOCK structure and IVSM device
	// 여기에
	free(fd_to_ivsock_map[fd]);
	fd_to_ivsock_map[fd] = NULL;

	return 0;
}

#define VIRT_NET_ADDR_SPACE "192.168.122"
bool socketiv_check_vm_subnet(const struct sockaddr *addr) { // (장기적 수정 필요) determine whether this address belongs to a virtual network
return true;
	struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
	char *addr_str = inet_ntoa(addr_in->sin_addr);

	printf("DEBUG: addr_str: %s\n", addr_str);
	bool ret = !strncmp(addr_str, VIRT_NET_ADDR_SPACE, strlen(VIRT_NET_ADDR_SPACE));
	if (ret) puts("YES"); else puts("NO"); return ret;
}
int socketiv_accept(int new_sockfd) {
//	return attach_new_ivsock_to_fd(new_sockfd);
}
int socketiv_connect(int sockfd) {
//	return attach_new_ivsock_to_fd(sockfd);
}
int socketiv_socket(int fd) {
//	return attach_new_ivsock_to_fd(fd);
}
bool socketiv_check_ivsock(int fd) { // determine whether this file descriptor has been paired with an inter-vm socket
	if (fd < fd_to_ivsock_map_size && fd_to_ivsock_map[fd])
		return true;
	return false;
}

/*
ssize_t socketiv_read(int sockfd, void *buf, size_t count) {
	// 채우세요
}
ssize_t socketiv_write(int sockfd, const void *buf, size_t count) {
	// 채우세요
}
*/

int socketiv_close(int sockfd) { // close an inter-vm socket
	return detach_ivsock_from_fd(sockfd);
}

#include "socketiv2.c"
