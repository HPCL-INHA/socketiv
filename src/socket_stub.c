#include <sys/socket.h>
#include <sys/types.h>

int socket(int domain, int type, int protocol) // create socket
{

}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) // set socket option
{
    return 0;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) // set port number
{

}

int listen(int sockfd, int backlog) // mark socket as passive socket
{

}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) // create new socket requested by passive socket
{

}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) // connect to server socket
{

}

ssize_t read(int fd, void *buf, size_t count) // read from socket
{

}

ssize_t write(int fd, const void *buf, size_t count) // write to socket
{

}

int close(int fd) // close socket
{

}