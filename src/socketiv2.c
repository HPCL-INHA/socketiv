#ifdef SOCKETIV_IN_HOST
//static inline int socketiv_create_ivshmem(int sockfd, 무슨 인자?){
// 호스트에서 동작하는 라이브러리 - ivshmem 생성}
//static inline int socketiv_remove_ivshmem(int sockfd, 무슨 인자?){
// 호스트에서 동작하는 라이브러리 - ivshmem 소거}
#else
static inline int socketiv_create_ivshmem(int sockfd) {
	// VM에서 동작하는 라이브러리 - ivshmem 생성
	// TODO
	return 0;
}

static inline int socketiv_remove_ivshmem(int sockfd) {
	// VM에서 동작하는 라이브러리 - ivshmem 소거
	// TODO
	return 0;
}
#endif

#define barrier() __asm__ __volatile__("": : :"memory")
#define OFFSET (1024 * 1024)

ssize_t socketiv_read(int fd, void *buf, size_t count) {
	IVSOCK *ivsock = fd_to_ivsock_map[fd];
	IVSM *ivsm = ivsock->ivsm_addr;

	// poll
	while (ivsm->wptr - ivsm->rptr < count) {
		usleep(SLEEP); // 시간 얼마? or clock_nanosleep()?
	};
	
	memcpy(buf, (void*)ivsm + OFFSET + ivsm->rptr, count);
	ivsm->rptr += count;

	printf("IVSH: READ\n");

	return count;
}

static inline int64_t getmstime(void) {
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);

	return (tp.tv_sec * 1000) + (tp.tv_nsec / 1000 / 1000);
}

ssize_t socketiv_write(int fd, const void *buf, size_t count) {
	IVSOCK *ivsock = fd_to_ivsock_map[fd];
	IVSM *ivsm = ivsock->ivsm_addr;

	memcpy((void*)ivsm + OFFSET + ivsm->wptr, buf, count);
	ivsm->wptr += count;

	printf("IVSH: WRITE\n");

	return count;
}

