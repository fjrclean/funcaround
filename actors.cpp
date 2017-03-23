enum actor_types {
  HUMAN,
  ALIEN,
  WEAPON_ALIEN_RIFLE
};

class actor {
public:
  uint64_t actions_base;
  uint64_t attributes_base;
  uint64_t actions_available;
  uint64_t actions_this_turn;
  actor_types actor_type;
};

