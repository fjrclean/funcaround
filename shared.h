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
  LOG_NETMSG = 3
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


// on client, commands will be prefixed with cl_ or sv_ ,
// with sv_ being removed and rest sent to server.
// server will never see these prefixes.
enum commands_t { 
  CMD_SET=1,// set a variable
  CMD_EXIT=2
};

enum variables_server { //
  VAR_PLAYER_NAME,
  VAR_PLACEHOLDER1,
  VAR_PLACEHOLDER2,
};

// loop through formats with console input using sscanf. If 1 returned, you know it's valid command, and which one, but wrong arguments.
const char varServerFormats[] = {
  "player_name %s"
};

const char varServerHelp[] = {
  "player_name <name>"
};

  
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
    makeLog(1,"failed makeLog");
  }
  if ( configTxt!=NULL ) {
    makeLog(LOG_NORMAL,"%s found",configFn);
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
