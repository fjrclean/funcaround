#ifndef NET_HPP
#define NET_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <iostream>

namespace network {

    int createSocket(const unsigned int localPort);

}

#endif // NET_HPP
