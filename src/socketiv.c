#define _GNU_SOURCE
#include <dlfcn.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "socketiv.h"

typedef enum fd_type {
	FD_TYPE_INV = 0,
	FD_TYPE_GEN = 1,
	FD_TYPE_IVSOCK = 2
} FD_TYPE;
typedef struct ivsm
{
	void *cts_queue;
	size_t cts_queue_size;
	size_t cts_read_head;
	size_t cts_write_head;
	void *stc_queue;
	size_t stc_queue_size;
	size_t stc_read_head;
	size_t stc_write_head;
} IVSM;
typedef struct ivsock
{
	int enabled;

	// QoS 에 필요한 변수들
	size_t blk_size;
	size_t int_dly_time_slot;
	size_t int_strom_thresh;
	size_t poll_intvl;
	size_t poll_tmo;
	// END
	size_t blk_size;
	int recv_int_uio;   // file descriptor for receiving interrupt
	void *send_int_uio; // address for sending interrupt

	IVSM *ivsm_addr;
} IVSOCK;

IVSOCK *fd_to_ivsock_map;
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
	orig_write = (ssize_t (*)(int, void *, size_t))dlsym(RTLD_NEXT, "write");
	orig_close = (int (*)(int))dlsym(RTLD_NEXT, "close");
}
static inline int socketiv_register_fd(int fd, SOCKETIV_FD_IVSOCK_PAIR fd_ivsock_pair) { // register a fd_type entry to global list
	if (fd < 0 || (fd <))
		return EINVAL;
	if (fd >= fd_ivsock_map_tbl_size) {
		if (fd >= fd_ivsock_map_tbl_reserve) {
			fd_ivsock_map_tbl = realloc(fd_ivsock_map_tbl, sizeof(fd_ivsock_map_tbl) * (fd_ivsock_map_tbl_reserve * 2));
			memset(fd_ivsock_map_tbl + fd_ivsock_map_tbl_reserve, 0, fd_ivsock_map_tbl_reserve);
			fd_ivsock_map_tbl_reserve *= 2;
		}
		fd_ivsock_map_tbl_prev_size = fd_ivsock_map_tbl_size;
		fd_ivsock_map_tbl_size = fd + 1;
	}
	fd_ivsock_map_tbl[fd].fd_type = fd_type;
	fd_ivsock_map_tbl[fd].ivsock = ivsock;
	return 0;
}
}
static inline int unmap(int fd) { // register a fd_type entry of global list
	if (socketiv_verify_fd(fd)){

	}
	return EBADF;
	fd_ivsock_map_tbl[fd].fd_type = TYPE_INV;
	if ((fd + 1) == fd_list_size)
		if ((fd_list_prev_size <= fd_list_reserve / 2) && (fd_list_preserve > 4))
		{
			fd_list = realloc(fd_list, fd_list_reserve /= 2);
			fd_list_size = fd_list_prev_size;
		}
}

// 수정 필요
#define VIRT_NET_ADDR_SPACE "192.168.122."
int socketiv_check_vm_subnet(const struct sockaddr *addr) { // determine wheter this address belongs to a virtual network
	struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
	char *addr_str = inet_ntoa(addr_in->sin_addr);
	if (strstr(addr_str, VIRT_NET_ADDR_SPACE) == NULL)
		return 1;
	return 0;
}
int socketiv_accept(int new_sockfd) {
	return map_ivsock_to_fd(new_sockfd, socketiv_establish_ivsock());
}
int socketiv_connect(int sockfd) {
	return map_ivsock_to_fd(new_sockfd, socketiv_establish_ivsock());
}
// END

int socketiv_check_ivsock(int fd){
	if (fd >= fd_to_ivsock_map_size || fd < 0 || !fd_to_ivsock_map[fd].enabled)
		return 0;
	return 1;
}

int socketiv_close(int fd) { // close a SocketIV socket
	if (socketiv_remove_ivshmem(fd) || unmap_ivsock_from_fd(fd))
		return EBADF;
	return EXIT_SUCCESS;
}
