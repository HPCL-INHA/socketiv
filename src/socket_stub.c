#include <sys/socket.h>
#include <sys/types.h>

#include "socketiv.h"

int open(const char *pathname, int flags, mode_t mode) // open file
{
    int fd = original_open(pathname, flags, mode);
    socketiv_register_socket(fd, TYPE_GENERIC, NULL);
}

int socket(int domain, int type, int protocol) // create socket
{
    int sock = original_socket(domain, type, protocol);
    socketiv_register_socket(sock, TYPE_GENERIC, NULL);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) // create new socket requested by passive socket
{
    int ret;
    if (ret = orig_accept(sockfd, addr, addrlen))
    {
        return ret;
    }
    if (socketiv_check_vm_subnet(addr))
    {
        socketiv_accept(sockfd);
    }
    return ret;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) // connect to server socket
{
    int ret;
    if (ret = orig_connect(sockfd, addr, addrlen))
    {
        return ret;
    }
    if (socketiv_check_vm_subnet(addr))
    {
        socketiv_connect(sockfd);
    }
    return ret;
}

ssize_t read(int fd, void *buf, size_t count) // read from socket
{
    if (socketiv_check_fd(fd) == TYPE_IVSOCK)
    {
        return socketiv_read(fd, buf, count);
    }
    return orig_read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) // write to socket
{
    if (socketiv_check_fd(fd) == TYPE_IVSOCK)
    {
        return socketiv_write(fd, buf, count);
    }
    return orig_write(fd, buf, count);
}

int close(int fd) // close socket
{
    if (socketiv_check_fd(fd) == TYPE_IVSOCK)
    {
        socketiv_close(fd);
    }
    int ret;
    if (ret = orig_close(fd))
    {
        return ret;
    }
    socketiv_unregister_fd(fd);
    return ret;
}
