#include "socketiv.h"
#include "intr.h"

FD_INFO *fd_list;
int fd_list_size;
void __attribute__((constructor)) socketiv_init();

int socketiv_start_qos();
int socketiv_stop_qos();

int socketiv_interrupt();
int socketiv_wait();

int socketiv_poll();

int socketiv_sleep();

void __attribute__((constructor)) socketiv_init()
{
    fd_list = calloc(3, sizeof(FD_INFO));
    fd_list_size = 3;

    orig_open = (int (*)(const char *, int, mode_t))dlsym(RTLD_NEXT, "open");
    orig_socket = (int (*)(int, int, int))dlsym(RTLD_NEXT, "socket");
    orig_accept = (int (*)(int, struct sockaddr *, socklen_t *))dlsym(RTLD_NEXT, "accept");
    orig_connect = (int (*)(int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "connect");
    orig_read = (ssize_t(*)(int, void *, size_t))dlsym(RTLD_NEXT, "read");
    orig_write = (ssize_t(*)(int, void *, size_t))dlsym(RTLD_NEXT, "write");
    orig_close = (int (*)(int))dlsym(RTLD_NEXT, "close");

    intr_init();
}

// link
static int socketiv_link_local(SOCKETIV * socketiv, const char *file_name,
			 const int server)
{
	// check size of the file mapped to shared memory
	struct stat st;
	if (stat(file_name, &st) < 0) {
		return ERR_INVALID;
	}
	socketiv->size = st.st_size;
	socketiv->len = socketiv->size;

	// get the file descriptor
	socketiv->fd = open(file_name, O_RDWR, 0600);
	if (socketiv->fd < 0) {
		perror("open()");
		exit(1);
	}

	// map the file to its virtual address space
	socketiv->virt_addr =
	    mmap(NULL, socketiv->len, PROT_READ | PROT_WRITE, MAP_SHARED,
		 socketiv->fd, 0);
	if (!socketiv->virt_addr) {
		perror("mmap()");
		exit(1);
	}

	// set memory type as local
	socketiv->loc = LOCAL;

	return NO_ERR;
}

static int socketiv_link_remote(SOCKETIV * socketiv, const uint64_t phys_addr,
			  const size_t size, const int server)
{
	uint64_t pagesize = getpagesize();
	uint64_t addr = phys_addr & (~(pagesize - 1));
	socketiv->size = size;
	socketiv->len = (phys_addr & (pagesize - 1)) + size;

	// get the file descriptor
	socketiv->fd = open("/dev/mem", O_RDWR, 0600);
	if (socketiv->fd < 0) {
		perror("open()");
		exit(1);
	}

	// map the file to its virtual address space
	socketiv->virt_addr =
	    mmap(NULL, socketiv->len, PROT_READ | PROT_WRITE, MAP_SHARED,
		 socketiv->fd, addr);
	if (!socketiv->virt_addr) {
		perror("mmap()");
		exit(1);
	}

	// set memory type as remote
	socketiv->loc = REMOTE;

	return NO_ERR;
}

int socketiv_register_fd(int fd, FD_TYPE fd_type, const IVSOCK *ivsock_ptr)
{
	if (fd_list_size < fd) {
		// Re-allocate fd_list
		realloc(fd_list, fd * sizeof(FD_INFO));
		memset(fd_list + (fd_list_size * sizeof(FD_INFO)),
		       0,
		       (fd - fd_list_size) * sizeof(FD_INFO));
		fd_list_size = fd;
	}

	fd_list[fd]->fd_type = fd_type;
	fd_list[fd]->ivsock_ptr = ivsock_ptr;

	// socketiv_link_remote();
}

FD_TYPE socketiv_check_fd(int fd)
{
	if (unlikely(fd_list_size < fd))
		return TYPE_GENERIC;

	return fd_list[fd]->fd_type;
}

int socketiv_unregister_fd(int fd)
{
}

bool socketiv_check_vm_subnet(const struct sockaddr *addr)
{
	// Only accepts IPv4 atm
	char buf[1024];

	inet_ntop(AF_INET, &(((struct sockaddr_in *)addr)->sin_addr), buf, 1024);

	return !strncmp(buf, VM_ADDR, strlen(VM_ADDR));
}

int socketiv_accept(int sockfd)
{
}

int socketiv_connect(int sockfd)
{
}

ssize_t socketiv_read(int fd, void *buf, size_t count)
{
	// Interrupt
	intr_wait();

	// memcpy
	// rptr
}

ssize_t socketiv_write(int fd, const void *buf, size_t count)
{
	// memcpy
	// wptr

	// Interrupt
	intr_send();
}
