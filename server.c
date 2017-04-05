#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sys/time.h>

#include "types.h"
#include "shared.h"
#include "actors.h"

#define WAIT_PLAYER_MIA 5 // time to wait, in seconds, before dropping player who has sent no legible data

int main(int argc, char* args[]) {
    /** GAME OPTIONS **/
    unsigned int listenPort = 5666;
    unsigned int maxPlayers = 3;
        for ( int i=0; i<argc; i++ ) {
        if ( args[i]=="--port" ) {
            i+=1;
            listenPort = atoi(args[i]);
        } else if ( args[i]=="--map" ) {
            i+=1;
	    //        mapName = std::string(args[i]);
        }
    }
    int logVerbosity=3;
    double ticksPerSec=1;
    
    /** NETWORK SETUP **/
    const int listenSocket = createSocket(listenPort);
    typedef struct player_unique {
      sockaddr_in remote;
      unsigned int actorId;
      unsigned int ticksSinceLast; // since last received, as game ticks
      char playerName[PLAYER_NAME_SIZE];
    }player_unique;
    typedef struct player_t {
      int fd;
      uint32_t port;
      unsigned int id;
      bool receivedMsg;
      char msg[DGRAM_SIZE+1]; // +1 is because msg always needs to end in NUL for read protection
      // repeated in memset zero whipe so final NUL always present.
      char *msgWalk;
      bool inUse;
      player_unique unique; // this should be cleared each time connection becomes free
    }player_t;
    player_t playerSckts[maxPlayers];
    for ( int i=0; i<maxPlayers; i++ ) {
      // does compiler combine all playerSckts[i] into one and then just use address from that?
      // so it doesn't have to do arithmetic to convert index to pointer more than once.
      playerSckts[i].port = listenPort+1+i; 
      playerSckts[i].fd =  createSocket(playerSckts[i].port);
      playerSckts[i].inUse = false;
      playerSckts[i].id = i;
      memset((void*)&playerSckts[i].unique,0,sizeof(player_unique));
      makeLog(3,"player socket created: fd:%d port:%u",playerSckts[i].fd,playerSckts[i].port);
    }
    player_t *freePlayer_p = NULL;
    // I think MSG_DONTWAIT still ensures if it reads anything, it will read the whole dgram.
    // So no possibility of reading incomplete messages unless they are too large and split into multiple dgrams?
    const int recv_flags = MSG_DONTWAIT | MSG_TRUNC;// | MSG_WAITALL;

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
	  makeLog(1,"Player %u left. Reason: lost sync too long.",i->id);
	  memset((void*)&i->unique,0,sizeof(player_unique));
	  continue;	  
	}
      }
      /** WAIT LOOP **/
      do { // do wait loop at least once, and then additionally until next tick
	/** RECEIVE NET MSG FROM PLAYER **/ {
	  for ( player_t* i=playerSckts; i<&playerSckts[maxPlayers]; i++ ) {
	    i->msgWalk=NULL;
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
	      makeLog(LOG_ERROR,"Receive from player socket");
	      return -1;
	    }
	    if ( !(j>0 && srcAddr.sin_port==i->unique.remote.sin_port && srcAddr.sin_addr.s_addr==i->unique.remote.sin_addr.s_addr) ) {
	      continue;
	    }
	    i->receivedMsg = true;
	    i->unique.ticksSinceLast=0;
	  }
	}

      } while ( !startTick(ticksPerSec,&tickStart) );

      /** END WAIT LOOP **/
		
        /** PARSE IMMEDIATE PLAYER ACTIONS **/ {
	// immediate means data in player message that should effect the game state sent back to player after update.
	// non immediate actions (like console commands) can be parsed after send to player, so as to not increase latency.
	for ( player_t* i=playerSckts; i<&playerSckts[maxPlayers]; i++ ) {
	  if ( i->receivedMsg==false && i->inUse ) {
	    makeLog(3,"player %u missed sync; %u in row.",i->id,i->unique.ticksSinceLast);
	    continue;
	  }
	  if ( !i->inUse )
	    continue;
	  makeLog(3,"player %u hit sync",i->id);
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

      /** DO PHYSICS/COLLISION **/ {

      }

      /** SEND UPDATES TO PLAYERS **/ {
	// @todo send changed server variables back to player.
      }

      /** JOIN OR QUERY SERVER **/ {
	// @todo get rid of json in join request & reply.
	// @todo player join, send back to client ticksPerSec as well as port to join.
        sockaddr_in srcAddr;
        socklen_t srcAddrSz = sizeof(srcAddr);
	char *recvMsg = NULL;
	size_t recvMsgSz = 0;
        static char sendMsg[DGRAM_SIZE+1];
        memset(sendMsg,0,DGRAM_SIZE+1);
	// @todo is bool a fixed size, good for net msgs?
	bool *accept = (bool*)&sendMsg;
	*accept = false;
	int32_t *reason = (int32_t*)accept+1;
	*reason = REASON_FULL;
	uint32_t *tick = (uint32_t*)reason+1;
	*tick = ticksPerSec;
	uint32_t *port = tick+1;
	*port = freePlayer_p->port;
        if ( freePlayer_p!=NULL ) {
	  recvMsg = freePlayer_p->msg;
	  recvMsgSz = DGRAM_SIZE;
	  freePlayer_p->msgWalk = freePlayer_p->msg+(sizeof(uint32_t)*2);
	  freePlayer_p->receivedMsg = true;
	  *accept = true;
	}
	errno = 0;
	int i=0;
        if ( (i=recvfrom(listenSocket,(void *) recvMsg,recvMsgSz,recv_flags,(sockaddr*)&srcAddr,&srcAddrSz))<0
	  && errno!=0 && errno!=EAGAIN && errno!=EWOULDBLOCK ) { // with MSG_DONTWAIT errors are returned if would have to block
	  makeLog(1,"join error");
	  return -1;
        }
	// @todo add version check to join request
	// @todo ban system with filter in join code to work with it, better done at OS level with sockets?
	
	if ( i>0 && *accept==true ) {
	  freePlayer_p->inUse = true;
	  makeLog(3,"Player %u on port %u reserved, awating connection.",freePlayer_p->id,freePlayer_p->port);
	  freePlayer_p = NULL;
	}
	if ( i>0 ) {
	  // @todo reduce size of sendMsg buf as join/query will be fixed.
	  if ( sendto(listenSocket,&sendMsg,DGRAM_SIZE,0,(sockaddr *) &srcAddr,sizeof(srcAddr))<0 )
	    makeLog(LOG_ERROR,"send join request");
	  
	}

      }

      /** NON-IMMEDIATE ACTIONS **/ {
	for ( player_t* i=playerSckts; i<&playerSckts[maxPlayers]; i++ ) {
	  // loop through number of commands in message
	  
	  uint32_t numCmds = 0;
	  if ( i->msgWalk!=NULL )
	    numCmds=(uint32_t)*i->msgWalk;
	  if ( numCmds==0 )
	    continue;
	  if ( numCmds>MAX_PLAYER_CMDS ) {
	    // handle excess commands; save them for next tick or discard and warn player
	  }
	  i->msgWalk+=sizeof(uint32_t);
	  for ( int32_t j=0; j<numCmds; j++ ) {
	    int32_t cmd = (int32_t)*i->msgWalk;
	    i->msgWalk+=sizeof(int32_t);
	    int32_t *var = NULL;
	    switch ( cmd ) {
	    case CMD_SET:
	      var = (int32_t*)i->msgWalk;
	      i->msgWalk+=sizeof(int32_t);
	      switch ( *var ) {
	      case VAR_PLAYER_NAME:
		*(i->msgWalk+(PLAYER_NAME_SIZE-1))='\0';
		strncpy(i->unique.playerName,i->msgWalk,PLAYER_NAME_SIZE);
		makeLog(3,"player %u's name set to %s.\n",i->id,i->unique.playerName);
		i->msgWalk+=PLAYER_NAME_SIZE;
	      }
	      break;
	    default:
	      // cmd not recognised.
	      break;
	    }
	    }
	}
      }


    }
    }

}


