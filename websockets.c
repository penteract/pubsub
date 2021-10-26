#include<general.c>

int sndlen(int sock, char* msg, int len){
    printf("\nsending message of length %d to socket %d\n", len, sock);
    puts(msg);
    int c = send(sock,msg,len,0);
    if(c<0){perror("send failed");}
    printf("send result: %d\n",c);
    if(c<0){return -1;}
    if(c<len){
        printf("!!! sent partial message - %d bytes of %d\n", c, len );
        return -1;
    }
    return 0;
}
int snd(int sock, char* msg){
    return sndlen(sock,msg,strlen(msg));
}

unsigned short tags[MAXCONNS];

#define CONNSPERTAG 100
int listening[65536][CONNSPERTAG];

char ws_response[] = "HTTP/1.1 101 Switching Protocols\r\n" //34
"Upgrade: websocket\r\n" // 20 (54)
"Connection: Upgrade\r\n" // 21 (75)
"Sec-WebSocket-Accept: ****************************\r\n" //starts at 97, length 28
"\r\n";

// char ws_close_message[] = "\x88\x00";
// char ws_join_message[] = "\x82\x01\x01";

int close_websocket(int fd){
    if(tags[fd]){
        int idx;
        for(idx=0; idx<CONNSPERTAG; idx++){
            if(listening[tags[fd]][idx]==fd) break;
        }
        if(listening[tags[fd]][idx]!=fd){
            printf("!!! fd %d at tag %d not found in listening\n",fd,tags[fd]);
        }
        else listening[tags[fd]][idx]=0;
        tags[fd]=0;
    }
    else printf("!!! fd %d does not have a tag \n",fd);
    return close_and_track(fd);
}

char hash_target[] = "************************258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; // key length 24, total length 60

char* hash_loc = &ws_response[97];

int ws_header(int fd, char* name, int name_len, char* value, int value_len){
    printf("processing header - key:%.*s \nvalue:%.*s\n",name_len,name,value_len,value);
    if(name_len!=17 || memcmp(name, "Sec-WebSocket-Key", 17)!=0){
        return 0;
    }
    if(value_len!=24) return -1;
    memcpy(hash_target,value,24);
    sha1(hash_target,60,ws_response+97);
    return 1;
}

int islws(char c){
    return (c==' ' || c=='\t');
}

int ws_init(int fd, char* request){
    // find the tag being connected to
    puts("starting ws_init\n");
    unsigned short tag = 0;
    for(int i=0; i<4; i++){
        tag <<= 4;
        unsigned char c = *(request++);
        if(c==0) return 1;
        tag += c & 0x0F;
    }
    if(tag==0) tag+=0xa6; // avoid a tag of 0 to make debugging easier
    int idx;
    for(idx=0;idx<CONNSPERTAG;idx++){
        if(listening[tag][idx]==0)break;
    }
    if(listening[tag][idx]) return 1; // tag is full
    while(*(++request)!='\r'){
        if ((*request)=='\0') return 1;
    }
    if(*(++request) != '\n') return 1;
    // process each header
    request++;
    bool found = false;
    while(memcmp(request,"\r\n",2)){
        char* name_start = request;
        int name_len=0;
        if(*request==' ' || *request==':') return 1;
        while(*request!=' ' && *request!=':'){
            if (*request=='\0') return 1;
            request++;
            name_len++;
        }
        //checking for whitespace here is not required as of rfc7230 and whether it's required for
        // rfc2616 (as implied LWS) is ambiguous to me
        while(*request!=':'){
            request++;
            if (*request=='\0') return 1;
        }
        request++;
        if (*request=='\0') return 1;
        
        // skip leading whitespace, folding lines together
        while(islws(*request)){
            request++;
            if (*request=='\0') return 1;
            if(memcmp(request,"\r\n",2)==0){
                if(islws(*(request+2))) request+=2;
            }
        }
        char* value_start = request;
        int value_len = 0;
        int len_inc_ws = 0;
        while(memcmp(request,"\r\n",2)){
            if (*request=='\0') return 1;
            len_inc_ws++;
            if(!islws(*request)) value_len=len_inc_ws;
            request++;
            if(memcmp(request,"\r\n",2)==0 && islws(*(request+2)) ){
                len_inc_ws+=2;
                request+=2;
            }
        }
        request+=2;
        //We've now found the header name and value, let's process it
        if(!found){
            if(ws_header(fd, name_start, name_len, value_start, value_len)==1){
                found=true;
            }
        }
    }
    if(!found) return 1;
    else{
        snd(fd,ws_response);
        listening[tag][idx]=fd;
        tags[fd]=tag;
        return 0;
    }
}
/*
int new_conn(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd,F_SETFL, flags|O_NONBLOCK);
    int n = recv(fd, request, REQUEST_SIZE, 0);
    if(n<0){
        int e = errno;
        perror("recv faild");
        if(e&(EAGAIN|EWOULDBLOCK)){
            snd(fd,"HTTP/1.1 408 Too Slow\r\n\r\n");
        }
        return -1;
    }
}*/

int more_data(int fd, char* data, int data_len){
    if (data_len<6) return -1;
    unsigned char flags = data[0];
    printf("flags %0hx, ",flags);
    unsigned char len = data[1];
    printf("mask %d, ", len>>7);
    len&=127;
    printf("payload len %d\n",len);
    if(len>=126 || len!=data_len-6) return -1; // control frames can't be longer than that anyway
    char* mask = data+2;
    puts("mask: ");
    for(int i=0;i<4;i++)printf("%02x",mask[i]&0xff);
    puts("\n");
    for(int i=0;i<len;i++) data[i+6]^=mask[i&3];
    printf("data: ");
    for(int i=0;i<len;i++)printf("%02x",data[i+6]&0xff);
    printf("\n");
    data[4] = data[0]; // set up for forwarding the frame to others or replying to it (close/pong)
    data[5]=len;
    if(flags&0x08){//control frame
        switch(flags&0x0f){
            case 0x08: // close - reply with recieved data
                sndlen(fd,data+4,len+2);
                return -1;
            case 0x09: //ping - reply with pong (fun fact: forwarding pings and pongs would be conformant so long
                 // as there are at least 2 clients on each non-empty tag and they are all conformant)
                data[4] = (flags&0xf0)|0x0a;
                return sndlen(fd,data+4,len+2);
            case 0x0a: // lone pong, no need to respond
                return 0;
            default:
                return -1; //must fail here - unrecognised control frame
        }
    }
    if(flags!=0x82) return -1; // only permit final binary frames
    //send it to everyone else
    // Only accept binary frames to avoid having to permit utf8
    for(int i=0; i<CONNSPERTAG; i++){
        int other = listening[tags[fd]][i];
        if(other && other!=fd){
            if(sndlen(other, data+4, len+2)){
                listening[tags[fd]][i]=0;
                if (tags[other] != tags[fd]){
                    printf("!!! fd %d expected to have tag %d, instead has tag %d\n",
                                 other,                tags[fd],      tags[other]);
                    close_websocket(other);
                }
                else{
                    tags[other]=0;
                    close_and_track(other);
                }
            }
        }
    }
    return 0;
}
