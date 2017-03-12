#include <vector>

class Actor {

    // This is where we would lua connection to specialize actors

    private:
    float m_speed;
    // an actor can have control over other actors
    // this number references an actor id in the global actors list
    // some filter to allow partial control, eg only has control over certain actions for certain time/distance, etc
    std::vector<int> m_actors;
    int m_worldObject; // WorldObject which belongs to this actor, int referencing global list of WorldActors

    public:
    Actor(float speed);

    float getSpeed();

    void setSpeed(float speed);

    int getWorldObject();
};
