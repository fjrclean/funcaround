#include "network.hpp"

int network::createSocket(const unsigned int localPort) {
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


