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
#include <assert.h>
#include <assert.h>

#include <sys/time.h>

#include "types.hpp"
#include "shared.hpp"
#include "actors.hpp"

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
    struct player_unique {
      sockaddr_in remote;
      unsigned int actorId;
      unsigned int ticksSinceLast; // since last received, as game ticks
      char playerName[PLAYER_NAME_SIZE];
    };
    struct player_t {
      int fd;
      unsigned int port;
      unsigned int id;
      bool receivedMsg;
      char msg[DGRAM_SIZE+1]; // +1 is because msg always needs to end in NUL for read protection
      // repeated in memset zero whipe so final NUL always present.
      char *msgWalk;
      bool inUse;
      player_unique unique; // this should be cleared each time connection becomes free
    };
    player_t playerSckts[maxPlayers];
    for ( int i=0; i<maxPlayers; i++ ) {
      // does compiler combine all playerSckts[i] into one and then just use address from that?
      // so it doesn't have to do arithmetic to convert index to pointer more than once.
      playerSckts[i].port = listenPort+1+i; 
      playerSckts[i].fd =  shared::createSocket(playerSckts[i].port);
      playerSckts[i].inUse = false;
      playerSckts[i].id = i;
      memset((void*)&playerSckts[i].unique,0,sizeof(player_unique));
      shared::log(globVerb,3,"player socket created: fd:%d port:%u",playerSckts[i].fd,playerSckts[i].port);
    }
    player_t *freePlayer_p = NULL;
    // I think MSG_DONTWAIT still ensures if it reads anything, it will read the whole dgram.
    // So no possibility of reading incomplete messages unless they are too large and split into multiple dgrams?
    const int recv_flags = MSG_DONTWAIT | MSG_TRUNC;// | MSG_WAITALL;
    std::vector<Json::Value> jsonFromPlayers;

    /** GAME LOOP **/ {
    timeval tickStart;
    gettimeofday(&tickStart,NULL);
    while ( true ) {
      for ( player_t* i=playerSckts; i<&playerSckts[maxPlayers]; i++ ) {
	i->receivedMsg=false;
	if ( i->inUse )
	  i->unique.ticksSinceLast+=1;
	if ( (i->unique.ticksSinceLast/ticksPerSec)>WAIT_PLAYER_MIA && i->inUse ) { // drop player
	  // without inUse check this continuously runs for dropped player??
	  // first conditional should be enough, as any player dropped as their ticksSinceLast set
	  // to zero through memset below.
	  i->inUse = false;
	  // remove their actor from game too (with necessary cleanup)
	  // perhaps send message to client, saying they have been dropped, reasons
	  shared::log(globVerb,1,"Player %u left. Reason: lost sync too long.",i->id);
	  memset((void*)&i->unique,0,sizeof(player_unique));
	  continue;	  
	}
      }
      /** WAIT LOOP **/
      do { // do wait loop at least once, and then additionally until next tick
	/** RECEIVE NET MSG FROM PLAYER **/ {
	  jsonFromPlayers.clear();
	  for ( player_t* i=playerSckts; i<&playerSckts[maxPlayers]; i++ ) {
	    if ( i->inUse==false ) {
	      freePlayer_p = i;
	      continue;
	    }
	    if ( i->receivedMsg==true )
	      continue;
	    sockaddr_in srcAddr;
	    socklen_t srcAddrSz = sizeof(srcAddr);
	    memset(&srcAddr,0,srcAddrSz);
	    memset(i->msg,0,DGRAM_SIZE+1); //+1 for always trailing NUL
	    int j;
	    if ( (j=recvfrom(i->fd,(void*)i->msg,DGRAM_SIZE,recv_flags,(sockaddr*)&srcAddr,&srcAddrSz))<0
		 && errno!=0 && errno!=EAGAIN && errno!=EWOULDBLOCK ) { // with MSG_DONTWAIT errors are returned if would have to block
	      shared::log(globVerb,LOG_ERROR,"Receive from player socket");
	      return -1;
	    }
	    Json::Value receive;
	    Json::Reader jsonReader;
	    Json::Value send;
	    Json::FastWriter jsonWriter;
	    if ( j>0 && srcAddr.sin_port==i->unique.remote.sin_port && srcAddr.sin_addr.s_addr==i->unique.remote.sin_addr.s_addr ) {
	      jsonReader.parse(i->msg,i->msg+(DGRAM_SIZE-1),receive,false);
	    }
	    if ( !jsonReader.good() || j<=0 ) { 
	      // player data corrupted, add notice of this to game state sent to this player
	      continue;
	    }
	    i->receivedMsg = true;
	    //	    receive["actor"] = playerSck->player.actorId;
	    jsonFromPlayers.push_back(receive);
	    i->unique.ticksSinceLast=0;
	  }
	}

      } while ( !shared::tickStart(ticksPerSec,&tickStart) );

      /** END WAIT LOOP **/
		
        /** PARSE IMMEDIATE PLAYER ACTIONS **/ {
	// immediate means data in player message that should effect the game state sent back to player after update.
	// non immediate actions (like console commands) can be parsed after send to player, so as to not increase latency.
	for ( player_t* i=playerSckts; i<&playerSckts[maxPlayers]; i++ ) {
	  if ( i->receivedMsg==false && i->inUse ) {
	    shared::log(globVerb,3,"player %u missed sync; %u in row.",i->id,i->unique.ticksSinceLast);
	    continue;
	  }
	  if ( !i->inUse )
	    continue;
	  shared::log(globVerb,3,"player %u hit sync",i->id);
	  i->msgWalk = i->msg;
	  uint32_t runActions = (uint32_t)*i->msgWalk;
	  i->msgWalk+=sizeof(uint32_t);
	  float lookVec[3];
	  // floats fixed in net msg?
	  for ( float*j=lookVec; j<&lookVec[3]; j++ ) {
	  *j = (float)*i->msgWalk;
	  i->msgWalk+=sizeof(float);
	  }
	  uint32_t actorUseId = (uint32_t)*i->msgWalk;
	  i->msgWalk+=sizeof(uint32_t);
	  // Since, for now at least, all types in contiguous immediate actions area of msg are fixed size

	  /// @todo parse runActions,lookVec & actorUseId using player's actor \
	  and such
	  

	  actor_t *actor;// = i->unique.actorId;
	  /*	  actor.actionsDo = (actions.asUInt64() & ~(actor.actionsPass));
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
        static char dgram[DGRAM_SIZE];
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
        if ( freePlayer_p!=NULL ) {
            send["acc"] = true;
	    send["prt"] = freePlayer_p->port;
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
	    freePlayer_p->inUse = true;
	    shared::log(globVerb,1,"Player %u joined on port %u",freePlayer_p->id,freePlayer_p->port);
	    freePlayer_p = NULL;
	  }
	}

        }

      /** NON-IMMEDIATE ACTIONS **/ {
	for ( player_t* i=playerSckts; i<&playerSckts[maxPlayers]; i++ ) {
	  // loop through number of commands in message
	  int32_t numCmds = (int32_t)*i->msgWalk;
	  if ( numCmds>MAX_PLAYER_CMDS ) {
	    // handle excess commands; save them for next tick or discard and warn player
	    numCmds=0;
	  }
	  for ( int32_t j=0; j<numCmds; j++ ) {
	    i->msgWalk+=sizeof(int32_t);
	    switch ( j )
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


