#include <vector>
#include <string>
#include <cmath>

enum attributes_t {
    IS_ALIEN=1,
    IS_HUMAN=2
};

struct action_t {
    unsigned int actorId;
    uint64_t action;
};

class Actor {

    public:
    uint64_t attributes;
    uint64_t actions; // actions this actor does
    uint64_t actionsDo; //actions done this run.
    uint64_t actionsPass; //actions to be passed this run.
    Actor* passActionsTo[64]; // elements 0-63 correspond to actions in actions_t enum.
    // should only be checked if action is in actionsPass


    private:

    public:

    virtual void run() =0;

    Actor() {

    }
    Actor(uint64_t acts, uint64_t attrs) {
        m_attributes = attrs;
        m_actions = acts;
    }

};
