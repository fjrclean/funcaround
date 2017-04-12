#ifndef SHARED_HPP
#define SHARED_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <stdbool.h>
#include <stdarg.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct timeval timeval;
//typdef struct


//#define MAKELOG_ERROR -1
#define MAKELOG_NET 4
enum makeLog_t {
  LOG_ERROR = -1,
  // nothing is output at 0
  LOG_NORMAL = 1,
  LOG_DETAILS = 2,
  LOG_NETMSG = 3,
  LOG_CHAT = 4

};

enum vector_t {
  X,
  Y,
  Z
};

enum reject_reason {
  REASON_FULL
};

enum server_query {
  JOIN
};

#define DGRAM_SIZE 1400 //bytes?
const int MAX_PLAYER_CMDS=3;
const int VERSION=1; // number to compare client/server versions.
//any json root must be split if bigger
const int PLAYER_NAME_SIZE=32;
const int MAX_JOIN_CMDS=3;


  
  //correspond to client_commands
  //string constants include null terminator?
  const char* cmd_format_strings[] = {
    "%s"
  };
  int logVerbosity = 0;
// @note function attributes const,pure seem not stop variadic functions from working?
void makeLog(int messgVerb,const char*message, ...)  __attribute__ ((format (printf,2,3)));
void makeLog(int messgVerb,const char*message, ...) {
  if ( messgVerb<=logVerbosity ) {
      switch ( messgVerb ) {
      case LOG_ERROR:
	printf("ERROR: ");
	break;
      default:
	printf("%d: ",messgVerb);
	break;
      }
      va_list valist;
      va_start(valist,message);
      vprintf(message,valist);
      va_end(valist);
      printf("\n");
    }
    //    if ( messgVerb==MAKELOG_NETMSG && )
    // print to net messg Log file
  }

  int createSocket(const unsigned int localPort) {
    int sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if ( sockfd<0 ) {
      return -1;
    }
    struct sockaddr_in lAddr;
    lAddr.sin_family = AF_INET;
    lAddr.sin_addr.s_addr = INADDR_ANY;
    lAddr.sin_port = htons(localPort);
    if ( bind(sockfd,(sockaddr *) &lAddr,sizeof(lAddr))<0 ) {
      //      std::cerr << "Error binding socket." << std::endl;
      return -1;
    }
    return sockfd;
  }

// @note with const func attribute, startConsole can read&set both global vars and objects pointed to by parameters, even though gcc manual states "Note that a function that has pointer arguments and examines the data pointed to must not be declared const."
FILE* startConsole(const char *configFn,char const *consoleFn);// __attribute__ ((const));
FILE* startConsole(const char *configFn,char const *consoleFn) {
    // @todo make multiple paths to be searched for files
    // @todo file error checking
    // @todo console file stuff to shared.hpp
  FILE *consoleFd = fopen(consoleFn,"w+");
  FILE *configTxt = fopen(configFn,"r");
  if ( consoleFd==NULL ) {
    makeLog(1,"failed startConsole");
    //    exit();
  }
  if ( configTxt!=NULL ) {
    makeLog(LOG_NORMAL,"Reading from %s:",configFn);
    int c;
    do {
      c = fgetc(configTxt);
      if ( c!=EOF )
	fputc(c,consoleFd);
    } while ( c!=EOF);
    fclose(configTxt);
  }
  fseek(consoleFd,0,SEEK_SET);
  return consoleFd;
}


// client should be able to parse some server variables when they are sent from server,
// eg ticksPerSec. Other details sent to client should either be arbitrary data in inital
// join reply, eg client's player number/ID, or if dynamic in game 'snapshot', eg player
// names which are attached to a player number.
// Therefore server variables are not stored locally on the client, as many are really
// commands that would have to be determined at time of execution.
// So a var request get/set is sent to server, and a reply returned.
// Shared code should be that which determines if cmd line is for server or client,
// and converts cmd string into game readable buffer.
// This buffer is then sent over network to server, or if on server added to local buffer.

