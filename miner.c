#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <errno.h>
#include "MiningProto.h"

#include <openssl/sha.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

unsigned f(unsigned char *bytes, unsigned int n) {
    unsigned int idx = 0;
    while( (idx < (n<<3)) && (bytes[idx/8] & (1 << (7-(idx%8)))) == 0)
    idx++;
    return idx;
}

void hexdump(const unsigned char *p, size_t n) {
    for(size_t i = 0; i <n; ++i)
        printf("%02x ", p[i]);
    putchar('\n');
}

void Prepare(Miner_t *pm){ //devuelve un minero conectado
    pm->working = 1;
    //pm->minerID = 1; //como defino un miner id para cada minero?
    int portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = 1233; //hardcodeado el puerto por ahora
    pm->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (pm->sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname("localhost"); //server address harcodeada por ahora
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(pm->sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

}


void mine(MSG *msg, Miner_t* pm){
    Block_t block = getJobRespBlock(msg);
    uint8_t target = getJobRespTarget(msg);
    Range range = getJobRespRange(msg);
    MSG msg_out;

    printf("Entrando a minar\n");

    char digest[SHA256_DIGEST_LENGTH];

    int i;
    for(i=range.low; i<range.high; i++)
    {
        block.nonce = i;

        SHA256((unsigned char *)&block, sizeof(Block_t), (unsigned char*)digest);
        unsigned idx = f(digest, sizeof(digest));
        if(idx >= target)
        { //si entra aca es porque cumplio el target
            setSubShare(&msg_out, pm->minerID, i);
            if(sendMsg(pm->sockfd, &msg_out) != 1 )
                error("Enviando SubShare de mine()");
        printf("Encontre un nonce!! Es: %d \n", i);
        hexdump(digest, 8);
        }
        //llegar al bit dentro de un str : byte: i/8, bit i%8
    }
    printf("Pase por todos los nonce\n");
    setJobEndID(&msg_out, pm->minerID);
    if(sendMsg(pm->sockfd, &msg_out) != 1 )
            error("Enviando JobEnd de mine()");
}

void RunMiner(Miner_t *pm){
MSG msg;
int recvd; 

//primero nos logeamos antes del while(1)
setLoginID(&msg, pm->minerID);
if(sendMsg(pm->sockfd, &msg) != 1 )
        error("Enviando Login MSG en RunMiner");

while(1){
    recvd = recvMsg(pm->sockfd, &msg);
    if((recvd == -1 && errno != EINTR) || recvd == 0 )
            perror("Recv MSG de Run Miner en server");
    switch (msg.HDR.msg_type)
    {
    case type_job_resp:
        mine(&msg, pm); 
        break;
    default:
        printf("Me llego un mensaje desconocido\n");
        break;
    }
}

}

