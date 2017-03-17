#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <cstring>

#include <sys/time.h>

#define GLFW_INCLUDE_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Actor.hpp"
#include "WorldObject.hpp"
#include "types.hpp"
#include "network.hpp"

int main() {

    unsigned int serverPort = 5666;
    char serverIp[] = "127.0.0.1";
    int sockfd = network::createSocket(0);
    Json::Reader jsonReader;
    Json::FastWriter jsonWriter;
    Json::Value jsonSend;
    jsonSend["ver"] = 2;
    jsonSend["ord"] = 0;
    std::string jsonSend_str = jsonWrite.writer(jsonSend);
    sockaddr_in rAddr;
    rAddr.sin_family = AF_INET;
    if ( inet_aton(serverIp,&rAddr.sin_addr)==0 ) {
        std::cerr << "Error server IP" << std::endl;
        return -1;
    }
    rAddr.sin_port = htons(serverPort);
    if ( sendto(sockfd,jsonSend_str.c_str(),jsonSend_str.length(),0,(sockaddr *) &rAddr,sizeof(rAddr))<0 ) {
        std::cerr << "Error on send join request" << std::endl;
    }
    int flags = MSG_TRUNC; // | MSG_WAITALL;
    sockaddr_in srcAddr;
    socklen_t srcAddrSz = sizeof(srcAddr);
    char dgram[DGRAM_SIZE];
    memset(dgram,0,DGRAM_SIZE);
    int i=0;
    errno = 0;
    if ( (i=recvfrom(sockfd,(void *) &dgram,DGRAM_SIZE,flags,(sockaddr*)&srcAddr,&srcAddrSz))<0 && errno!=0 ) {
        // always error?
        //std::cerr << "Error on listen socket: " << errno << std::endl;
        //return -1;
    }
    if ( rAddr.sin_addr.s_addr!=srcAddr.sin_addr.s_addr ) {
        std::cerr << "Error: response received from unknown address" << std::endl;
        return -1;
    }
    Json::Value jsonReceive;
    jsonReader.parse(dgram,dgram+(DGRAM_SIZE-1),jsonReceive,false);
    if ( jsonSend["acc"]!=true ) {
        std::cerr << "Could not join server, reason: " << jsonSend.get("rea","error").asString() << std::endl;
        return -1;
    }
    if ( !jsonReceive["prt"].isUint() ) {
        std::cerr << "Error: response received with nonsense port" << std::endl;
        return -1;
    }
    rAddr.sin_port = htons(jsonReceive.["prt"].asUInt());
    jsonSend["nam"] = "clean";
    jsonSend["ord"] = jsonReceive["ord"].asUInt()+=1;
    jsonSend_str = jsonWriter.write(jsonSend);
    std::cout << "connect " << jsonSend << std::endl;
    if ( sendto(sockfd,jsonSend_str.c_str(),jsonSend_str.length(),0,(sockaddr *) &rAddr,sizeof(rAddr))<0 ) {
        std::cerr << "Error on jsonSend connect" << std::endl;
    }

    if ( !!glfwInit() ) {
        std::cerr << "Error: glfw init\n";
        return -1;
    }
    glfwSetErrorCallback(glfwError);
    GLFWwindow *window = glfwCreateWindow(640,480,"funaround",NULL,NULL);
    if ( !window ) {
        glfwTerminate();
        std::cerr << "Error: glfw window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window,glfwKey);
    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    double mouseX,mouseY;
    glfwGetCursorPos(window,&mouseX,&mouseY);
    float ratio;
    int width,height;
    while( !glfwWindowShouldClose() ) {
        glfwGetFramebufferSize(window,&width,&height);
        ratio=width/(float)height;
        glViewPort(0,0,width,height);
        glClear(GL_COLOR_BUFFER_BIT);
    }


    return 0;
}
