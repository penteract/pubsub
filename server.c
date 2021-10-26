char resp404[] = "HTTP/1.1 404 Not Found\r\n"
"Content-Length: 14\r\n"
"\r\n"
"Page not found";

char request[2049];

void snd404(int sock){
    snd(sock, resp404);
}

int process (int sock){
    int c = recv(sock, request, 2048, 0);
    if(c<=0){ return -1;}
    request[c]='\0';
    if(memcmp(request,"GET ",4)){
        snd404(sock);
        return -1;
    }
    if(memcmp(request+4,"/connect/",9)){
        snd404(sock);
        return -1;
    }
    return ws_init(sock, request+4+9);
}
