#ifndef SERVER_H_
#define SERVER_H_
#include "MiningProto.h"

void error(const char *msg);

void InitMiners(Server *srv);

void Prepare(Server *srv);

void RunServer(Server *srv);


#endif //SERVER_H_