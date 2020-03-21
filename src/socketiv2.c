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

ssize_t socketiv_read(int fd, void *buf, size_t count) {
	IVSOCK *ivsock = fd_to_ivsock_map[fd];
	IVSM *ivsm = ivsock->ivsm_addr;

	do {
		intr_wait();
		if (ivsm->writer_end) {
			ivsm->writer_end = 0;
			break;
		}
	} while (true);
	ivsm->reader_ack = 1;
	
	memcpy(buf, (void*)ivsm + sizeof(IVSM), count);
	ivsm->reader_end = 1;

	// temporal solution - intr_send() 가 성공할지 안할지 몰라서...
	do {
		intr_send(1);
		usleep(1); // interrupt retry - 얼마정도 쉬어야 할까 or clock_nanosleep()
	} while (!ivsm->writer_ack);
	ivsm->writer_ack = 0;

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

	memcpy((void*)ivsm + sizeof(IVSM), buf, count);
	ivsm->writer_end = 1;

	// temporal solution - intr_send() 가 성공할지 안할지 몰라서...
	do {
		intr_send(0);
		usleep(1); // interrupt retry - 얼마정도 쉬어야 할까 or clock_nanosleep()?
	} while (!ivsm->reader_ack);
	ivsm->reader_ack = 0;

	do {
		intr_wait();
		if (ivsm->reader_end) {
			ivsm->reader_end = 0;
			break;
		}
	} while (true);
	ivsm->writer_ack = 1;

	return count;
}

