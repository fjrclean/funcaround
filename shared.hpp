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

const int DGRAM_SIZE=1400; //bytes?
const int VERSION=1; // number to compare client/server versions.
//any json root must be split if bigger
const int PLAYER_NAME_SIZE=32;

enum client_commands {
  CMD_PLAYER_NAME
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
