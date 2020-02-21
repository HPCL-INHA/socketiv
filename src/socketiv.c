#include "socketiv.h"

FD_INFO *fd_list;
int fd_list_size;
void __attribute__((constructor)) socketiv_init();

int socketiv_start_QoS();
int socketiv_stop_QoS();

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
}