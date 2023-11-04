#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include "MiningProto.h"
#include <openssl/sha.h>

#define TRUE 1
#define FALSE 0
#define TARGET 16
#define DIFF 24

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

void InitMiners(Server *srv) {
    int i;
    for(i = 0; i < MAX_MINERS ; i++) {

        srv->miners[i].working = 0;
        srv->miners[i].srv = srv; 
    }
}

void Prepare(Server *srv)
{  
    printf("Entrando a Prepare\n");
    socklen_t clilen;
    int val = 1; 
    struct sockaddr_in serv_addr;

    srv->serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (srv->serverSock < 0) 
    error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(1233);
    setsockopt(srv->serverSock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (bind(srv->serverSock, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0) 
            error("ERROR on binding");
    listen(srv->serverSock,5);
    printf("Despues del listen\n");
    InitMiners(srv);
    printf("Despues del InitMiners\n");
    if(pthread_mutex_init(&srv->lock, NULL) != 0)
        error("Creando el mutex");
    
    srv->block = CreateBlock(0, rand64());
    srv->diff = DIFF;
    srv->target = TARGET; //los shares tiene 3 ceros al comienzo
    srv->top_range = (uint64_t) 0;
    srv->prev_hash = 0;
    printf("Termino Prepare\n");  

}

Miner_t *AllocateMiner(Server *srv, int fd){
    int i;
    for(i = 0; i < MAX_MINERS; i++) {

        if(srv->miners[i].working == 0) {
            
            srv->miners[i].working = 1;

            srv->miners[i].sockfd = fd;

            return &srv->miners[i];

        }

    }

    return NULL;

}

void LoginProcess(Miner_t * pm, MSG* msg){
    MSG msg_out;
    pm->minerID = getLoginID(msg);
    printf("Me llego un login id\n");   
    setJobResp(&msg_out, GetCurrentBlock(pm->srv), GetCurrentTarget(pm->srv), GetNextRange(pm->srv));
    if(sendMsg(pm->sockfd, &msg_out) != 1 )
        error("Enviando job Resp de Login\n");
}

void JobReqProcess(Miner_t * pm){ 
    MSG msg_out;
    setJobResp(&msg_out, GetCurrentBlock(pm->srv), GetCurrentTarget(pm->srv), GetNextRange(pm->srv));
    if(sendMsg(pm->sockfd, &msg_out) != 1 )
        error("Enviando job Resp de Job Req\n");
}

void SubShareProcess(MSG *msg, Miner_t *pm){
    printf("Me llego un nonce de: %lu\n", pm->minerID);
    assert(pm->minerID == getSubShareID(msg));
    uint64_t nonce = getSubShareNonce(msg);
    pm->srv->block.nonce = nonce;

    char digest[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)&pm->srv->block, sizeof(Block_t), (unsigned char*)digest);
    unsigned idx = f(digest, sizeof(digest));

    if(idx >= pm->srv->diff) //si me llego el nonce bueno
    {
        pthread_mutex_lock(&pm->srv->lock);
        pm->srv->prev_hash = (uint64_t) digest; //actualizo el prev hash
        pm->srv->block= CreateBlock(pm->srv->prev_hash, rand64()); //creo un nuevo bloque
        pm->srv->top_range = (uint64_t) 0;
        pthread_mutex_unlock(&pm->srv->lock);
        printf("Llego el nonce bueno nene!\n");
        hexdump(digest, 8);

    }
    
}

void JobEndProcess(Miner_t *pm){
    MSG msg_out;
    printf("Me llego un JobEnd de : %lu\n", pm->minerID);
    setJobResp(&msg_out, GetCurrentBlock(pm->srv), GetCurrentTarget(pm->srv), GetNextRange(pm->srv));
    if(sendMsg(pm->sockfd, &msg_out) != 1 )
        error("Enviando job Resp de Job End\n");
}

//Funcion que gestiona al minero en el server, lo escucha, le manda laburo
void* RunMiner(void* pm){ 
    Miner_t* miner = (Miner_t *) pm;
    MSG msg;
    int recvd; 
    while(1){
        recvd = recvMsg(miner->sockfd, &msg);
        if((recvd == -1 && errno != EINTR) || recvd == 0 )
            perror("Recv MSG de Run Miner en server\n");
        switch (msg.HDR.msg_type) //endian
        {
            case type_login:
            LoginProcess(miner, &msg);
                break;
            case type_job_req:
                JobReqProcess(miner);
                break;
            case type_submit_share:
                SubShareProcess(&msg, miner);
                break;
            case type_job_end:
                JobEndProcess(miner);
                break;
            default:
                printf("Tipo de mensaje desconocido\n");
                break;
        }
    }

}

void RunServer(Server *srv){

    Miner_t *pm;
    srv->running = TRUE;
    socklen_t clilen;
    int newsockfd;
    pthread_t tid[10];
    int cont = 0;
    printf("Antes del while srv->running\n");
    while(srv->running) {
        struct sockaddr_in cli_addr;
        
        clilen = sizeof(cli_addr);
        printf("Antes del accept\n");
        newsockfd = accept(srv->serverSock, 
                    (struct sockaddr *) &cli_addr, 
                    &clilen);
        printf("Despues del accept\n");
        if (newsockfd < 0) 
            error("ERROR on accept");
        
        pm = AllocateMiner(srv, newsockfd);

        if(pthread_create(&tid[cont++ % MAX_MINERS], NULL, RunMiner, (void *) pm)){
            perror("Creando thread...\n");
        
        } /*else {

            printf("too many clients, ignoring connection\n");

        }*/

    }

}



