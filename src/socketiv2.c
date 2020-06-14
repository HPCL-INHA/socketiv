#include "socketiv.h"

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

ssize_t socketiv_read(int fd, void *buf, size_t count) {
	IVSOCK *ivsock = fd_to_ivsock_map[fd];
	size_t to_read = 0; // if 문 안에서 처리할 바이트
	size_t remain_cnt = count, processed_byte = 0;
	IVSM *ivsm = ivsock->ivsm_addr_read;

	while (remain_cnt) {
		// Poll
		while ((ivsm->rptr == ivsm->wptr) && !ivsm->fulled)
			usleep(SLEEP); // 시간 얼마? or clock_nanosleep()?

		// Partial-Read Until Endpoint
		if ((ivsm->rptr > ivsm->wptr) || ivsm->fulled){
			printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", ivsm->wptr, ivsm->rptr, ivsm->fulled, remain_cnt);
			puts("(partial-read until endpoint)");
			to_read = ENDPOINT - ivsm->rptr;
			printf("to_read: %lu", to_read);

			memcpy(buf + processed_byte, (void *)ivsm + OFFSET + ivsm->rptr, to_read);
			remain_cnt -= to_read;
			processed_byte += to_read;

			ivsm->fulled = 0;
			
			ivsm->rptr = 0;

			continue;
		}

		// Partial-Read Until Write Pointer
		if (ivsm->rptr + remain_cnt > ivsm->wptr) {
			printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", ivsm->wptr, ivsm->rptr, ivsm->fulled, remain_cnt);
			puts("(partial-read until write pointer)");
			to_read = ivsm->wptr - ivsm->rptr;
			printf("to_read: %lu", to_read);

			memcpy(buf + processed_byte, (void *)ivsm + OFFSET + ivsm->rptr, to_read);
			remain_cnt -= to_read;
			processed_byte += to_read;

			ivsm->fulled = 0;

			// 포인터가 엔드 포인트에 도달하면 0으로 변경
			if (ivsm->rptr + to_read == ENDPOINT)
				ivsm->rptr = 0;
			else
				ivsm->rptr += to_read;

			continue;
		}

		// 마지막 읽기
		printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", ivsm->wptr, ivsm->rptr, ivsm->fulled, remain_cnt);
		puts("(final read)");
		printf("processed_byte: %lu\n", processed_byte);
		printf("ivsm->rptr: %lu\n", ivsm->rptr);
		printf("processed_byte: %lu\n", processed_byte);
		memcpy(buf + processed_byte, (void *)ivsm + OFFSET + ivsm->rptr, remain_cnt);
		remain_cnt = 0;
		ivsm->fulled = 0;
		puts("final read end");
		printf("ivsm->fulled: %d\n", ivsm->fulled);

		// 포인터가 엔드 포인트에 도달하면 0으로 변경
		if (ivsm->rptr + remain_cnt == ENDPOINT) {
			puts("(rptr reached endpoint)");
			ivsm->rptr = 0;
		} else {
			ivsm->rptr += remain_cnt;
		}
		printf("ivsm->rtpr: %lu\n", ivsm->rptr);
	}

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
	size_t to_write = 0; // if 문 안에서 처리할 바이트
	size_t remain_cnt = count, processed_byte = 0;
	IVSM *ivsm = ivsock->ivsm_addr_write;

	while (remain_cnt) {
		// Poll
		while ((ivsm->wptr == ivsm->rptr) && ivsm->fulled)
			usleep(SLEEP); // 시간 얼마? or clock_nanosleep()?

		// Partial-Write Until Endpoint
		if ((ivsm->wptr + remain_cnt > ENDPOINT) && (ivsm->wptr >= ivsm->rptr)) {
			printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", ivsm->wptr, ivsm->rptr, ivsm->fulled, remain_cnt);
			puts("(partial-write until endpoint)");
			to_write = ENDPOINT - ivsm->wptr;
			printf("to_write: %lu", to_write);
			
			memcpy((void *)ivsm + OFFSET + ivsm->wptr, (void*)(buf + processed_byte), to_write);
			remain_cnt -= to_write;
			processed_byte += to_write;

			if(0 == ivsm->rptr)
				ivsm->fulled = 1;

			ivsm->wptr = 0;

			continue;
		}

		// Partial-Write Until Read Pointer
		if (ivsm->wptr < ivsm->rptr) {
			printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", ivsm->wptr, ivsm->rptr, ivsm->fulled, remain_cnt);
			puts("(partial-write until write pointer)");
			to_write = ivsm->rptr - ivsm->wptr;
			printf("to_write: %lu", to_write);

			memcpy((void *)ivsm + OFFSET + ivsm->wptr, (void*)(buf + processed_byte), to_write);
			remain_cnt -= to_write;
			processed_byte += to_write;
			
			ivsm->fulled = 1;

			// 포인터가 엔드 포인트에 도달하면 0으로 변경
			if (ivsm->wptr + to_write == ENDPOINT)
				ivsm->wptr = 0;
			else
				ivsm->wptr += to_write;
			
			continue;
		}

		// 마지막 쓰기
		printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", ivsm->wptr, ivsm->rptr, ivsm->fulled, remain_cnt);
		memcpy((void *)ivsm + OFFSET + ivsm->wptr, (void*)(buf + processed_byte), remain_cnt);
		remain_cnt = 0;
		if((ivsm->wptr + remain_cnt == ivsm->rptr) || (ivsm->wptr + remain_cnt == ENDPOINT))
			ivsm->fulled = 1;

		// 포인터가 엔드 포인트에 도달하면 0으로 변경
		if (ivsm->wptr + remain_cnt == ENDPOINT) {
			printf("1111\n");
			ivsm->wptr = 0;
		} else {
			printf("2222\n");
			ivsm->wptr += remain_cnt;
		}
	}

	printf("IVSH: WRITE\n");

	return count;
}

