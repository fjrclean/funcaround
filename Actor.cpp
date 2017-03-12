#include "Actor.hpp"

Actor::Actor(float speed) {
    m_speed = speed;
}

float Actor::getSpeed() {
    return m_speed;
}

void Actor::setSpeed(float speed) {
    m_speed = speed;
}

int Actor::getWorldObject() {
    return m_worldObject;
}
