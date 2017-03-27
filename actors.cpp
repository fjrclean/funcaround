enum actor_types {
  HUMAN,
  ALIEN,
  WEAPON_ALIEN_RIFLE
};

char *varBufFormatStrings[] = {

}

struct actor {
  char *varBuf;
  char *varBufFormatString;
  void *runFunction;
  uint64_t actions_type;
  uint64_t attributes_type;
  uint64_t actions_instance;
  uint64_t actions_run;
  uint64_t actions_pass;
  actor_types actor_type;
  unsigned int actionPass[];
};

//each actor type file should include actors.hpp
void example_actor_main(uint64_t *actions,uint64_t *attributes,

int createActor(actor_types type,

