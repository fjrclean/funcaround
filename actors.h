typedef enum actor_types {
  HUMAN,
  ALIEN,
  WEAPON_ALIEN_RIFLE
}actor_types;

/*char *varBufFormatStrings[] = {

  }*/

const int MAX_ACTIONS = 32;

enum actions_t {
  // up by twos
  ACT_JUMP=1
};

typedef struct actor_t {
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
  /*  struct world {
    float x,y,z;
    float lookVec[3];
    };*/
}actor_t;

//each actor type file should include actors.hpp
//void example_actor_main(uint64_t *actions,uint64_t *attributes,

//int createActor(actor_types type,

