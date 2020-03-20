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

ssize_t socketiv_read(int fd, void *buf, size_t count)
{
	IVSOCK *ivsock = fd_to_ivsock_map[fd];
	IVSM *ivsm = ivsock->ivsm_addr;
	size_t rlen, wlen;

	printf("%p\n", ivsm);

	// 인터럽트 모드 일 때: 인터럽트가 올 때 까지 wait -> 인터럽트가 발생하면 available 한 블록들 모두 read copy
	if (!ivsm->poll_mode) {
		printf("INTR MODE\n");
		// TODO: Calculate interrupt storm rate
		// TODO: EOF & Loop handling??
		rlen = 0;
		while (rlen != count) {
			wlen = ivsm->stc_write_head;
			if (wlen > count)
				wlen = count;
			memcpy(buf + wlen - rlen, (void*)ivsm + sizeof(IVSM) + wlen - rlen, wlen);
			rlen = wlen;
			ivsm->stc_read_head = rlen;
			intr_wait();
		}
	} else {
		printf("POLL MODE\n");
		rlen = 0;
		while (rlen != count) {
			wlen = ivsm->stc_write_head;
			if (wlen > count)
				wlen = count;
			memcpy(buf + wlen - rlen, (void*)ivsm + sizeof(IVSM) + wlen - rlen, wlen);
			rlen = wlen;
			ivsm->stc_read_head = rlen;
			if (rlen != count)
				usleep(POLL_US);
		}
	}

	return count;

	// 스루풋이 더욱 증가하여 threshold 를 초과할 시: IVSOCK 내의 기술된 폴링 모드(= 폴링 interval)를 다음 단계로 증가하여 interval 단축
	// 스루풋이 하향: 이전 interval로 단축
	// 일정 time window(burst timeout mode) 동안 전송이 발생하지 않음: 다시 인터럽트 모드로 전환
	// 큰 쓰기가 발생한 경우: 더욱 큰 block size로 전환 - 이후 충분히 큰 쓰기가 이뤄지지 않으면 block size 축소
	// TODO: ^ ???
}

static inline int64_t getmstime(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);

	return (tp.tv_sec * 1000) + (tp.tv_nsec / 1000 / 1000);
}

#define barrier() __asm__ __volatile__("": : :"memory")

ssize_t socketiv_write(int fd, const void *buf, size_t count)
{
	IVSOCK *ivsock = fd_to_ivsock_map[fd];
	IVSM *ivsm = ivsock->ivsm_addr;
	int temp;

	assert(ivsm->reader_ack == 0);

	memcpy((void*)ivsm + sizeof(IVSM), buf, count);
	ivsm->writer_end = 1;
	do {
		intr_send(0);
	} while (!ivsm->reader_ack);
	intr_wait();
	ivsm->writer_end = 0;
	ivsm->writer_ack = 1;
	ivsm->reader_ack = 0;

	return count;
}

