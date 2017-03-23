#ifndef TYPES_HPP
#define TYPES_HPP

const int DGRAM_SIZE=1400; //bytes?
const int VERSION=1; // number to compare client/server versions.
//any json root must be split if bigger

enum direction_t {
    FORWARD=1,
    LEFT=2,
    RIGHT=4,
    BACKWARD=6,
    UP=8,
    DOWN=10
};

enum actions_t { // up to 64 different actions
    JUMP,
    SHOOT,
    RELOAD,
    MV_START_FWD,
    MV_STOP_FWD,
    MV_START_LEFT,
    MV_STOP_LEFT,
    MV_START_RIGHT,
    MV_STOP_RIGHT,
    MV_START_BACK,
    MV_STOP_BACK,
    MV_START_UP,
    MV_STOP_UP,
    MV_START_DOWN,
    MV_STOP_DOWN,
    USE
};



#endif // TYPES_HPP
