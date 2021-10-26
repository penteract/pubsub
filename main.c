// https://www.binarytides.com/socket-programming-c-linux-tutorial/
#include <netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include <stdbool.h>
#include<signal.h>
#include <fcntl.h>
#include<errno.h>
#include <sys/resource.h>


#ifndef PORT
#define PORT 8086
#endif

#define MAXCONNS 470772
char status[MAXCONNS];
int close_and_track(int fd){
    status[fd] = 0;
    return close(fd);
}

#include<linux.c>
#include<websockets.c>
#include<server.c>

//char res[100];

int main(int argc , char *argv[])
{
    //sha1("dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11",60,res);
    //printf("b64: %s\n",res);
    
    // Ensure that the program doesn't write out of bounds if the number of fds on the server increases
    struct rlimit rlp;
    getrlimit(RLIMIT_NOFILE, &rlp);
    rlp.rlim_cur = MAXCONNS;
    setrlimit(RLIMIT_NOFILE, &rlp);
    
    int listen_sock, new_socket, k;
    listen_sock = socket(AF_INET , SOCK_STREAM , 0);
    if (listen_sock == -1)
    {
        printf("Could not create socket");
    }
    //https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
    //TODO: remove when done debugging
    int enable = 1;
    signal(SIGPIPE, SIG_IGN);
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)); 
    struct sockaddr_in server, client;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons( PORT );
    int c = bind(listen_sock , (struct sockaddr *) &server, sizeof(server));
    if (c<0){
        perror("bind failed");
        return 1;
    }
    puts("Bound");
    printf("%d\n",c);
    listen(listen_sock , 3);
    //puts("Waiting for incoming conns");
    c = sizeof(struct sockaddr_in);

    epollfd = make_poll();
    if (epollfd == -1){
        return 1;
    }

    add_socket(listen_sock);
    int nfds;
    int fdn;
    BEGIN_WAITING(epollfd, fdn)
        printf("notified from fd %d\n",fdn);
        if(fdn == listen_sock){
            new_socket = accept(listen_sock, (struct sockaddr*) &client, (socklen_t*) &k);
            if(new_socket<=0){
                perror("accept failed");
                return 1;
            }
            status[new_socket] = 1; // starting
            add_socket(new_socket);
        }
        else if(status[fdn] == 1){
            if(process(fdn)){
                close_and_track(fdn);
            }
            else{
                status[fdn]=2; // running
            }
        }
        else{ // I should add another status "closing" to close the websockets properly.
            c = recv(fdn, request, 1000, 0);
            if(c>0){
                request[c]=0; //for parser safety, not just for printing convenience
                printf("count: %d\n",c);
                //puts(request);
                for(int i=0;i<c;i++)printf("%02x",request[i]&0xff);
                puts("\n");
                if(more_data(fdn,request, c)) {
                    close_websocket(fdn);
                }
            } else {
                perror("recv failed");
                close_websocket(fdn);
            }
        }
    END_WAITING
    close(listen_sock);
	return 0;
}
