#include "WorldObject.hpp"

WorldObject::WorldObject(float x, float y, float z) {
    m_x = x;
    m_y = y;
    m_z = z;
    m_grounded = false;
    m_speed = 0;
}


