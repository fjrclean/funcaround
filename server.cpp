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
      unsigned int time_since_last; // since last received, as game ticks
      bool received_msg;
    };
    struct player_socket_t {
        int fd;
        unsigned int port;
      char *playerName; // change to array like msg
      char msg[DGRAM_SIZE];
      player_t player; // this should be cleared each time connection becomes free
      player_socket_t *next;
      player_socket_t *prev;
    };
    player_socket_t *freePlayerSocket = NULL;
    // create a linked list of available sockets for players.
    for ( unsigned int i=0;i<maxPlayers;i++ ) {
      player_socket_t *socket = (player_socket_t*)malloc(sizeof(socket));
      socket->port = i+1+listenPort;
      socket->fd =  shared::createSocket(socket->port);
      socket->playerName = (char*)malloc(32);
      socket->prev = NULL;
      if ( freePlayerSocket!=NULL )
	freePlayerSocket->prev = socket;
      socket->next = freePlayerSocket;
      memset((void*)&socket->player,0,sizeof(player_t));
      freePlayerSocket = socket;
      shared::log(globVerb,3,"player socket created: fd:%d port:%u",freePlayerSocket->fd,freePlayerSocket->port);
    }
    player_socket_t *usedPlayerSocket = NULL;
    unsigned int usedPlayerCount = 0;
    unsigned int freePlayerCount = 0;
    // I think MSG_DONTWAIT still ensures if it reads anything, it will read the whole dgram.
    // So no possibility of reading incomplete messages unless they are too large and split into multiple dgrams?
    const int recv_flags = MSG_DONTWAIT | MSG_TRUNC;// | MSG_WAITALL;
    std::vector<Json::Value> jsonFromPlayers;
    
    /** GAME LOOP **/ {
    timeval tickStart;
    gettimeofday(&tickStart,NULL);
    while ( true ) {
      player_socket_t *socket = usedPlayerSocket;
      for ( int i=1; i<=usedPlayerCount; i++ ) {
	socket->player.received_msg=false;
	socket->player.time_since_last+=1;
	if ( (socket->player.time_since_last/ticksPerSec)>WAIT_PLAYER_MIA ) { // drop player
	  if ( socket->next!=NULL )
	    socket->next->prev = socket->prev;
	  if ( socket->prev!=NULL )
	    socket->prev->next = socket->next;
	  usedPlayerSocket = socket->next;
	  usedPlayerCount--;
	  i--;
	  if ( freePlayerSocket!=NULL ) 
	    freePlayerSocket->prev = socket;
	  freePlayerSocket = socket;
	  memset((void*)freePlayerSocket->playerName,0,32);
	  memset((void*)&freePlayerSocket->player,0,sizeof(player_t));	    
	  // remove their actor from game too (with necessary cleanup)
	  // perhaps send message to client, saying they have been dropped, reasons
	  shared::log(globVerb,1,"Player %s left. Reason: lost sync too long.",socket->playerName);
	  continue;	  
	}
      }
      /** WAIT LOOP **/
      do { // do wait loop at least once, and then additionally until next tick
	/** RECEIVE NET MSG FROM PLAYER **/ {
	  jsonFromPlayers.clear();
	  socket = usedPlayerSocket;
	  for ( int i=0; i<usedPlayerCount; i++ ) {
	    if ( socket->player.received_msg==true )
	      continue;
	    sockaddr_in srcAddr;
	    socklen_t srcAddrSz = sizeof(srcAddr);
	    memset(&srcAddr,0,srcAddrSz);
	    char dgram[DGRAM_SIZE];
	    memset(socket->msg,0,DGRAM_SIZE);
	    int j;
	    if ( (j=recvfrom(socket->fd,(void*)&socket->msg,DGRAM_SIZE,recv_flags,(sockaddr*)&srcAddr,&srcAddrSz))<0
		 && errno!=0 && errno!=EAGAIN && errno!=EWOULDBLOCK ) { // with MSG_DONTWAIT errors are returned if would have to block
	      shared::log(globVerb,LOG_ERROR,"Receive from player socket");
	      return -1;
	    }
	    Json::Value receive;
	    Json::Reader jsonReader;
	    Json::Value send;
	    Json::FastWriter jsonWriter;
	    if ( j>0 && srcAddr.sin_port==socket->player.remote.sin_port && srcAddr.sin_addr.s_addr==socket->player.remote.sin_addr.s_addr ) {
	      jsonReader.parse(socket->msg,socket->msg+(DGRAM_SIZE-1),receive,false);
	    }
	    std::cout << socket->player.time_since_last << ":" << jsonReader.good() << std::endl;
	    if ( !jsonReader.good() || j<=0 ) { 
	      // player data corrupted, add notice of this to game state sent to this player
	      continue;
	    }
	    socket->player.received_msg = true;
	    std::cout << "received" << std::endl;
	    //	    receive["actor"] = playerSck->player.actorId;
	    receive["id"] = i;
	    jsonFromPlayers.push_back(receive);
	    socket->player.time_since_last=0;
	  }
	}

      } while ( !shared::tickStart(ticksPerSec,&tickStart) );

      /** END WAIT LOOP **/
		

        /** PROCESS PLAYER ACTIONS **/ {
        for ( int i=0; i<jsonFromPlayers.size(); i++ ) {
	  int id = jsonFromPlayers.at(i)["id"].asInt();
	  switch (jsonFromPlayers.at(i)["cnm"].asInt()) {
	  case CMD_PLAYER_NAME:
	    //strcpy(usedPlayerSockets.at(id)->playerName,jsonFromPlayers.at(i)["cvl"].asCString());
	    //shared::log(globVerb,3,"player id:%d set name to:%s",jsonFromPlayers.at(i)["id"].asInt(),usedPlayerSockets.at(id)->playerName);
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
        if ( freePlayerSocket!=NULL ) {
            send["acc"] = true;
	    send["prt"] = freePlayerSocket->port;
	}
	// we can set name and join preferences like team here
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
	    player_socket_t *socket = freePlayerSocket;
	    if ( usedPlayerSocket!=NULL )
	      usedPlayerSocket->prev = socket;
	    freePlayerSocket = freePlayerSocket->next;
	    if ( freePlayerSocket!=NULL )
	      freePlayerSocket->prev = NULL;
	    socket->next = usedPlayerSocket;
	    usedPlayerSocket = socket;
	    usedPlayerSocket->player.remote = srcAddr;
	    usedPlayerCount++;
	    std::cout << "player " << usedPlayerCount << " joined on port " << usedPlayerSocket->port << std::endl;
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


