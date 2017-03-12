#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <cstring>

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
//any json root must be split if bigger

int main() {

    int sockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if ( sockfd<0 ) {
        std::cerr << "Error creating socket." << std::endl;
        return -1;
    }

    Json::Value request;
    request["name"] = "clean";
    request["team"] = 1;
    Json::FastWriter test0;
    std::string test00 = test0.write(request);
    const char *data = test00.c_str();
    std::cout << data << std::endl;


    unsigned int serverPort = 5666;
    char serverIp[] = "127.0.0.1";
    sockaddr_in rAddr;
    rAddr.sin_family = AF_INET;
    if ( inet_aton(serverIp,&rAddr.sin_addr)==0 ) {
        std::cerr << "Error server IP" << std::endl;
    }
    rAddr.sin_port = htons(serverPort);
    if ( sendto(sockfd,data,strlen(data),0,(sockaddr *) &rAddr,sizeof(rAddr))<0 ) {
        std::cerr << "Error on send join request" << std::endl;
    }

    return 0;
}
