#include "Actor.hpp"
#include "types.hpp"

class Player : Actor {

    private:
    std::vector<int> m_weapons;
    m_speed;

    public:
    Actor() {
        attributes =| HUMAN;
        actions =| SHOOT | JUMP | RELOAD;
        m_speed = 20;
    }
    void run() {
        if

    }

};
