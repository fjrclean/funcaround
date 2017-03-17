#include "Actor.hpp"
#include "types.hpp"

class Player : Actor {

    private:
    std::vector<int> m_weapons;
    float m_speed;

    public:
    void shoot();
    void jump();
    void reload();
    void mvStartFwd();
    void mvStopFwd();

};
