#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#define GLFW_INCLUDE_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shared.h"
#include "types.h"
#include "actors.h"

uint64_t player_actions;

static void glfwKey(GLFWwindow* window, int key, int scancode, int action, int mods);
int main() {
  /**GAME OPTIONS**/
  char playerName[32] = "clean";
  int actionGlfwKeyIndex[MAX_ACTIONS];

  FILE *consoleFd = startConsole("sv_config","sv_console");
  
  /** NETWORK SETUP **/
  unsigned int serverPort = 5666;
  char serverIp[] = "127.0.0.1";
  int sockfd = createSocket(0);
  sockaddr_in rAddr;
  rAddr.sin_family = AF_INET;
  

  /** QUERY SERVER **/ {
    char sendMsg[DGRAM_SIZE+1];
    size_t sendMsgSz = DGRAM_SIZE;
    if ( inet_aton(serverIp,&rAddr.sin_addr)==0 ) {
      makeLog(1,"Invalid server ip.");
      return -1;
    }
    uint32_t *version = (uint32_t*)&sendMsg;
    uint32_t *query = (uint32_t*)version+1; //type of query, eg if client wants to join or just information
    //    char *sendMSgWalk = (char*)(query+1);
    uint32_t *numOfCmds = (query+1);
    *numOfCmds = 1;
    int32_t *cmd = (int32_t*)numOfCmds+1;
    *cmd = CMD_SET;
    int32_t *var = cmd+1;
    *var = VAR_PLAYER_NAME;
    char *val1 = (char*)(var+1);
    strncpy(val1,playerName,32);
    
  rAddr.sin_port = htons(serverPort);
  if ( sendto(sockfd,&sendMsg,DGRAM_SIZE,0,(sockaddr *) &rAddr,sizeof(rAddr))<0 ) {
    makeLog(1,"Error on send join request");
  }
  int flags = MSG_TRUNC; // | MSG_WAITALL;
  sockaddr_in srcAddr;
  socklen_t srcAddrSz = sizeof(srcAddr);
  char recvMsg[DGRAM_SIZE+1];
  memset(recvMsg,0,DGRAM_SIZE+1);
  int i=0;
  errno = 0;
  if ( (i=recvfrom(sockfd,(void *) &recvMsg,DGRAM_SIZE,flags,(sockaddr*)&srcAddr,&srcAddrSz))<0 && errno!=0 ) {
    // always error?
    //std::cerr << "Error on listen socket: " << errno << std::endl;
    //return -1;
  }
  if ( rAddr.sin_addr.s_addr!=srcAddr.sin_addr.s_addr ) {
    makeLog(LOG_NORMAL,"Response received from unknown address");
    return -1;
  }
  bool *accept = (bool*)recvMsg;
  int32_t *reason = (int32_t*)accept+1;
  if ( accept==false ) {
    // @todo state reason for rejection
    return -1; // @todo change return's to exit() or better if using nested functions with gnu c.
  }
  uint32_t *ticksPerSec = (uint32_t*)reason+1;
  uint32_t *serverPort = ticksPerSec+1;
  printf("server tick %u on port %u\n",*ticksPerSec,*serverPort);
    
  }

  /** LOAD MAP **/
  
  if ( !glfwInit() ) {
    makeLog(LOG_ERROR,"glfw init");
    return -1;
  }
  //  glfwSetErrorCallback(glfwError);
  GLFWwindow *window = glfwCreateWindow(640,480,"funcaround",NULL,NULL);
  if ( !window ) {
    glfwTerminate();
    makeLog(LOG_ERROR,"glfw window");
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  //glfwSetKeyCallback(window,glfwKey);
  glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
  double mouseX,mouseY;
  glfwGetCursorPos(window,&mouseX,&mouseY);

  /** GAME LOOP **/
  float ratio;
  int width,height;
  timeval tickStart;
  uint32_t ticksPerSec=1;
  char *cmd = (char*)malloc(256);
  size_t cmdSz = 256;
  gettimeofday(&tickStart,NULL);
  while( !glfwWindowShouldClose(window) ) {
    /** WAIT LOOP **/
    do { // pause game loop until time for next tick.
      // @todo another loop for rendering with different rate.
      glfwGetFramebufferSize(window,&width,&height);
      ratio=width/(float)height;
      glViewport(0,0,width,height);
      glClear(GL_COLOR_BUFFER_BIT);
      glfwSwapBuffers(window);
      glfwPollEvents();

      /** INTERPRET CONSOLE COMMANDS **/
      //getline()
      int i;
      if ( (i=getline(&cmd,&cmdSz,consoleFd))>=0 )
	 fwrite(cmd,i,1,stdout);
      
    } while (startTick(ticksPerSec,&tickStart));

    /** SEND ACTIONS TO SERVER **/

  }
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
  return 0;
}

// for callback input method
/*static void glfwKeysToActions(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if ( glfwKeyActionIndex[key]==0 && glfwKeyCmdIndex[key]==0 )
    return;
  
  if ( glfwKeyCmdIndex[key]!=0 )
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    }*/

// for glfwGetKey() polling method
//uint32_T keysToActions(uint32_t actions,
