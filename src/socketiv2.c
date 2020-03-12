#include "socketiv.h"

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

// determine whether this address belongs to virtual network
int socketiv_accept(int new_sockfd)
{
	//if(socketiv_create_ivshmem(sockfd, 무슨 인자?) || socketiv_alter_fd(fd, SOCKETIV_FD_TYPE_IVSOCK))
	//      return EXIT_FAILURE;
	//return EXIT_SUCCESS;
	// TODO
	return 0;
}

int socketiv_connect(int sockfd)
{
	//if(socketiv_create_ivshmem(sockfd, 무슨 인자?) || socketiv_alter_fd(fd, SOCKETIV_FD_TYPE_IVSOCK))
	//      return EXIT_FAILURE;
	//return EXIT_SUCCESS;
	// TODO
	return 0;
}

ssize_t socketiv_read(int fd, void *buf, size_t count)
{
	IVSM *ivsm_ptr = fd_map[fd]->ivsm_ptr;
	ssize_t len, wlen;

	// 인터럽트 모드 일 때: 인터럽트가 올 때 까지 wait -> 인터럽트가 발생하면 available 한 블록들 모두 read copy
	/* if (mode == interrupt) */ {
		// TODO: Calculate interrupt storm rate
		// TODO: EOF & Loop handling??
		for (len = 0; len <= count - blk_size; len += blk_size)
		{
			intr_wait();
			wlen = ivsm_ptr->host_wlen;
			memcpy(buf + len, ivsm_ptr + sizeof(IVSM), wlen);
			ivsm_ptr->host_rlen = len;
		}
		return len;
	}

	/*
	// 인터럽트가 과도할 때(즉 IVSOCK 내의 인터럽트 한계(count/s)를 넘긴 경우): IVSOCK 내의 기술된 폴링 모드로 전환
	else {
		// TODO: Calculate polling storm rate and adjust usleep value
		// usleep(value);
		// TODO: if (last request was a while ago) mode = interrupt;
		len = *(ssize_t*)plain_mmap;
		memcpy(buf, plain_mmap + 256, len); // 256: HARDCODING ATM
		return len;
	}
*/

	// 스루풋이 더욱 증가하여 threshold 를 초과할 시: IVSOCK 내의 기술된 폴링 모드(= 폴링 interval)를 다음 단계로 증가하여 interval 단축
	// 스루풋이 하향: 이전 interval로 단축
	// 일정 time window(burst timeout mode) 동안 전송이 발생하지 않음: 다시 인터럽트 모드로 전환
	// 큰 쓰기가 발생한 경우: 더욱 큰 block size로 전환 - 이후 충분히 큰 쓰기가 이뤄지지 않으면 block size 축소
	// TODO: ^ ???
}

ssize_t socketiv_write(int fd, const void *buf, size_t count)
{
	IVSM *ivsm_ptr = fd_map[fd]->ivsm_ptr;
	ssize_t len;

	// 인터럽트 모드 일 때: 쓰는 동안 블록 사이즈 별로 나눠 인터럽트
	/* if (mode == interrupt) */ {
		ssize_t blk_size = fd_map[fd]->block_size_mode;
		for (len = 0; len <= count - blk_size; len += blk_size)
		{
			ivsm_ptr->host_wlen = len;
			memcpy(ivsm_ptr + sizeof(IVSM) + len - blk_size, buf + len - blk_size, blk_size);
			intr_send();
		}
		if (len != count)
		{
			// Send remaining data
			// TODO: 블록 사이즈 보다 작은 write의 경우: 지정된 delay time window 만큼 대기후에 인터럽트(이때는 메인 함수내에서 block 하지 않음)
			ivsm_ptr->host_wlen = count - len;
			memcpy(ivsm_ptr + sizeof(IVSM) + count - len - blk_size, buf + count - len - blk_size, blk_size);
			intr_send();
		}
	}
	// - 인터럽트 delay 중(즉 non-block으로서 메인함수로 돌아간 경우), socket에 read() 할 시 delay time window 축소 - 이런 경우가 발생할 때 마다 단계적으로 축소(반대의 경우는 단계적으로 회복)

	//	{
	// 인터럽트가 과도할 때: reader를 polling mode로 전환 시킴
	// 스루풋이 증가할 시: reader에게 더 짧은 polling interval mode로 전환하라 명령
	// 스루풋이 하향: reader에게 더 긴 polling interval mode로 전환하라 명령
	//	} copy from above

	// 일정 time window 동안 전송이 발생하지 않음: reader를 인터럽트 모드로 wait, 다시 인터럽트 모드로 전환
	// 큐를 모두 채울때: 단계적으로 큐를 키움 - 이후, 충분히 쓰지 않으면 단계적으로 큐 축소
	// 큰 쓰기가 발생한 경우: 더욱 큰 block size로 전환 - 이후 충분히 큰 쓰기가 이뤄지지 않으면 block size 축소
}
