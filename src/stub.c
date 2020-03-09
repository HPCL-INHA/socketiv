#include <sys/socket.h>
#include <sys/types.h>

#include "socketiv.h"

int open(const char *pathname, int flags, mode_t mode){ // open file
    int fd = orig_open(pathname, flags, mode);
    if(fd >= 0)
        socketiv_register_fd(fd, SOCKETIV_FD_TYPE_GEN);
    return fd;
}

int socket(int domain, int type, int protocol){ // create socket
    int sockfd = orig_socket(domain, type, protocol);
    if(sockfd >= 0)
        socketiv_register_fd(sockfd, SOCKETIV_FD_TYPE_GEN);
    return sockfd;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){ // create new socket requested by passive socket
    int new_sockfd = orig_accept(sockfd, addr, addrlen);
    if(new_sockfd >= 0)
        if(socketiv_check_vm_subnet(addr))
            socketiv_accept(new_sockfd);
    return new_sockfd;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){ // connect to server socket
    int ret = orig_connect(sockfd, addr, addrlen);
    if(ret == 0)
        if(socketiv_check_vm_subnet(addr))
            socketiv_connect(sockfd);
    return ret;
}

ssize_t read(int fd, void *buf, size_t count){ // read from socket
    if(socketiv_check_fd(fd) == SOCKETIV_FD_TYPE_IVSOCK)
        return socketiv_read(fd, buf, count);
    return orig_read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count){ // write to socket
    if(socketiv_check_fd(fd) == SOCKETIV_FD_TYPE_IVSOCK)
        return socketiv_write(fd, buf, count);
    return orig_write(fd, buf, count);
}

int close(int fd){ // close socket
    int ret;
    if(socketiv_check_fd(fd) == SOCKETIV_FD_TYPE_IVSOCK)
        socketiv_close(fd);
    if((ret = orig_close(fd)) == 0)
        socketiv_unregister_fd(fd);
    return ret;
}
