#include <stdio.h>
#include "MiningProto.h"
#include "server.h"

int main(){
    Server srv;
    
    Prepare(&srv);

    RunServer(&srv); 

    return 0;
}