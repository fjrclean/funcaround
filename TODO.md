client.c:38:  // @todo warn server variables in config file; no effect
client.c:92:    // @todo state reason for rejection
client.c:93:    return -1; // @todo change return's to exit() or better if using nested functions with gnu c.
client.c:130:      // @todo another loop for rendering with different rate.
server.c:143:	  /// @todo parse runActions,lookVec & actorUseId using player's actor \
server.c:167:	// @todo send changed server variables back to player.
server.c:171:	// @todo get rid of json in join request & reply.
server.c:172:	// @todo player join, send back to client ticksPerSec as well as port to join.
server.c:179:	// @todo is bool a fixed size, good for net msgs?
server.c:188:	// @todo send player their player id/num.
server.c:203:	// @todo add version check to join request
server.c:204:	// @todo ban system with filter in join code to work with it, better done at OS level with sockets?
server.c:212:	  // @todo reduce size of sendMsg buf as join/query will be fixed.
shared.h:101:    // @todo make multiple paths to be searched for files
shared.h:102:    // @todo file error checking
shared.h:103:    // @todo console file stuff to shared.hpp
shared.h:185:  // @todo add aliases, so you can for example exit the game with just "/exit" rather than "/set exit 1"
shared.h:218:  // @todo add warning if player:<player_id> was used on client variable.
