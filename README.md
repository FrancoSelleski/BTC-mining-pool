# BTC-mining-pool

In this repo I share a project I developed for a prototype of a **Bitcoin mining pool protocol** using network programming in **C using web sockets**. This project was develope for a class in network programming. 

It contains a **server that handles connection/communication to mining processes active in the pool, as well as this mining processes**. Each miner tries different nonce until obtaining SHA defined by difficulty target of the network.

It's a prototype in the sense that it takes care of connection and communication between server and miners, but in the messages there are not valid tx yet. Only checked value is nonce and SHA of block to validate it complies with network difficulty target. 

The server takes care of logging in new miners, sending them new jobs upon request, receiving share of proposed solutions and validating, as well as ending jobs and connections. 

There is a pdf attached, which contains presentation of dissertation given about this project in the network programming class.