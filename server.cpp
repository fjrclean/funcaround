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

#include "types.hpp"
#include "shared.hpp"

#define WAIT_PLAYER_MIA 5 // time to wait, in seconds, before dropping player who has sent no legible data

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
    int globVerb=3;
    double ticksPerSec=1;
    
    /** NETWORK SETUP **/
    const int listenSocket = shared::createSocket(listenPort);
    struct player_t {
      sockaddr_in remote;
      unsigned int actorId;
      char *name;
      unsigned int time_since_last; // since last received, as game ticks
    };
    struct connection_t {
        int fd;
        unsigned int port;
      player_t player; // this should be cleared each time connection becomes free
    };
    connection_t *playerSockets = (connection_t*)malloc(sizeof(connection_t)*maxPlayers);
    std::vector<connection_t*> freePlayerSockets;
    for ( unsigned int i=0;i<maxPlayers;i++ ) {
        playerSockets[i].port = i+1+listenPort;
        playerSockets[i].fd =  shared::createSocket(playerSockets[i].port);
	memset((void*)&playerSockets[i].player,0,sizeof(player_t));
        freePlayerSockets.push_back(&playerSockets[i]);
	shared::log(globVerb,3,"player socket created: fd:%d port:%u",freePlayerSockets.back()->fd,freePlayerSockets.back()->port);
    }
    std::vector<connection_t*> usedPlayerSockets;
    const int recv_flags = MSG_DONTWAIT | MSG_TRUNC;// | MSG_WAITALL;
    std::vector<Json::Value> jsonFromPlayers;
    
    /** GAME LOOP **/ {
    timeval tickStart;
    gettimeofday(&tickStart,NULL);
    while (true) {
      while (!shared::tickStart(ticksPerSec,&tickStart)) { // pause game loop until time for next tick.
	  
      }
		
	/** RECEIVE FROM PLAYER **/ {
	jsonFromPlayers.clear();
	  for ( int i=0; i<usedPlayerSockets.size(); i++ ) {
	  connection_t *playerSck = usedPlayerSockets.at(i);
	  if ( (playerSck->player.time_since_last/ticksPerSec)>WAIT_PLAYER_MIA ) {
	    free((void*)playerSck->player.name);
	    freePlayerSockets.push_back(usedPlayerSockets.at(i));
	    usedPlayerSockets.erase(usedPlayerSockets.begin()+i); // drop player
	    memset((void*)&freePlayerSockets.back()->player,0,sizeof(player_t));	    
       	    // remove their actor from game too (with necessary cleanup)
	    // perhaps send message to client, saying they have been dropped, reasons
	    std::cout << "player " << i << " dropped. reason: mia" << std::endl;
	    i-=1;
	    continue;
	  }
	  sockaddr_in srcAddr;
	  socklen_t srcAddrSz = sizeof(srcAddr);
	  memset(&srcAddr,0,srcAddrSz);
	  char dgram[DGRAM_SIZE];
	  memset(&dgram,0,DGRAM_SIZE);
	  int j;
	  if ( (j=recvfrom(playerSck->fd,(void *) &dgram,DGRAM_SIZE,recv_flags,(sockaddr*)&srcAddr,&srcAddrSz))<0 && errno!=0 ) {
            // always error?
            //std::cerr << "Error on listen socket: " << errno << std::endl;
            //return -1;
	  }
	  Json::Value receive;
	  Json::Reader jsonReader;
	  Json::Value send;
	  Json::FastWriter jsonWriter;
	  if ( j>0 && srcAddr.sin_port==playerSck->player.remote.sin_port && srcAddr.sin_addr.s_addr==playerSck->player.remote.sin_addr.s_addr ) {
	    jsonReader.parse(dgram,dgram+(DGRAM_SIZE-1),receive,false);
	  }
	  playerSck->player.time_since_last+=1;
	  std::cout << playerSck->player.time_since_last << ":" << jsonReader.good() << std::endl;
	  if ( !jsonReader.good() || j<=0 ) { 
	    // player data corrupted, add notice of this to game state sent to this player
	    continue;
	  }
	  std::cout << "received" << std::endl;
	  receive["actor"] = playerSck->player.actorId;
	  receive["id"] = i;
	  jsonFromPlayers.push_back(receive);
	  playerSck->player.time_since_last=0;
	  }
	}

        /** PROCESS PLAYER ACTIONS **/ {
        for ( int i=0; i<jsonFromPlayers.size(); i++ ) {
	  int id = jsonFromPlayers.at(i)["id"].asInt();
	  switch (jsonFromPlayers.at(i)["cnm"].asInt()) {
	  case CMD_PLAYER_NAME:
	    if( (void*)usedPlayerSockets.at(id)->player.name != NULL )
	      usedPlayerSockets.at(id)->player.name = (char*)malloc(32);
	    memset((void*)usedPlayerSockets.at(id)->player.name,0,32);
	    strcpy(usedPlayerSockets.at(id)->player.name,jsonFromPlayers.at(i)["cvl"].asCString());
	    shared::log(globVerb,3,"player id:%d set name to:%s",jsonFromPlayers.at(i)["id"].asInt(),usedPlayerSockets.at(id)->player.name);
	    break;
	  default:
	    break;
	  }
	  /* Actor &actor = actors.at(fromPlayers.at(i)["actor"]);
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
		}*/
	    }
        }

        /** PLAYER JOINING **/ {
        sockaddr_in srcAddr;
        socklen_t srcAddrSz = sizeof(srcAddr);
        char dgram[DGRAM_SIZE];
        memset(dgram,0,DGRAM_SIZE);
        errno = 0;
	int i=0;
        if ( (i=recvfrom(listenSocket,(void *) &dgram,DGRAM_SIZE,recv_flags,(sockaddr*)&srcAddr,&srcAddrSz))<0 && errno!=0 ) {
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
        if ( freePlayerSockets.size()>0 ) {
            send["acc"] = true;
	    send["prt"] = freePlayerSockets.back()->port;
	}
        jsonReader.parse(dgram,dgram+(DGRAM_SIZE-1),receive,false);
	if ( receive["ver"] != VERSION ) {
	  send["acc"] = false;
	  send["rea"] = "version";
	}
	if ( !jsonReader.good() ) {
	  send["acc"] = false;
	  send["rea"] = "corrupt";
	}
	if ( i> 0 ) {
	  std::string data = jsonWriter.write(send);
	  if ( data.length()>DGRAM_SIZE ) {
	    std::cerr << "Error, send > DGRAM_SIZE" << std::endl;
	    return -1;
	  }
	  if ( sendto(listenSocket,data.c_str(),data.length(),0,(sockaddr *) &srcAddr,sizeof(srcAddr))<0 ) {
	    std::cerr << "Error on send join reply" << std::endl;
	    return -1;
	  }
	  std::cout << "sent reply " << send.get("acc","error").asString() << std::endl;
	  if ( send.get("acc","error").asBool() == true ) {
	    usedPlayerSockets.push_back(freePlayerSockets.back());
	    freePlayerSockets.pop_back();
	    usedPlayerSockets.back()->player.remote = srcAddr;
	    std::cout << "player " << usedPlayerSockets.size() << " joined on port " << usedPlayerSockets.back()->port << std::endl;
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


