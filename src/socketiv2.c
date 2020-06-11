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
	IVSM *ivsm = ivsock->ivsm_addr_read;
	size_t to_read = 0; // if 문 안에서 처리할 바이트
	size_t remain_cnt = count, processed_byte = 0;

	while (remain_cnt) {
		// Poll
		while ((ivsm->rptr == ivsm->wptr) && !ivsm->fulled)
			usleep(SLEEP); // 시간 얼마? or clock_nanosleep()?

		// 순환
		if (ivsm->rptr > ivsm->wptr) {
			to_read = END_POINT - ivsm->rptr;
			
			memcpy(buf + processed_byte, (void *)ivsm + OFFSET + ivsm->rptr, to_read);
			ivsm->rptr = 0;
			remain_cnt -= to_read;
			processed_byte += to_read;
			ivsm->fulled = 0;

			continue;
		}

		// partial-read
		if (ivsm->rptr + remain_cnt > ivsm->wptr) {
			to_read = ivsm->wptr - ivsm->rptr;

			memcpy(buf + processed_byte, (void *)ivsm + OFFSET + ivsm->rptr, to_read);
			ivsm->rptr += to_read;
			remain_cnt -= to_read;
			processed_byte += to_read;
			ivsm->fulled = 0;

			// 예외 처리
			if (ivsm->rptr == END_POINT)
				ivsm->rptr = 0;

			continue;
		}

		// 마지막 읽기
		memcpy(buf + processed_byte, (void *)ivsm + OFFSET + ivsm->rptr, remain_cnt);
		ivsm->rptr += remain_cnt;
		remain_cnt = 0;
		ivsm->fulled = 0;

		// 예외 처리
		if (ivsm->rptr == END_POINT)
			ivsm->rptr = 0;
	}

	return count;
}

static inline int64_t getmstime(void) {
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);

	return (tp.tv_sec * 1000) + (tp.tv_nsec / 1000 / 1000);
}

ssize_t socketiv_write(int fd, const void *buf, size_t count) {
	IVSOCK *ivsock = fd_to_ivsock_map[fd];
	IVSM *ivsm = ivsock->ivsm_addr_write;
	size_t to_write = 0; // if 문 안에서 처리할 바이트
	size_t remain_cnt = count, processed_byte = 0;

	while (remain_cnt) {
		// Poll
		while ((ivsm->wptr == ivsm->rptr) && ivsm->fulled)
			usleep(SLEEP); // 시간 얼마? or clock_nanosleep()?

		// 순환
		if ((ivsm->wptr + remain_cnt > END_POINT) && (ivsm->wptr >= ivsm->rptr)) {
			to_write = END_POINT - ivsm->wptr;
			
			memcpy(buf + processed_byte, (void *)ivsm + OFFSET + ivsm->rptr, to_write);
			ivsm->wptr = 0;
			remain_cnt -= to_write;
			processed_byte += to_write;
			if(ivsm->wptr == ivsm->rptr)
				ivsm->fulled = 1;

			continue;
		}

		// partial-write
		if (ivsm->wptr < ivsm->rptr) {
			to_write = ivsm->rptr - ivsm->wptr;

			memcpy(buf + processed_byte, (void *)ivsm + OFFSET + ivsm->wptr, to_write);
			ivsm->wptr += to_write;
			remain_cnt -= to_write;
			processed_byte += to_write;
			ivsm->fulled = 1;

			// 예외 처리
			if (ivsm->wptr == END_POINT)
				ivsm->wptr = 0;

			continue;
		}

		// 마지막 쓰기
		memcpy(buf + processed_byte, (void *)ivsm + OFFSET + ivsm->wptr, remain_cnt);
		ivsm->wptr += remain_cnt;
		remain_cnt = 0;
		if(ivsm->wptr == ivsm->rptr)
			ivsm->fulled = 1;

		// 예외 처리
		if (ivsm->wptr == END_POINT)
			ivsm->wptr = 0;
	}

	return count;
}

