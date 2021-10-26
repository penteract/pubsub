char b64[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
               "abcdefghijklmnopqrstuvwxyz"
               "0123456789+/";

unsigned char b64inv[256];

char* b64e(unsigned char* in, unsigned char* out, int len){
    int c=0;
    int tmp=0;
    while(len--){
        tmp |= *(in++);
        if((++c)%3==0){
            for(int i=0;i<4;i++){
                *(out++)=b64[(tmp>>18)&0x3F];
                tmp<<=6;
            }
            tmp=0;
        }
        else tmp<<=8;
    }
    int k = c%3;
    if(k){
        tmp<<=(2-k)*8;
        for(int i=0;i<4;i++){
            *(out++) = i>k?'=':b64[(tmp>>18)&0x3F];
            tmp<<=6;
        }
    }
    return out;
}

unsigned char* b64d(unsigned char* in, unsigned char* out, int len){
    int lenin = (len/3)*4 + ((len%3)*3+1)/2;
    int tmp=0;
    int c=0;
    while(b64inv[*in]!=0xFF && c<lenin){
        tmp |= b64inv[*(in++)];
        if(++c%4==0){
            for(int i=0;i<3;i++){
                *(out++)=(tmp>>16)&0xFF;
                tmp<<=8;
            }
            tmp=0;
        }
        else tmp<<=6;
    }
    int k = c%4;
    if(k){
        tmp<<=(3-k)*6;
        for(int i=0;i<k-1;i++){
            *(out++)=(tmp>>16)&0xFF;
            tmp<<=8;
        }
    }
    return in;
}

void b64setup(){
    for(int i=0;i<256;i++)
        b64inv[i]=0xFF;
    for(char i=0;i<64;i++)
        b64inv[b64[i]]=i;
        /*TESTING
    char s[100] = "XXXXXXXXXX";
    int x = b64d("UVdFUlQ=",s,5);
//    s[x]='\0';
    printf("%d %s\n",x, s);
    exit(0);*/
}

unsigned int w[80]; // working space
char result[35];
// Take a sequence of at most 2^32-1 bits of length ml
// and put the base64 encoding of the SHA-1 hash into result
void sha1(char* bytes, unsigned int ml, char* out){
    printf("hashing %.*s\n",ml,bytes);
unsigned int h0 = 0x67452301;
unsigned int h1 = 0xEFCDAB89;
unsigned int h2 = 0x98BADCFE;
unsigned int h3 = 0x10325476;
unsigned int h4 = 0xC3D2E1F0;
    unsigned int n=0;
    while(n < (ml+1+8)){
        // write message into w and do pre-processing
        for(int k=0; k<16; k++){
            w[k]=0;
            for(int j=0;j<4;j++){
                w[k]<<=8;
                if(n<ml) w[k] += bytes[n];
                if(n==ml) w[k] += 0x80;
                n+=1;
            }
            if(k==15 && n >= ml+1+8) w[k]=ml<<3; //message length in bits
        }
        //for(int k=0;k<16;k++) printf("%08x ",w[k]);
        //printf("\n");
        int i;
        for(i=16; i<80;i++){
            w[i] = (w[i-3]^w[i-8]^w[i-14]^w[i-16]);
            w[i] = (w[i]<<1) | (w[i]>>31);
        }
        unsigned int a=h0;
        unsigned int b=h1;
        unsigned int c=h2;
        unsigned int d=h3;
        unsigned int e=h4;
        unsigned int f,k, temp;
        for(i=0;i<80;i++){
            if(i<20){
                f = (b&c)|((~b)&d);
                k = 0x5A827999;
            }
            else if (i<40){
                f = b^c^d;
                k = 0x6ED9EBA1;
            }
            else if (i<60){
                f = (b&c)|(b&d)|(c&d);
                k = 0x8F1BBCDC;
            }
            else{
                f = b^c^d;
                k = 0xCA62C1D6;
            }
            temp = ((a<<5)| (a>>27))+f+e+k+w[i];
            e = d;
            d = c;
            c = (b<<30) | (b>>2);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }
    w[0]=h0;
    w[1]=h1;
    w[2]=h2;
    w[3]=h3;
    w[4]=h4;
    for(int k=0;k<5;k++){
        result[k*4+0] = (char)(w[k]>>24);
        result[k*4+1] = (char)((w[k]>>16)&255);
        result[k*4+2] = (char)((w[k]>>8)&255);
        result[k*4+3] = (char)(w[k]&255);
    }
    //for(int k=0;k<20;k++) printf("%hhx",result[k]);
    //printf("\n");
    b64e(result, out, 20);
}





















