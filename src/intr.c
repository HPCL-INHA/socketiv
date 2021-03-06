#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <pthread.h>

#include "socketiv.h"

enum ivshmem_registers {
	IntrMask = 0,
	IntrStatus = 4,
	IVPosition = 8,
	Doorbell = 12,
	IVLiveList = 16
};

static int doorbell_fd;
static void *doorbell_mmap;
static void *plain_mmap_read;
static void *plain_mmap_write;

void intr_send(const unsigned short dest)
{
	const unsigned short cmd = 0;
	int msg = ((dest & 0xffff) << 16) + (cmd & 0xffff);

	*((int *)(doorbell_mmap + Doorbell)) = msg;

	// TODO: This can safely be async'ed, evaluate!
}

void intr_wait()
{
	ssize_t len;
	int buf;

	printf("INTR_WAIT\n");

	len = read(doorbell_fd, &buf, sizeof(buf));
	if (unlikely(len != sizeof(buf))) {
		perror("Interrupt received size mismatch");
		exit(1);
	}
}

void *intr_quirk_func(void *data)
{
	for (;;) {
		usleep(100 * 1000);
//		intr_send();
	}
}

void intr_init()
{
	pthread_t intr_quirk;
	uint64_t pagesize, addr, len;
	int fd;

#if 0
	// Setup doorbell fd for receiving interrupts
	doorbell_fd = open(doorbell_path, O_RDWR);
	if (doorbell_fd == -1) {
		perror("Failed to open doorbell device path");
		exit(1);
	}

	// Setup doorbell mmap for sending interrupts
	doorbell_mmap =
	    mmap(NULL, 256, PROT_READ | PROT_WRITE, MAP_SHARED, doorbell_fd, 0);
	if (doorbell_mmap == MAP_FAILED) {
		perror("Failed to mmap doorbell device path");
		exit(1);
	}
#endif

	// Setup plain mmap for looking up read()/write() sizes
	fd = open("/dev/mem", O_RDWR);
	if (fd == -1) {
		perror("Failed to open /dev/mem");
		exit(1);
	}

	pagesize = getpagesize();
	addr = PHYS_ADDR_READ & (~(pagesize - 1));
	len = (PHYS_ADDR_READ & (pagesize - 1)) + TOTAL_SIZE;
	plain_mmap_read = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr);
	if (plain_mmap_read == MAP_FAILED) {
		perror("Failed to mmap plain device path");
		exit(1);
	}

	addr = PHYS_ADDR_WRITE & (~(pagesize - 1));
	len = (PHYS_ADDR_WRITE & (pagesize - 1)) + TOTAL_SIZE;
	plain_mmap_write = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr);
	if (plain_mmap_write == MAP_FAILED) {
		perror("Failed to mmap plain device path");
		exit(1);
	}

	close(fd);

	// Setup interrupt quirk thread to send occasional interrupts
//	pthread_create(&intr_quirk, NULL, intr_quirk_func, NULL);
}