// In client code check if comment, or local cmd, or set/show cmd using cl_ variable;
// if so do relevent client only thing.
// Else parse through shared function to convert line into netmsg and place in buf.
// Client needs to track each collection of cmds it sends to server, so it knows when
// they have been received. In the case of a show sv_ command, the server can just send
// variable value back as chat message to player for now.
// Later on it would be good for client to keep a table of sv_ variables that are kept up
// to date.??

enum variables_client {
  CL_EXIT
};

enum variables_server { // both clients and server must be aware of server variables
  SV_PLAYER_NAME,
  SV_ticksPerSec,
  NUM_SV_VARS
};

// loop through formats with console input using sscanf. If 1 returned, you know it's valid command, and which one, but wrong arguments.
#define MAX_VAR_NAME_SIZE 32
const char varServerNames[NUM_SV_VARS][MAX_VAR_NAME_SIZE] = {
  "sv_playerName",
  "sv_ticksPerSec"
};
const char varServerValues[NUM_SV_VARS][MAX_VAR_NAME_SIZE] = {
  "%s",
  "%u"
};
const char varServerHelp[] = {
  "player_name <name>"
};

#define CMD_VALUE_MAX_SIZE 32

typedef struct cmdMsg_t {
  uint32_t asPlayerId;
  bool set; // false for show variable;
  uint32_t varId;
  char bufValue[CMD_VALUE_MAX_SIZE];
} cmdMsg_t;

// add a player:<playerId> server cmd that will interpret the next cmd as if sent
// by that player, adding the netmsg onto that...???

enum getValidCmd_ {
  getValidCmd_CLIENT,getValidCmd_SERVER,getValidCmd_INVALID };
int getValidCmd(char *line, cmdMsg_t *msg, bool asServer, uint32_t playerId);
int getValidCmd(char *line, cmdMsg_t *msg, bool asServer, uint32_t playerId) {
  // @todo add aliases, so you can for example exit the game with just "/exit" rather than "/set exit 1"
  memset((void*)msg,0,sizeof(cmdMsg_t));

  msg->asPlayerId=playerId;
  unsigned int len = 0;
  // %n count argument success is not included in scanf return;
  if ( sscanf(line," player:%n%u%n",&len,&msg->asPlayerId,&len)==0 && len>0 ) {
    makeLog(LOG_NORMAL,"player:<player_id> must include player id as a positive number.");
    return getValidCmd_INVALID;
  }
  line+=len;

  len = 0;
  msg->set=false;
  sscanf(line," set %n",&len);
  line+=len;
  if ( len>0 )
    msg->set = true;
  len=0;

  int type = getValidCmd_INVALID;
  printf(":%s:",line);
  int j=0;
  for ( int i=0; i<NUM_SV_VARS; i++ ) {
    printf("name:%s:\n",varServerNames[i]);
    if ( (j=strcmp(line,varServerNames[i]))==0 ) {
      msg->varId=i;
      printf("yes\n");
      type=getValidCmd_SERVER;
    }
  }
  printf("%i\n",j);
  
  // @todo add warning if player:<player_id> was used on client variable.
 unknown_keyword:
  
  if ( type==getValidCmd_SERVER )
    printf("server\n");
  return type;
}


int consoleGetNextLine(char *valBuf) {
  int i;
  //  if ( (i=getline(&cmd,&cmdSz,consoleFd))>=0 )
  //    fwrite(cmd,i,1,stdout);

}

  bool startTick(uint32_t ticksPerSec,timeval *tickStart) { // use glfw time source?
    double tickDuration = (1/(double)ticksPerSec) * 1000000;
    timeval now, diff;
    gettimeofday(&now,NULL);
    timersub(&now,tickStart,&diff);
    if ( diff.tv_sec>=1 || diff.tv_usec>=tickDuration ) {
      gettimeofday(tickStart,NULL);
      return true;
    }
    return false;
  }

#endif // SHARED_HPP
