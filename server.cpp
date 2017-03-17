#include <json/json.h>
#include <json/reader.h>
#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <sstream>
#include <cerrno>
#include <cstring>

#include <sys/time.h>

#include "Actor.hpp"
#include "WorldObject.hpp"
#include "types.hpp"
#include "network.hpp"



int createSocket(unsigned int localPort);

int main(int argc, char* args[]) {
    /** GAME OPTIONS **/
    unsigned int listenPort = 5666;
    unsigned int maxPlayers = 3;
    std::string mapName;
    for ( int i=0; i<argc; i++ ) {
        if ( args[i]=="--port" ) {
            i+=1;
            listenPort = std::atoi(args[i]);
        } else if ( args[i]=="--map" ) {
            i+=1;
            mapName = std::string(args[i]);
        }
    }

    /** NETWORK SETUP **/
    int listenSocket = network::createSocket(listenPort);
    struct connection_t {
        int fd;
        unsigned int port;
        sockaddr_in remote;
        unsigned int playerActor;
        char *playerName;
    };
    std::vector<connection_t> freeSockets;
    for ( unsigned int i=0;i<maxPlayers;i++ ) {
        connection_t sp;
        sp.port = i+1+listenPort;
        sp.fd =  network::createSocket(sp.port);
        freeSockets.push_back(sp);
    }
    std::vector<connection_t> usedSockets;


    /** MAP SETUP **/
    std::vector<Actor> actors;

    /** GAME LOOP **/ {
    double tickRate = 1; //ticks per second
    double tickDuration = (1/(double)tickRate) * 1000000;
    timeval tickStart;
    gettimeofday(&tickStart,NULL);
    int waitJoin = 0;
    int listenfd = listenSocket;
    while (true) {
        while (true) { // pause game loop until time for next tick.
            timeval now, diff;
            gettimeofday(&now,NULL);
            timersub(&now,&tickStart,&diff);
            if ( diff.tv_sec>=1 || diff.tv_usec>=tickDuration ) {
                gettimeofday(&tickStart,NULL);
                break;
            }
        }

        /** PROCESS PLAYER ACTIONS **/ {
        std::vector<Json::Value> fromPlayers;
        for ( int i=0; i<fromPlayers.size(); i++ ) {
            Actor &actor = actors.at(fromPlayers.at(i)["actor"]);
            Json::Value actions = fromPlayers.at(i)["actions"];
            if ( !actions.isUInt64() ) {
                ///TODO: if too many corrupt actions from client, take action, eg notify client
                continue;
            }
            actor.actionsDo = (actions.asUInt64() & ~(actor.actionsPass));
            uint64_t actionsPass = (actions.asUInt64() & actor.actionsPass);
            uint64_t bitValue = 1;
            for ( int j=0;j<64;j++ ) {
                if ( bitValue&actionsPass!=0 ) {
                    actor.passActionsTo[j]->actionsDo |= actor.passActionsTo[j]->actions & bitValue;
                    if ( !actor.passActionsTo[j]->actions&bitValue )
                        actor.actionsDo |= bitValue;
                }
                bitValue *= 2;
            }
        }
        }

        /** PLAYER JOINING **/ {
        waitJoin-=1;
        int flags = MSG_DONTWAIT | MSG_TRUNC | MSG_WAITALL;
        sockaddr_in srcAddr;
        socklen_t srcAddrSz = sizeof(srcAddr);
        char dgram[DGRAM_SIZE];
        memset(dgram,0,DGRAM_SIZE);
        int i=0;
        errno = 0;
        if ( (i=recvfrom(listenfd,(void *) &dgram,DGRAM_SIZE,flags,(sockaddr*)&srcAddr,&srcAddrSz))<0 && errno!=0 ) {
            // always error?
            //std::cerr << "Error on listen socket: " << errno << std::endl;
            //return -1;
        }
        Json::Value receive;
        Json::Reader jsonReader;
        Json::Value send;
        Json::FastWriter jsonWriter;
        send["acc"] = false;
        send["rea"] = "full";
        send["prt"] = freeSockets.back().port;
        send["ord"] = receive["ord"].asUInt()+=1;
        if ( freeSockets.size()>0 )
            send["acc"] = true;
        jsonReader.parse(dgram,dgram+(DGRAM_SIZE-1),receive,false);
        if ( waitJoin>0 && freeSockets.back().remote.sin_addr.s_addr==srcAddr.sin_addr.s_addr && i>0 ) { // player joined
            usedSockets.push_back(freeSockets.back());
            freeSockets.pop_back();
            usedSockets.back().playerName = new char[32];
            unsigned int length = receive["nam"].asString().size();
            if ( length>31 )
                length = 31;
            if ( receive["nam"].isString() )
                std::memcpy(usedSockets.back().playerName,receive["nam"].asString().c_str(),length+1);
            usedSockets.back().playerActor = actors.size();
            actors.push_back(Actor());
        }
        if ( waitJoin<=0 && i>0 ) { // join request d
            if ( receive["ver"] != VERSION ) {
                send["acc"] = false;
                send["rea"] = "version";
            }
            if ( !jsonReader.good() ) {
                send["acc"] = false;
                send["rea"] = "corrupt";
            }
            std::string data = jsonWriter.write(send);
            if ( data.length()>DGRAM_SIZE ) {
                std::cerr << "Error, send > DGRAM_SIZE" << std::endl;
                return -1;
            }
            if ( sendto(listenSocket,data.c_str(),data.length(),0,(sockaddr *) &srcAddr,sizeof(srcAddr))<0 ) {
                std::cerr << "Error on send join reply" << std::endl;
                return -1;
            }
            freeSockets.back().remote = srcAddr;
            listenfd = listenSocket;
            if ( send["acc"]==true ) {
                waitJoin = tickRate+5; //wait a full second before forgetting player.
                listenfd = freeSockets.back().fd;
            }
        }

        }




        /** DO PHYSICS/COLLISION **/ {

        }

        /** SEND UPDATES TO PLAYERS **/ {

        }
    }
    }

}
