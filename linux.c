
#include <sys/epoll.h>

int epollfd;
struct epoll_event ev, events[10];

int add_socket(int fd){ // add a socket to the epoll
    printf("adding fd: %d\n",fd);
    ev.data.fd=fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1){
        return 1;
    }
    return 0;
}

int make_poll(){
    ev.events = EPOLLIN;
    int fd = epoll_create1(0);
    if(fd==-1){
        perror("error in epoll_create1(0)");
    }
    return fd;
}

#define BEGIN_WAITING(epollfd, fdn)    while((nfds=epoll_wait((epollfd), events, 10, -1))!=-1){\
        for(int n=0;n<nfds;n++){\
            fdn = events[n].data.fd;
#define END_WAITING        }\
    }\
    perror("epoll_wait failed");
