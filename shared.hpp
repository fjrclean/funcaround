#ifndef SHARED_HPP
#define SHARED_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <stdarg.h>

//#define LOG_ERROR -1
#define LOG_NET 4
enum log_t {
  LOG_ERROR = -1,
  // nothing is output at 0
  LOG_NORM = 1,
  LOG_DETAILS = 2,
  LOG_NETMSG = 3
};

enum vector_t {
  X,
  Y,
  Z
};

const int DGRAM_SIZE=1400; //bytes?
const int VERSION=1; // number to compare client/server versions.
//any json root must be split if bigger
const int PLAYER_NAME_SIZE=32;
const int MAX_JOIN_CMDS=3;


// on client, commands will be prefixed with cl_ or sv_ ,
// with sv_ being removed and rest sent to server.
// server will never see these prefixes.
enum commands_server {
  CMD_SET // set a variable
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

namespace shared {

  //correspond to client_commands
  //string constants include null terminator?
  const char* cmd_format_strings[] = {
    "%s"
  };

  int createSocket(const unsigned int localPort) {
    int sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if ( sockfd<0 ) {
      return -1;
    }
    sockaddr_in lAddr;
    lAddr.sin_family = AF_INET;
    lAddr.sin_addr.s_addr = INADDR_ANY;
    lAddr.sin_port = htons(localPort);
    if ( bind(sockfd,(sockaddr *) &lAddr,sizeof(lAddr))<0 ) {
      //      std::cerr << "Error binding socket." << std::endl;
      return -1;
    }
    return sockfd;
  }

  //  void log(int,int,const char*,...) __attribute__ ((pure));
  void log(int globalVerb,int messgVerb,const char*message, ...) {
    if ( messgVerb<=globalVerb ) {
      printf("%d: ",messgVerb);
      va_list valist;
      va_start(valist,message);
      vprintf(message,valist);
      va_end(valist);
      printf("\n");
    }
    //    if ( messgVerb==LOG_NETMSG && )
      // print to net messg log file
  }

  bool tickStart(double ticksPerSec,timeval *tickStart) { // use glfw time source?
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
}
#endif // SHARED_HPP
