#include <sys/types.h>

typedef enum {
    NONE,
    NORMAL,
    FIRE,
    WATER,
    ELECTRIC,
    GRASS,
    ICE,
    FIGHTING,
    POISON,
    GROUND,
    FLYING,
    PSYCHIC,
    BUG,
    ROCK,
    GHOST,
    DRAGON,
    DARK,
    STEEL,
    FAIRY,
} Type;

const char* type_str[] = {
   "None",
   "Normal",
   "Fire",
   "Water",
   "Electric",
   "Grass",
   "Ice",
   "Fighting",
   "Poison",
   "Ground",
   "Flying",
   "Psychic",
   "Bug",
   "Rock",
   "Ghost",
   "Dragon",
   "Dark",
   "Steel",
   "Fairy",
};

typedef struct {
    char* name;
    off_t offset;
} INDEX_T;

typedef struct {
    char* name;
    char* desc;
    off_t offset;
} ABILITY_T;

typedef struct {
    char * ability_one;
    char * ability_two;
    char * ability_three;
} GEN_ABILITY_T;
 
typedef struct {
    char *name;
    unsigned id; 
    Type type;
    Type subtype;
    ABILITY_T* ability_one;
    ABILITY_T* ability_two;
    unsigned ability_two_hidden;
    ABILITY_T* ability_three;
    unsigned ability_three_hidden;
    unsigned base_experience;
    float weight;
    float height;
    unsigned hp; 
    unsigned attack;
    unsigned defense;
    unsigned special_attack;
    unsigned special_defense;
    unsigned speed;
    off_t offset;
} POKEMON_T;
