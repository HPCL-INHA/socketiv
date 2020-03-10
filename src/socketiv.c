#include "socketiv.h"

/* 값 조정 필요 */
typedef enum queue_size_mode {	// byte
	QSM_1 = 262144		// 256KB
	QSM_2 = 2097152	// 2MB
} QUEUE_SIZ_MODE;
typedef enum block_size_mode {	// byte
	BSM_2 = 4096		// 4KB
	BSM_1 = 16384	// 16KB
} BLK_SIZ_MODE;
typedef enum interrupt_delay_mode {	// ns
	IDM_1 = 0		// 0ns
	IDM_2 = 1000	// 1us
} INT_DELAY_MODE;
typedef enum interrupt_threshold_mode {	// count/s
	ITM_1 = 10		// 10/s
	ITM_2 = 20		// 20/s
} INT_THR_MODE;
typedef enum polling_interval_mode {	// ns
	PIM_1 = 1000,		// 1us
	PIM_2 = 100000		// 100us
} POLL_INTVL_MODE;
typedef enum speed_threshold_mode {	// MB/s
	STM_1 = 1024		// 1GB/s
	STM_2 = 10240	// 10GB/s
} SPD_THR_LVL;
typedef enum burst_timeout_mode {	// us, 얘는 조정 메커니즘 필요없어 보임
	BTM_1 = 1000		// 1ms
} BURST_TMO_MODE;
/* END */

typedef struct inter_vm_socket {
	QUEUE_SIZ_MODE queue_size_mode;
	BLK_SIZ_MODE block_size_mode;
	INT_DELAY_MODE interrupt_delay_mode;
	INT_THR_MODE interrupt_threshold_mode;
	POLL_INTVL_MODE polling_interval_mode;
	SPD_THR_LVL speed_threshold_mode;
	BURST_TMO_MODE burst_timeout_mode;

	IVSM *ivsm_ptr;
} IVSOCK;
typedef struct inter_vm_shmem {
	Q_SIZ_MODE host_size;
	void *host_base;
	void *host_rptr;
	void *host_wptr;
	Q_SIZ_MODE remote_size;
	void *remote_base;
	void *remote_rptr;
	void *remote_wptr;
	void *send_interrupt_uio;	// memory I/O for sending interrupt
	int recv_interrupt_uio;	// file I/O for receiving interrupt
} IVSM;

static SOCKETIV_FD_TYPE *fd_list;
static size_t fd_list_reserve;
static int fd_list_prev_size;
static int fd_list_size;

