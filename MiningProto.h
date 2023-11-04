#ifndef MINING_PROTO
#define MINING_PROTO

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include <pthread.h>
#define MAX_MINERS 10


///Prototipos

typedef struct __attribute__(( packed )){
    uint64_t prev_hash;
    uint64_t tx; //deberia ir pero si complica mucho podriamos sacarlo 
    uint64_t nonce;
} Block_t;

Block_t CreateBlock(uint64_t prev_hash, uint64_t tx);

uint64_t rand64(void);


struct Server_str;


typedef struct{
    uint64_t low;
    uint64_t high;
} Range;

typedef struct{
    //Estructura de un minero deberia tener: 
    uint64_t minerID;
    Range range;
    int sockfd;
    int working;
    struct Server_str* srv;
    //quiza el ultimo nonce que probo al submitear un share (para seguir desde ahi)
} Miner_t;

typedef struct Server_str{

    int serverSock;

    Miner_t miners[MAX_MINERS];
    Block_t block;
    uint8_t target;
    uint8_t diff;
    uint64_t top_range;
    uint64_t prev_hash;


    int running;
    pthread_mutex_t lock;

} Server;



inline static Block_t GetCurrentBlock(Server *srv){
    return srv->block;
}

inline static uint8_t GetCurrentTarget(Server *srv){
    return srv->target;
}

inline static Range GetNextRange(Server *srv){
    Range rng;
    static const uint64_t delta = 100000;
    if(srv->top_range > UINT64_MAX- delta ) //caso particulas me
    {                                       //estoy quedando sin nonce
        pthread_mutex_lock(&srv->lock);
        srv->top_range= (uint64_t) 0;
        rng.low = srv->top_range;
        rng.high = rng.low + UINT64_MAX- delta;
        pthread_mutex_unlock(&srv->lock);

    } else{
        pthread_mutex_lock(&srv->lock);
        rng.low = srv->top_range;
        rng.high = rng.low + delta;
        srv->top_range+= delta;
        pthread_mutex_unlock(&srv->lock);
    }
    return rng;
}


#define type_login 0
#define type_job_req 1
#define type_submit_share 2
#define type_job_end 3

#define type_login_resp (type_login|0x100)
#define type_job_resp (type_job_req|0x100)


typedef struct __attribute__(( packed )) {
    uint32_t Payload_Size8; //tamaño en bytes del payload
    uint32_t msg_type; //tipo del mensaje
} Header;

typedef struct __attribute__(( packed )) {
    uint64_t id;
}   Login;

typedef struct __attribute__(( packed )) {
    uint64_t id;
} Job_Req;

typedef struct __attribute__(( packed )) {
    uint64_t id;
    uint64_t nonce;
    //pensar un job id
} Sub_share;

typedef struct __attribute__(( packed )) {
    Block_t block;
    uint8_t target;  
    Range range;
    //job id 
} Job_Resp;

typedef struct __attribute__(( packed )) {
    uint64_t id;
} Job_End;

typedef struct __attribute__(( packed )) {
    Header HDR;

    union __attribute__(( packed )) {
        Login login;
        Job_Req Job_Req;
        Sub_share sub_share;
        Job_Resp Job_Resp;
        Job_End Job_end;
    } Payload;

}MSG;

inline static void setLoginID(MSG *msg, uint64_t id){
    msg->HDR.msg_type = type_login;
    msg->HDR.Payload_Size8 = sizeof(Login);
    msg->Payload.login.id = id;
}

inline static uint64_t getLoginID(MSG *msg){
    assert(msg->HDR.msg_type == type_login);
    return msg->Payload.login.id;
}

inline static void setJobReq(MSG *msg, uint64_t id){
    msg->HDR.msg_type = type_job_req;
    msg->HDR.Payload_Size8 = sizeof(Job_Req);
    msg->Payload.Job_Req.id = id;
}

inline static uint64_t getJobReqID(MSG *msg){
    assert(msg->HDR.msg_type == type_job_req);
    return msg->Payload.Job_Req.id;
}

