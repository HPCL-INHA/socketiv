#define _GNU_SOURCE
#include <dlfcn.h>

#include <stdlib.h>

#include "socketiv.h"

typedef struct fd_ivsock_map
{
	SOCKETIV_FD_TYPE fd_type;
	IVSOCK *ivsock;
} FD_IVSOCK_MAP;
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
	// QoS 에 필요한 변수들

	// END

	int recv_int_uio;   // file descriptor for receiving interrupt
	void *send_int_uio; // address for sending interrupt

	IVSM *ivsm_addr;
} IVSOCK;

static FD_IVSOCK_MAP *fd_ivsock_map_tbl;
static size_t fd_ivsock_map_tbl_reserve;
static int fd_ivsock_map_tbl_prev_size;
static int fd_ivsock_map_tbl_size;

void __attribute__((constructor)) socketiv_init() { // initialize SocketIV
	fd_ivsock_map_tbl = calloc(4, sizeof(fd_ivsock_map_tbl));
	fd_ivsock_map_tbl_reserve = 4;
	fd_ivsock_map_tbl_prev_size = 2;
	fd_ivsock_map_tbl_size = 3;

	orig_open = (int (*)(const char *, int, mode_t))dlsym(RTLD_NEXT, "open");
	orig_socket = (int (*)(int, int, int))dlsym(RTLD_NEXT, "socket");
	orig_accept = (int (*)(int, struct sockaddr *, socklen_t *))dlsym(RTLD_NEXT, "accept");
	orig_connect = (int (*)(int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "connect");
	orig_read = (ssize_t (*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
	orig_write = (ssize_t (*)(int, void *, size_t))dlsym(RTLD_NEXT, "write");
	orig_close = (int (*)(int))dlsym(RTLD_NEXT, "close");
}

static inline int socketiv_alter_fd(int fd, SOCKETIV_FD_TYPE fd_type, IVSOCK *ivsock) { // modify a fd_type entry of global list
	if (fd >= fd_ivsock_map_tbl_reserve || fd < 0)
		return EINVAL;
	fd_ivsock_map_tbl[fd].fd_type = fd_type;
	fd_ivsock_map_tbl[fd].ivsock = ivsock;
	return 0;
}
void socketiv_register_generic_fd(int fd) { // register a fd_type entry to global list
	if (fd < 0 )
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
	socketiv_alter_fd(fd, SOCKETIV_FD_TYPE_GEN, NULL);
}
SOCKETIV_FD_TYPE socketiv_get_fd_type(int fd) { // check type of a fd_type entry in global list
	return fd_ivsock_map_tbl[fd].fd_type;
}

void socketiv_unregister_fd(int fd) { // register a fd_type entry of global list
	socketiv_alter_fd(fd, SOCKETIV_FD_TYPE_INV, NULL);
	if (fd < 0 || fd >= fd_ivsock_map_tbl_size)
		return EINVAL;
	if ((fd + 1) == fd_list_size)
		if ((fd_list_prev_size <= fd_list_reserve / 2) && (fd_list_preserve > 4))
		{
			fd_list = realloc(fd_list, fd_list_reserve /= 2);
			fd_list_size = fd_list_prev_size;
		}
}

int socketiv_close(int fd)
{ // close a SocketIV socket
	if (socketiv_remove_ivshmem(fd) || socketiv_alter_fd(fd, SOCKETIV_FD_TYPE_GEN))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