void __attribute__ ((constructor)) socketiv_init()
{				// initialize SocketIV
	fd_list = malloc(4 * sizeof(SOCKETIV_FD_TYPE));
	fd_list_reserve = 4;
	fd_list_prev_size = 2;
	fd_list_size = 3;

	orig_open =
	    (int (*)(const char *, int, mode_t))dlsym(RTLD_NEXT, "open");
	orig_socket = (int (*)(int, int, int))dlsym(RTLD_NEXT, "socket");
	orig_accept =
	    (int (*)(int, struct sockaddr *, socklen_t *))dlsym(RTLD_NEXT,
								"accept");
	orig_connect =
	    (int (*)(int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT,
								    "connect");
	orig_read = (ssize_t(*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
	orig_write = (ssize_t(*)(int, void *, size_t))dlsym(RTLD_NEXT, "write");
	orig_close = (int (*)(int))dlsym(RTLD_NEXT, "close");
}

static inline int socketiv_alter_fd(int fd, SOCKETIV_FD_TYPE fd_type)
{				// modify a fd_type entry of global list 
	if (fd >= fd_list_reserve)
		return EXIT_FAILURE;
	fd_list[fd] = fd_type;
	return EXIT_SUCCESS;
}

void socketiv_register_fd(int fd, SOCKETIV_FD_TYPE fd_type)
{				// register a fd_type entry to global list
	if (fd >= fd_list_reserve) {
		fd_list =
		    realloc(fd_list,
			    sizeof(SOCKETIV_FD_TYPE) * (fd_list_reserve * 2));
		memset(fd_list + fd_list_reserve, SOCKETIV_FD_TYPE_INV,
		       fd_list_reserve);
		fd_list_reserve *= 2;
	}
	if (fd >= fd_list_size) {
		fd_list_prev_size = fd_list_size;
		fd_list_size = fd + 1;
	}
	socketiv_alter_fd(fd, fd_type);
}

SOCKETIV_FD_TYPE socketiv_check_fd(int fd)
{				// check type of a fd_type entry in global list
	return fd_list[fd];
}

void socketiv_unregister_fd(int fd)
{				// register a fd_type entry of global list
	socketiv_alter_fd(fd, SOCKETIV_FD_TYPE_INV);
	if ((fd + 1) == fd_list_size)
		if ((fd_list_prev_size <= fd_list_reserve / 2)
		    && (fd_list_preserve > 4)) {
			fd_list = realloc(fd_list, fd_list_reserve /= 2);
			fd_list_size = fd_list_prev_size;
		}
}

#ifdef SOCKETIV_IN_HOST
//static inline int socketiv_create_ivshmem(int sockfd, 무슨 인자?){
	// 호스트에서 동작하는 라이브러리 - ivshmem 생성}
//static inline int socketiv_remove_ivshmem(int sockfd, 무슨 인자?){
	// 호스트에서 동작하는 라이브러리 - ivshmem 소거}
#else
static inline int socketiv_create_ivshmem(int sockfd)
{
	// VM에서 동작하는 라이브러리 - ivshmem 생성
	// TODO
	return 0;
}

static inline int socketiv_remove_ivshmem(int sockfd)
{
	// VM에서 동작하는 라이브러리 - ivshmem 소거
	// TODO
	return 0;
}
#endif

int socketiv_accept(int sockfd)
{
	// 클라이언트와 정보를 맞춰 socketiv_create_ivshmem()
	// TODO
}

int socketiv_connect(int sockfd)
{
	// 서버와 정보를 맞춰 socketiv_create_ivshmem()
	// TODO
}

// determine whether this address belongs to virtual network
bool socketiv_check_vm_subnet(const struct sockaddr *addr)
{
	#define IP_MAXLEN 64
	char buf[IP_MAXLEN];

	if (addr->sa_family != AF_INET)
		return false; // non-IPv4

	inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr),
		  buf, IP_MAXLEN);

	return !strncmp(buf, VM_ADDR, strlen(VM_ADDR));
}

#if 0
WTF?
int socketiv_accept(int new_sockfd)
{
	//if(socketiv_create_ivshmem(sockfd, 무슨 인자?) || socketiv_alter_fd(fd, SOCKETIV_FD_TYPE_IVSOCK))
	//      return EXIT_FAILURE;
	//return EXIT_SUCCESS;
}

int socketiv_connect(int sockfd)
{
	//if(socketiv_create_ivshmem(sockfd, 무슨 인자?) || socketiv_alter_fd(fd, SOCKETIV_FD_TYPE_IVSOCK))
	//      return EXIT_FAILURE;
	//return EXIT_SUCCESS;
}
#endif

ssize_t socketiv_read(int fd, void *buf, size_t count)
{
	ssize_t len;

	// 인터럽트 모드 일 때: 인터럽트가 올 때 까지 wait -> 인터럽트가 발생하면 available 한 블록들 모두 read copy
	if (mode == interrupt) {
		// TODO: Calculate interrupt storm rate
		intr_wait();
		len = *(ssize_t*)plain_mmap;
		memcpy(buf, plain_mmap + 256, len); // 256: HARDCODING ATM
		return len;
	}

	// 인터럽트가 과도할 때(즉 IVSOCK 내의 인터럽트 한계(count/s)를 넘긴 경우): IVSOCK 내의 기술된 폴링 모드로 전환
	else {
		// TODO: Calculate polling storm rate and adjust usleep value
		// usleep(value);
		// TODO: if (last request was a while ago) mode = interrupt;
		len = *(ssize_t*)plain_mmap;
		memcpy(buf, plain_mmap + 256, len); // 256: HARDCODING ATM
		return len;
	}

	// 스루풋이 더욱 증가하여 threshold 를 초과할 시: IVSOCK 내의 기술된 폴링 모드(= 폴링 interval)를 다음 단계로 증가하여 interval 단축
	// 스루풋이 하향: 이전 interval로 단축
	// 일정 time window(burst timeout mode) 동안 전송이 발생하지 않음: 다시 인터럽트 모드로 전환
	// 큰 쓰기가 발생한 경우: 더욱 큰 block size로 전환 - 이후 충분히 큰 쓰기가 이뤄지지 않으면 block size 축소
	// TODO: ^ ???
}

ssize_t socketiv_write(int fd, const void *buf, size_t count)
{
	// 인터럽트 모드 일 때: 쓰는 동안 블록 사이즈 별로 나눠 인터럽트
	if (mode == interrupt) {
		ssize_t len;
		for (len = count; len > blk_size; len -= blk_size) {
			*(ssize_t*)plain_mmap = blk_size;
			memcpy(plain_mmap + 256, buf, blk_size);
			intr_send();
		}
		if (len) {
			// Send remaining data
			// TODO: 블록 사이즈 보다 작은 write의 경우: 지정된 delay time window 만큼 대기후에 인터럽트(이때는 메인 함수내에서 block 하지 않음)
			*(ssize_t*)plain_mmap = len;
			memcpy(plain_mmap + 256, buf, len);
			intr_send();
		}
	}
	// - 인터럽트 delay 중(즉 non-block으로서 메인함수로 돌아간 경우), socket에 read() 할 시 delay time window 축소 - 이런 경우가 발생할 때 마다 단계적으로 축소(반대의 경우는 단계적으로 회복)

	{
	// 인터럽트가 과도할 때: reader를 polling mode로 전환 시킴
	// 스루풋이 증가할 시: reader에게 더 짧은 polling interval mode로 전환하라 명령
	// 스루풋이 하향: reader에게 더 긴 polling interval mode로 전환하라 명령
	} copy from above

	// 일정 time window 동안 전송이 발생하지 않음: reader를 인터럽트 모드로 wait, 다시 인터럽트 모드로 전환
	// 큐를 모두 채울때: 단계적으로 큐를 키움 - 이후, 충분히 쓰지 않으면 단계적으로 큐 축소
	// 큰 쓰기가 발생한 경우: 더욱 큰 block size로 전환 - 이후 충분히 큰 쓰기가 이뤄지지 않으면 block size 축소
}

int socketiv_close(int fd)
{				// close a SocketIV socket
	if (socketiv_remove_ivshmem(fd)
	    || socketiv_alter_fd(fd, SOCKETIV_FD_TYPE_GEN))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
