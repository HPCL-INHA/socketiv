#include "socketiv.h"

#include <assert.h>

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

	// temporal storage for shared variable
	size_t temp_rptr, temp_wptr;
	int temp_fulled, temp_enabled;
	
	temp_enabled = ivsm->enabled;
	temp_fulled = ivsm->fulled;
	temp_rptr = ivsm->rptr;
	temp_wptr = ivsm->wptr;

	// Check valid connection
	temp_enabled = ivsm->enabled;
	temp_fulled = ivsm->fulled;
	temp_rptr = ivsm->rptr;
	temp_wptr = ivsm->wptr;
	if ((temp_rptr == temp_wptr) && !temp_fulled && !temp_enabled)
		return -1;

	while (remain_cnt) {

		// try to correct 'fulled' bit
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;
		if (temp_rptr != temp_wptr)
			ivsm->fulled = 0;

		// Poll
		temp_fulled = ivsm->fulled;
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;
		while ((temp_rptr == temp_wptr) && !temp_fulled){

			temp_enabled = ivsm->enabled;
			if(!temp_enabled)
				return processed_byte;
			usleep(SLEEP); // 시간 얼마? or clock_nanosleep()?

			temp_fulled = ivsm->fulled;
			temp_wptr = ivsm->wptr;
			temp_rptr = ivsm->rptr;
		}

		// Partial-Read Until Endpoint
		temp_enabled = ivsm->enabled;
		temp_fulled = ivsm->fulled;
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;
		if ((temp_rptr > temp_wptr) && (temp_rptr + remain_cnt) > ENDPOINT){

			to_read = ENDPOINT - temp_rptr;

			memcpy(buf + processed_byte, (void *)ivsm + OFFSET + temp_rptr, to_read);
			remain_cnt -= to_read;
			processed_byte += to_read;

			// Update Shared Variables
			ivsm->fulled = 0;
			ivsm->rptr = 0;


			continue;
		}

		// Partial-Read Until Write Pointer
		temp_enabled = ivsm->enabled;
		temp_fulled = ivsm->fulled;
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;
		if ((temp_rptr + remain_cnt > temp_wptr) && (temp_rptr < temp_wptr)) {

			to_read = temp_wptr - temp_rptr;

			memcpy(buf + processed_byte, (void *)ivsm + OFFSET + temp_rptr, to_read);
			remain_cnt -= to_read;
			processed_byte += to_read;

			// Update Shared Variables
			ivsm->fulled = 0;
			ivsm->rptr += to_read;

			//usleep(SLEEP); // ???

			continue;
		}

		// 마지막 읽기
		temp_enabled = ivsm->enabled;
		temp_fulled = ivsm->fulled;
		temp_rptr = ivsm->rptr;
		temp_wptr = ivsm->wptr;

		memcpy(buf + processed_byte, (void *)ivsm + OFFSET + temp_rptr, remain_cnt);
		processed_byte += remain_cnt;

		prev_remain_cnt = remain_cnt;
		remain_cnt = 0;

		// Update Shared Variables
		ivsm->fulled = 0;
		if (temp_rptr + prev_remain_cnt == ENDPOINT) { // 포인터가 엔드 포인트에 도달하면 0으로 변경
			ivsm->rptr = 0;
		} else
			ivsm->rptr += prev_remain_cnt;

	}


	assert(processed_byte == count);
	return processed_byte;
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

	// temporal storage for shared variable
	size_t temp_rptr, temp_wptr;
	int temp_fulled, temp_enabled;

	temp_rptr = ivsm->rptr;
	temp_wptr = ivsm->wptr;
	temp_fulled = ivsm->fulled;
	temp_enabled = ivsm->enabled;

	// Check valid connection
	temp_enabled = ivsm->enabled;
	if (!temp_enabled)
		return -1;

	while (remain_cnt) {
		
		// try to correct 'fulled' bit
		temp_wptr = ivsm->wptr;
		temp_rptr = ivsm->rptr;
		if (temp_wptr != temp_rptr)
			ivsm->fulled = 0;

		temp_enabled = ivsm->enabled;
		if (!temp_enabled)
			return processed_byte;

		// Poll
		temp_fulled = ivsm->fulled;
		temp_wptr = ivsm->wptr;
		temp_rptr = ivsm->rptr;
		while ((temp_wptr == temp_rptr) && temp_fulled){

			usleep(SLEEP); // 시간 얼마? or clock_nanosleep()?

			temp_fulled = ivsm->fulled;
			temp_wptr = ivsm->wptr;
			temp_rptr = ivsm->rptr;
		}

		// Partial-Write Until Endpoint
		temp_enabled = ivsm->enabled;
		temp_fulled = ivsm->fulled;
		temp_wptr = ivsm->wptr;
		temp_rptr = ivsm->rptr;
		if ((temp_wptr + remain_cnt > ENDPOINT) && (temp_wptr >= temp_rptr)) {

			to_write = ENDPOINT - temp_wptr;
			
			memcpy((void *)ivsm + OFFSET + temp_wptr, (void*)(buf + processed_byte), to_write);
			remain_cnt -= to_write;
			processed_byte += to_write;

			// Update Shared Variable
			temp_rptr = ivsm->rptr;
			if(!temp_rptr)
				ivsm->fulled = 1;
			ivsm->wptr = 0;


			continue;
		}

		// Partial-Write Until Read Pointer
		temp_enabled = ivsm->enabled;
		temp_fulled = ivsm->fulled;
		temp_wptr = ivsm->wptr;
		temp_rptr = ivsm->rptr;
		if ((temp_wptr + remain_cnt > temp_rptr) && (temp_wptr < temp_rptr)) {

			to_write = temp_rptr - temp_wptr;

			memcpy((void *)ivsm + OFFSET + temp_wptr, (void*)(buf + processed_byte), to_write);
			remain_cnt -= to_write;
			processed_byte += to_write;
			
			// Update Shared Variables
			ivsm->fulled = 1;
			ivsm->wptr += to_write;


			continue;
		}

		// 마지막 쓰기
		temp_enabled = ivsm->enabled;
		temp_fulled = ivsm->fulled;
		temp_wptr = ivsm->wptr;
		temp_rptr = ivsm->rptr;

		memcpy((void *)ivsm + OFFSET + temp_wptr, (void*)(buf + processed_byte), remain_cnt);
		processed_byte += remain_cnt;

		prev_remain_cnt = remain_cnt;
		remain_cnt = 0;
		
		// Update Shared Variables
		if((temp_wptr + prev_remain_cnt == temp_rptr) || (temp_wptr + prev_remain_cnt == ENDPOINT))
			ivsm->fulled = 1;
		if (temp_wptr + prev_remain_cnt == ENDPOINT) { // 포인터가 엔드 포인트에 도달하면 0으로 변경
			ivsm->wptr = 0;
		}
		else
			ivsm->wptr += prev_remain_cnt;

		//usleep(SLEEP); // ???
	}


	assert(processed_byte == count);
	return processed_byte;
}

