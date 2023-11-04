#include "MiningProto.h"
#include <errno.h>

#define IMAX_BITS(m) ((m)/((m)%255+1) / 255%255*8 + 7-86/((m)%255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)
_Static_assert((RAND_MAX & (RAND_MAX + 1u)) == 0, "RAND_MAX not a Mersenne number");

uint64_t rand64(void) {
  uint64_t r = 0;
  for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
    r <<= RAND_MAX_WIDTH;
    r ^= (unsigned) rand();
  }
  return r;
}


int sendMsg(int sockfd, const MSG *msg)
{
    size_t toSend = sizeof(Header) + msg->HDR.Payload_Size8; //deberia usar hton, hacemos asi pensando mismo endianess
    ssize_t sent;
    uint8_t *ptr = (uint8_t *) msg;

    while( toSend ) {
        sent = send(sockfd, ptr, toSend, 0);
        if( (sent == -1 && errno != EINTR) || sent == 0 )
            return sent;
        toSend -= sent;
        ptr += sent;
    }
    return 1;
}

int recvMsg(int sockfd, MSG *msg){
    size_t toRecv = sizeof(Header);
    ssize_t recvd;
    uint8_t *ptr = (uint8_t *) &msg->HDR;
    int headerRecvd = 0;

    while( toRecv ) {
        recvd = recv(sockfd, ptr, toRecv, 0);
        if( (recvd == -1 && errno != EINTR) || recvd == 0 )
            return recvd;
        toRecv -= recvd;
        ptr += recvd;
        if( toRecv == 0 && headerRecvd == 0) {
            headerRecvd = 1;
            ptr = (uint8_t *) &msg->Payload;
            toRecv = msg->HDR.Payload_Size8;
        }
    }
    return 1;    
}


Block_t CreateBlock(uint64_t prev_hash, uint64_t tx){
    Block_t block;
    block.prev_hash = prev_hash;
    block.tx = tx;
    return block;
}