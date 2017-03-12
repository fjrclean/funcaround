#include "types.hpp"

class WorldObject {
    private:
    float m_x,m_y,m_z;
    bool m_grounded;
    float m_speed; // arbitrary movement speed not based in physics
    direction_t m_direction; // arbitrary movement direction

    public:
    WorldObject(float x, float y, float z);

    bool isGrounded() {
        return m_grounded;
    }

    void setSpeed(float speed) {
        m_speed += speed;
    }

    void setDirection(direction_t direction) {
        m_direction = direction;
    }


};
