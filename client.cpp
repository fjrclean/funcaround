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

#include "shared.hpp"
#include "types.hpp"

uint64_t player_actions;

static void glfwKey(GLFWwindow* window, int key, int scancode, int action, int mods);
int main() {
  /**GAME OPTIONS**/
  char *playerName;
  
  /** NETWORK SETUP **/
  unsigned int serverPort = 5666;
  char serverIp[] = "127.0.0.1";
  int sockfd = shared::createSocket(0);
  sockaddr_in rAddr;
  rAddr.sin_family = AF_INET;

  /** QUERY SERVER **/ {
  Json::Reader jsonReader;
  Json::FastWriter jsonWriter;
  Json::Value jsonSend;
  jsonSend["ver"] = VERSION;
  jsonSend["ord"] = 0;
  std::string jsonSend_str = jsonWriter.write(jsonSend);
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
  if ( !jsonReader.good() ) {
    std::cerr << "server json reply corrupt" << std::endl;
  }
  if ( jsonReceive["acc"]!=true ) {
    std::cerr << "Could not join server, reason: " << jsonReceive.get("rea","error").asString() << std::endl;
    return -1;
  }
  if ( !jsonReceive["prt"].isUInt() ) {
    std::cerr << "Error: response received with nonsense port" << std::endl;
    return -1;
  }
  rAddr.sin_port = htons(jsonReceive["prt"].asUInt());
  }

  /** LOAD MAP **/
  


  if ( !glfwInit() ) {
    std::cerr << "Error: glfw init\n";
    return -1;
  }
  //  glfwSetErrorCallback(glfwError);
  GLFWwindow *window = glfwCreateWindow(640,480,"funcaround",NULL,NULL);
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

  /** GAME LOOP **/
  float ratio;
  int width,height;
  timeval tickStart;
  double ticksPerSec=1;
  Json::Reader jsonReader;
  Json::FastWriter jsonWriter;
  Json::Value jsonSend;
  jsonSend["cnm"] = CMD_PLAYER_NAME; // command to execute on server
  jsonSend["cvl"] = "clean"; // value of command, can be array if cmd takes multiple arguments
  std::string jsonSend_str;
  gettimeofday(&tickStart,NULL);
  while( !glfwWindowShouldClose(window) ) {
    while (!shared::tickStart(ticksPerSec,&tickStart)) { // pause game loop until time for next tick.
      glfwGetFramebufferSize(window,&width,&height);
      ratio=width/(float)height;
      glViewport(0,0,width,height);
      glClear(GL_COLOR_BUFFER_BIT);
      glfwSwapBuffers(window);
      glfwPollEvents(); 
    }

    /** SEND ACTIONS TO SERVER **/

    jsonSend["hgs"] = 0; // tell service highest game state id received
    jsonSend_str = jsonWriter.write(jsonSend);
    std::cout << "connect " << jsonSend << std::endl;
    if ( sendto(sockfd,jsonSend_str.c_str(),jsonSend_str.length(),0,(sockaddr *) &rAddr,sizeof(rAddr))<0 ) {
      std::cerr << "Error on jsonSend connect" << std::endl;
    }
  }
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
  return 0;
}

static void glfwKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}