inline static void setSubShare(MSG *msg, uint64_t id, uint64_t nonce){
    msg->HDR.msg_type = type_submit_share;
    msg->HDR.Payload_Size8 = sizeof(Sub_share);
    msg->Payload.sub_share.id = id;
    msg->Payload.sub_share.nonce = nonce;
}

inline static uint64_t getSubShareID(MSG *msg){
    assert(msg->HDR.msg_type == type_submit_share);
    return msg->Payload.sub_share.id;
}

inline static uint64_t getSubShareNonce(MSG *msg){
    assert(msg->HDR.msg_type == type_submit_share);
    return msg->Payload.sub_share.nonce;
}

inline static void setJobResp(MSG *msg, Block_t block, uint8_t target, Range range){
    msg->HDR.msg_type = type_job_resp;
    msg->HDR.Payload_Size8 = sizeof(Job_Resp); //deberia usar hton, hacemos asi pensando mismo endianess
    msg->Payload.Job_Resp.block = block;
    msg->Payload.Job_Resp.target = target;
    msg->Payload.Job_Resp.range = range;  //esto esta bien? Hago que el range apunte al vector range definido
}

inline static Block_t getJobRespBlock(MSG *msg){
    assert(msg->HDR.msg_type == type_job_resp);
    return msg->Payload.Job_Resp.block;
}

inline static uint8_t getJobRespTarget(MSG *msg){
    assert(msg->HDR.msg_type == type_job_resp);
    return msg->Payload.Job_Resp.target;
}

inline static Range getJobRespRange(MSG *msg){
    assert(msg->HDR.msg_type == type_job_resp);
    return msg->Payload.Job_Resp.range;
}

inline static void setJobEndID(MSG *msg, uint64_t id){
    msg->HDR.msg_type = type_job_end;
    msg->HDR.Payload_Size8 = sizeof(Job_End);
    msg->Payload.Job_end.id = id;
}

inline static uint64_t getJobEndID(MSG *msg){
    assert(msg->HDR.msg_type == type_job_end);
    return msg->Payload.Job_end.id;
}

int sendMsg(int sockfd, const MSG *msg);
int recvMsg(int sockfd, MSG *msg);



///////////////////////////////////////////////
////SPAM (Ideas de funciones a implementar)////
///////////////////////////////////////////////
/*
void* MinerHello(){
    //Funcion que corre el minero para empezar a minar
    //Debe avisar al pool op quien es, y pedirle un bloque y target
    //"Hola Pool op, aca estoy soy MinerID, mandame laburo"
}

void* PoolOpHello(){
    //Funcion que usa el Pool op para aceptar un nuevo minero
    //Debe confirmar que recibio la info y pasarle un bloque y target al minero
    //"Hola MinerID, toma este bloque, proba los nonce entre [min, max], y avisame si encontrar algo con target 0 al comienzo"
}

void* JobRequest(){

}

void* JobResponse(){

}

void* SendWork(){
    //El Pool op manda un nuevo bloque y target a los mineros cuando alguno encontro la solucion
    //Problema: como les aviso que paren lo que estan haciendo? les tiro un signal? 
    //"Che les mando otro laburo porque alguien ya encontro la solucion"
}

void* Mine(Block_t Block, uint64_t target, uint64_t range[2]){
    //Funcion que corre el minero para minar
    //Toma el bloque
    //elige un nonce dentro del rango
    //prueba el hash
    //verifica si cumple el target
    //si lo verifica submitea un share (guarda el nonce por si hay que seguir minando)
    //sino prueba nonce+1
}

void* SubmitShare(){
    //Funcion que usa el minero para mandar un share al pool op
    //"Che Pool Op habla MinerID, este nonce cumplio el target, anota que labure"
}

void* PayMiner(){
    //Funcion que usa el pool op para pagar a los mineros basado en PPS o PPLNS
    //"Bueno MinerID, anote que mandaste X shares asi que te pago tanto"
}

//Opcionales por ahora
void* PoolStatistics(){
    //Cuanto tiempo tardamos en encontrar un bloque, que minero anda mejor
}
*/

#endif //MNING_PROTO