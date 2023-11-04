#include <stdio.h>
#include <stdlib.h>
#include "MiningProto.h"
#include "miner.h"


int main(int argc, char** argv){
    
    Miner_t miner;

    miner.minerID = atoi(argv[1]);

    if (argc < 2) {
         fprintf(stderr,"ERROR, no minerID provided\n");
         exit(1);
     }

    Prepare(&miner);

    RunMiner(&miner);

    return 0;

}