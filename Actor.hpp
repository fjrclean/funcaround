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
    uint64_t m_attributes;
    uint64_t m_actions; // actions this actor does
    uint64_t m_actionsDo; //actions done this run.
    uint64_t m_actionsPass; //actions to be passed this run.
    Actor* m_passActionsTo[64]; // elements 0-63 correspond to actions in actions_t enum.
    // should only be checked if action is in actionsPass


    private:

    public:

    virtual void run() =0;
    virtual std::vector<Drawable*> getDrawables() =0;
    Actor() {
        // use default actor type's attributes and actions
    }
    Actor(uint64_t acts, uint64_t attrs) {
        m_attributes = attrs;
        m_actions = acts;
    }

    uint64_t getActions(uint64_t yourAttributes) {
        return m_actions;
    }
    void setActionsDo(uint64_t actions) {
        m_actionsDo = actions;
    }
    void addActionsDo(uint64_t actions) {
        m_actionsDo |= actions;
    }
    uint64_t getAttributes() {
        return m_attributes;
    }

};
