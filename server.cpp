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

//networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <sys/time.h>

#include "Actor.hpp"
#include "WorldObject.hpp"
#include "types.hpp"

#define DGRAM_SIZE 1400 //bytes?
#define VERSION 1 // number to compare client/server versions.
//any json root must be split if bigger

enum actor_actions_t {
    START_MV_FORWARD,
    STOP_MV_FORWARD,
    START_MV_BACKWARD,
    STOP_MV_BACKWARD,
    START_MV_LEFT,
    STOP_MV_LEFT,
    START_MV_RIGHT,
    STOP_MV_RIGHT,
    START_MV_UP,
    STOP_MV_UP,
    START_MV_DOWN,
    STOP_MV_DOWN,
    JUMP
};

int createSocket(unsigned int localPort);

int main(int argc, char* args[]) {
    unsigned int listenPort = 5666;
    unsigned int maxPlayers = 16;
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
    int listenSocket = createSocket(listenPort);
    struct connection_t {
        int fd;
        unsigned int port;
        sockaddr remote;
    };
    std::vector<connection_t> freeSockets;
    for ( unsigned int i=0;i<maxPlayers;i++ ) {
        connection_t sp;
        sp.port = i+1+listenPort;
        sp.fd =  createSocket(sp.port);
        freeSockets.push_back(sp);
    }
    std::map<connection_t,Actor> players;

    /** GAME LOOP **/ {
    double tickRate = 1; //ticks per second
    double tickDuration = (1/(double)tickRate) * 1000000;
    timeval tickStart;
    gettimeofday(&tickStart,NULL);
    int waitJoin = 0;
    while (true) {
        while (true) {
            timeval now, diff;
            gettimeofday(&now,NULL);
            timersub(&now,&tickStart,&diff);
            if ( diff.tv_sec>=1 || diff.tv_usec>=tickDuration ) {
                gettimeofday(&tickStart,NULL);
                break;
            }
        }

        /** PLAYER JOINING **/ {
        waitJoin-=1;
        int flags = MSG_DONTWAIT | MSG_TRUNC; // | MSG_WAITALL;
        sockaddr srcAddr;
        socklen_t srcAddrSz = sizeof(srcAddr);
        char dgram[DGRAM_SIZE];
        memset(dgram,0,DGRAM_SIZE);
        int i=0;
        int j=0;
        errno = 0;
        if ( waitJoin > 0 ) {
            if ( (j=recvfrom(freeSockets.back().fd,(void *) &dgram,DGRAM_SIZE,flags,&srcAddr,&srcAddrSz))<0 && errno!=0 ) {

            }
        } else if ( (i=recvfrom(listenSocket,(void *) &dgram,DGRAM_SIZE,flags,&srcAddr,&srcAddrSz))<0 && errno!=0 ) {
            // always error?
            //std::cerr << "Error on listen socket: " << errno << std::endl;
            //return -1;
        }
        Json::Value joinRequest;
        Json::Reader jsonReader;
        Json::Value joinReply;
        Json::FastWriter jsonWrite;
        joinReply["accept"] = false;
        joinReply["ver"] = VERSION;
        joinReply["reason"] = "full";
        if ( freeSockets.size()>0 ) {
            joinReply["port"] = freeSockets.back().port;
            joinReply["accept"] = true;
        }
        if ( i>0 ) { // join request received
            waitJoin = tickRate; //wait a full second before forgetting player.
            jsonReader.parse(dgram,dgram+(DGRAM_SIZE-1),joinRequest,false);
            if ( joinRequest["ver"] != joinReply["ver"] ) {
                joinReply["accept"] = false;
                joinReply["reason"] = "version";
            }
            if ( !jsonReader.good() ) {
                joinReply["accept"] = false;
                joinReply["reason"] = "corrupt";
            }
            std::string data = jsonWrite.write(joinReply);
            if ( data.length()>DGRAM_SIZE ) {
                std::cerr << "Error, joinReply > DGRAM_SIZE" << std::endl;
                return -1;
            }
            if ( sendto(listenSocket,data.c_str(),data.length(),0,(sockaddr *) &srcAddr,sizeof(srcAddr))<0 ) {
                std::cerr << "Error on send join reply" << std::endl;
                return -1;
            }
        }
        if ( j>0 ) { // player joined
            // link player & socket as connection
            // check for things like prefered name, team etc
            // add player to game.
        }
        }


        /** PROCESS PLAYER ACTIONS **/
        std::vector<Actor> playerActors;
        std::vector<WorldObject> worldObjects;

        // same order as players
        std::vector<Json::Value> fromPlayers;

        // JSON structure for player sending actions to server:
        /*
        {
            "actor": [NUMOF, ACTION1, ACTION2, ACTION3],
            "connection": [NUMOF, ACTION1, ACTION2, ACTION3]
        }
        */

        for ( int i=0; i<fromPlayers.size(); i++ ) {
            //Json::Value root;
            Json::Value actor = fromPlayers.at(i)["actor"];
            for ( int j=0; j<actor.size(); j++ ) {
                WorldObject &object = worldObjects.at(playerActors.at(i).getWorldObject());
                Actor &player = playerActors.at(i);
                switch (actor[j].asInt()) {
                case START_MV_FORWARD:
                    if ( object.isGrounded() ){
                        object.setSpeed(player.getSpeed());
                        object.setDirection(FORWARD);
                    }
                    break;
                }
            }
        }

        /** DO PHYSICS/COLLISION **/
    }
    }

}

int createSocket(const unsigned int localPort) {
    int sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if ( sockfd<0 ) {
        std::cerr << "Error creating socket." << std::endl;
        return -1;
    }
    sockaddr_in lAddr;
    lAddr.sin_family = AF_INET;
    lAddr.sin_addr.s_addr = INADDR_ANY;
    lAddr.sin_port = htons(localPort);
    if ( bind(sockfd,(sockaddr *) &lAddr,sizeof(lAddr))<0 ) {
        std::cerr << "Error binding socket." << std::endl;
        return -1;
    }
    return sockfd;
}
