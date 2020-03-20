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

typedef volatile struct ivsm {
	int writer_end;
	int writer_ack;
	int reader_ack;
	bool poll_mode;
	void *cts_queue;
	size_t cts_queue_size;
	size_t cts_read_head;
	size_t cts_write_head;
	void *stc_queue;
	size_t stc_queue_size;
	size_t stc_read_head;
	size_t stc_write_head;
} IVSM;

#define TIMESTAMP_ENTRIES 20
#define STORM_RATE_MS 50
#define POLL_US 500
typedef volatile struct ivsock {
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

	IVSM *ivsm_addr;
} IVSOCK;

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
	orig_close = (int (*)(int))dlsym(RTLD_NEXT, "close");
}

static inline int attach_new_ivsock_to_fd(int fd) {
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
	ivsock->ivsm_addr = plain_mmap; //(void*)PHYS_ADDR;
	memset(plain_mmap, 0, sizeof(IVSOCK));

	return 0;
}
static inline int detach_ivsock_from_fd(int fd) {
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
	struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
	char *addr_str = inet_ntoa(addr_in->sin_addr);

	bool ret = !strncmp(addr_str, VIRT_NET_ADDR_SPACE, strlen(VIRT_NET_ADDR_SPACE));
	if (ret) puts("YES"); else puts("NO"); return ret;
}
int socketiv_accept(int new_sockfd) {
	return attach_new_ivsock_to_fd(new_sockfd);
}
int socketiv_connect(int sockfd) {
	return attach_new_ivsock_to_fd(sockfd);
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
