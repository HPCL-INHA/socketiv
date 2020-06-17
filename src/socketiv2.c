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
	size_t remain_cnt = count, prev_remain_cnt, processed_byte = 0;
	IVSM *ivsm = ivsock->ivsm_addr_read;

	size_t temp_rptr, temp_wptr, temp_fulled, temp_enabled; // temporal storage for shared variable

	while (remain_cnt) {
		// fulled 레이스 컨디션 수정 - 수정 필요하거나 삭제하게 될 수 있음
		if (ivsm->wptr != ivsm->rptr)
			ivsm->fulled = 0;
		else
			ivsm->fulled = 1;

		// Poll
		while ((ivsm->rptr == ivsm->wptr) && !ivsm->fulled){
			if(!ivsm->enabled)
				return -1;
			usleep(SLEEP); // 시간 얼마? or clock_nanosleep()?
		}

		printf("\n");
		printf("[remain_cnt]: %lu\n", remain_cnt);
		printf("[processed_byte]: %lu\n", processed_byte);
		printf("\n");

		// Partial-Read Until Endpoint
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;
		temp_fulled = ivsm->fulled;
		temp_enabled = ivsm->enabled;
		if ((temp_rptr > temp_wptr) && (temp_rptr + remain_cnt) > ENDPOINT){
			printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", temp_wptr, temp_rptr, temp_fulled, remain_cnt);

			puts("(partial-read until endpoint)");
			to_read = ENDPOINT - temp_rptr;
			printf("to_read: %lu\n", to_read);

			memcpy(buf + processed_byte, (void *)ivsm + OFFSET + temp_rptr, to_read);
			remain_cnt -= to_read;
			processed_byte += to_read;

			// Update Shared Variables
			ivsm->fulled = 0;
			ivsm->rptr = 0;

			continue;
		}

		// Partial-Read Until Write Pointer
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;
		temp_fulled = ivsm->fulled;
		temp_enabled = ivsm->enabled;
		if (temp_rptr + remain_cnt > temp_wptr) {
			printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", temp_wptr, temp_rptr, temp_fulled, remain_cnt);

			puts("(partial-read until write pointer)");
			to_read = temp_wptr - temp_rptr;
			printf("to_read: %lu\n", to_read);

			memcpy(buf + processed_byte, (void *)ivsm + OFFSET + temp_rptr, to_read);
			remain_cnt -= to_read;
			processed_byte += to_read;

			// Update Shared Variables
			ivsm->fulled = 0;
			ivsm->rptr += to_read;

			continue;
		}

		// 마지막 읽기
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;
		temp_fulled = ivsm->fulled;
		temp_enabled = ivsm->enabled;
		printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", temp_wptr, temp_rptr, temp_fulled, remain_cnt);
		puts("(final read)");
		memcpy(buf + processed_byte, (void *)ivsm + OFFSET + temp_rptr, remain_cnt);

		prev_remain_cnt = remain_cnt;
		remain_cnt = 0;

		// Update Shared Variables
		ivsm->fulled = 0;
		if (ivsm->rptr + prev_remain_cnt == ENDPOINT) { // 포인터가 엔드 포인트에 도달하면 0으로 변경
			puts("(rptr reached endpoint)");
			ivsm->rptr = 0;
		} else
			ivsm->rptr += prev_remain_cnt;
	}

	printf(" IVSH: READ %lu\n", count);

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
	size_t remain_cnt = count, prev_remain_cnt, processed_byte = 0;
	IVSM *ivsm = ivsock->ivsm_addr_write;

	size_t temp_rptr, temp_wptr, temp_fulled, temp_enabled; // temporal storage for shared variable

	while (remain_cnt) {
		// fulled 레이스 컨디션 수정 - 수정 필요하거나 삭제하게 될 수 있음
		if (ivsm->wptr != ivsm->rptr)
			ivsm->fulled = 0;
		else
			ivsm->fulled = 1;

		if (!ivsm->enabled)
			return -1;

		// Poll
		while ((ivsm->wptr == ivsm->rptr) && ivsm->fulled)
			usleep(SLEEP); // 시간 얼마? or clock_nanosleep()?

		// Partial-Write Until Endpoint
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;
		temp_fulled = ivsm->fulled;
		temp_enabled = ivsm->enabled;
		if ((temp_wptr + remain_cnt > ENDPOINT) && (temp_wptr >= temp_rptr)) {
			printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", temp_wptr, temp_rptr, temp_fulled, remain_cnt);

			puts("(partial-write until endpoint)");
			to_write = ENDPOINT - temp_wptr;
			printf("to_write: %lu\n", to_write);
			
			memcpy((void *)ivsm + OFFSET + temp_wptr, (void*)(buf + processed_byte), to_write);
			remain_cnt -= to_write;
			processed_byte += to_write;

			// Update Shared Variable
			if(0 == ivsm->rptr)
				ivsm->fulled = 1;
			ivsm->wptr = 0;

			continue;
		}

		// Partial-Write Until Read Pointer
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;
		temp_fulled = ivsm->fulled;
		temp_enabled = ivsm->enabled;
		if (ivsm->wptr + remain_cnt > ivsm->rptr && ivsm->wptr < ivsm->rptr) {
			printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", temp_wptr, temp_rptr, temp_fulled, remain_cnt);

			puts("(partial-write until read pointer)");
			to_write = temp_rptr - temp_wptr;
			printf("to_write: %lu\n", to_write);

			memcpy((void *)ivsm + OFFSET + temp_wptr, (void*)(buf + processed_byte), to_write);
			remain_cnt -= to_write;
			processed_byte += to_write;
			
			// Update Shared Variables
			ivsm->fulled = 1;
			ivsm->wptr += to_write;
			
			continue;
		}

		// 마지막 쓰기
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;
		temp_fulled = ivsm->fulled;
		temp_enabled = ivsm->enabled;
		printf("WPTR: %lu, RPTR: %lu, fulled: %d, remain_cnt: %lu\n", temp_wptr, temp_rptr, temp_fulled, remain_cnt);
		puts("(final write)");
		memcpy((void *)ivsm + OFFSET + temp_wptr, (void*)(buf + processed_byte), remain_cnt);

		prev_remain_cnt = remain_cnt;
		remain_cnt = 0;
		
		// Update Shared Variables
		if((temp_wptr + prev_remain_cnt == temp_rptr) || (temp_wptr + prev_remain_cnt == ENDPOINT))
			ivsm->fulled = 1;
		if (ivsm->wptr + prev_remain_cnt == ENDPOINT) { // 포인터가 엔드 포인트에 도달하면 0으로 변경
			puts("(wptr reached endpoint)");
			ivsm->wptr = 0;
		} else
			ivsm->wptr += prev_remain_cnt;
	}

	printf("IVSH: WRITE\n");

	return count;
}

