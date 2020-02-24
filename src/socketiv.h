#ifndef SOCKETIV_H
#define SOCKETIV_H

#include <stdint.h>
#include <stdlib.h>

#define VM_ADDR "192.168.122."

typedef enum queue_size_mode // byte
{
    QSM_1 = 2097152 // 2MB
} Q_SIZ_MODE;
typedef enum block_size_mode // byte
{
    BSM_1 = 262144 // 256KB
} BLK_SIZ_MODE;
typedef enum interrupt_mode // count
{
    IM_1 = 10
} INT_MODE;
typedef enum polling_mode // ns
{
    PM_1 = 1000,  // 1us
    PM_2 = 100000 // 100us
} POLL_MODE;
typedef enum throughput_level // KB/s
{
    TL_1 = 1048576 // 1GB/s
} T_LVL;
typedef enum burst_timeout_mode // ns
{
    BTM_1 = 1000000 // 1ms
} BURST_TMO_MODE;

typedef struct inter_vm_socket
{
    BLK_SIZ_MODE block_size_mode;
    INT_MODE interrupt_mode;
    POLL_MODE polling_mode;
    T_LVL throughput_level;
    BURST_TMO_MODE burst_timeout_mode;

    IVSM *ivsm_ptr;
} IVSOCK;
typedef struct inter_vm_shmem
{
    Q_SIZ_MODE host_size;
    void *host_base;
    void *host_rptr;
    void *host_wptr;
    void *host_interrupt_uio;
    Q_SIZ_MODE remote_size;
    void *remote_base;
    void *remote_rptr;
    void *remote_wptr;
    int remote_interrupt_uio;
} IVSM;

typedef enum fd_type
{
    TYPE_GENERIC = 0,
    TYPE_IVSOCK = 1
} FD_TYPE;

typedef struct fd_info
{
    FD_TYPE fd_type;
    IVSOCK *ivsock_ptr;
} FD_INFO;

typedef struct socketiv {
	enum mem_loc {
		INVALID = -1,
		LOCAL,
		REMOTE
	} loc;			// if > 0, memory is allocated remotely
	// else if == 0, memory is allocated locally
	// else if < 0, invalid
	size_t size;		// size of shared memory
	size_t len;		// ?
	void *virt_addr;	// virtual address mapped to physical address of shared memory
	int fd;			// file descriptor for later unlink()
} SOCKETIV;

int (*orig_open)(const char *, int, mode_t);
int (*orig_socket)(int, int, int);
int (*orig_accept)(int, struct sockaddr *, socklen_t *);
int (*orig_connect)(int, const struct sockaddr *, socklen_t);
int (*orig_close)(int);
ssize_t (*orig_read)(int, void *, size_t);
ssize_t (*orig_write)(int, void *, size_t);

int socketiv_register_fd(int fd, FD_TYPE fd_type, const IVSOCK *ivsock_ptr);
FD_TYPE socketiv_check_fd(int fd);
int socketiv_unregister_fd(int fd);

bool socketiv_check_vm_subnet(const struct sockaddr *addr);
int socketiv_accept(int sockfd);
int socketiv_connect(int sockfd);

ssize_t socketiv_read(int sockfd, void *buf, size_t count);
ssize_t socketiv_write(int sockfd, void *buf, size_t count);

int socketiv_close(int fd);

#endif
