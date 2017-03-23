#include "Actor.hpp"
#include "types.hpp"
#include "DrawableHuman.hpp"

class Player : Actor {

    private:
    std::vector<int> m_weapons;
    m_speed;
    DrawablePrimitive m_myCube;

    public:
    Actor() {
        attributes =| HUMAN;
        actions =| SHOOT | JUMP | RELOAD;
        m_speed = 20;
        m_myCube = DrawableHuman();
    }
    std::vector<Drawable*> getDrawables() {
        return std::vector<Drawable*>(&m_myCube);
    }
    void run() {
        if

    }

};
