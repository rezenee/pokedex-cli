#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "card.h"

void pretty_print(POKEMON_T* pokemon, FILE* fp_ascii);
POKEMON_T *parse_pokemon(char * buf, ABILITY_T** abilities, size_t total_abilities);
ABILITY_T* parse_ability(char * buf);

int sort_abilities(const void *a, const void *b) {
    ABILITY_T *a_ptr = *((ABILITY_T**) a);
    ABILITY_T *b_ptr = *((ABILITY_T**) b);
    return strcmp(a_ptr->name, b_ptr->name);
}

int sort_pokemon(const void *a, const void *b) {
    POKEMON_T* a_ptr = *((POKEMON_T**) a);
    POKEMON_T* b_ptr = *((POKEMON_T**) b);
    return strcmp(a_ptr->name, b_ptr->name);
}
int sort_comp(const void *a, const void *b) {
    const char *a_ptr = a;
    ABILITY_T *b_ptr = *((ABILITY_T**) b);
    return strcmp(a_ptr, b_ptr->name);
}
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: ./parser pokemon_csv.csv (0 || 1)\n");
        return -1;
    }
    if(*argv[2] != '0' && *argv[2] != '1') {
        printf("usage: ./parser pokemon_csv.csv (0 || 1)\n");
        return -1;
    }

    FILE * fp_abilities = fopen("abilities.csv", "r");
    ABILITY_T** abilities = NULL;
    size_t total_abilities = 0;
    char * line_buf = NULL;
    size_t n = 0;
    // skip line with variable definitions
    getline(&line_buf, &n, fp_abilities);
    // load the abilities into memory
    while(getline(&line_buf, &n, fp_abilities) > 0) {
        ABILITY_T * ability = parse_ability(line_buf);
        total_abilities++;
        abilities = realloc(abilities, total_abilities * 8);
        abilities[total_abilities - 1] = ability;
    }

    // sort the abilities array
    qsort(abilities, total_abilities, sizeof(*abilities), sort_abilities);

    FILE * fp_pokemon = fopen(argv[1], "r");
    size_t total_pokemon = 0;
    POKEMON_T ** pokemon_array = NULL;
    // skip line with variable definitions
    getline(&line_buf, &n, fp_pokemon);
    // write the pokemon to the file also create indexes array with their data
    while(getline(&line_buf, &n, fp_pokemon) > 0) {
        POKEMON_T * pokemon = parse_pokemon(line_buf, abilities, total_abilities);
        if(pokemon == NULL) continue; 

        pokemon_array = realloc(pokemon_array, sizeof(*pokemon_array) * ++total_pokemon);
        pokemon_array[total_pokemon -1] = pokemon;
    }
    qsort(pokemon_array, total_pokemon, sizeof(*pokemon_array), sort_pokemon);

    FILE* fp_ascii = 0;
    if(atoi(argv[2])) fp_ascii = fopen("ascii.bin", "rb");

    // output the data from the pokemon
    for(int i = 0; i < total_pokemon; i++) { 
        pretty_print(pokemon_array[i], fp_ascii);
        free(pokemon_array[i]->name);
        free(pokemon_array[i]);
    }
    free(pokemon_array);
    free(line_buf);
    for(int i = 0; i < total_abilities; i++) {
        free(abilities[i]->name);
        free(abilities[i]->desc);
        free(abilities[i]);
    }
    free(abilities);
    if(atoi(argv[2])) fclose(fp_ascii);
    fclose(fp_pokemon);
    fclose(fp_abilities);

    return 0;
}

ABILITY_T* parse_ability(char * buf) {
    ABILITY_T* ability = malloc(sizeof(*ability));
    ability->name = strdup(strsep(&buf, ","));

    ability->name[0] -= 32;
    char* stringp = ability->name;
    // walk through the string, if find -, make space, if next char not eol make capital.
    while(*(stringp++)) {
        if(*stringp == '-') {
            *stringp-= 13;
            if(*(stringp+1)) *(stringp+1) -= 32;
        }
    }

    // remove the ending newline
    ability->desc = strdup(strsep(&buf, "\n"));
    // if the last char is a period, should be removed
    if( ability->desc[strlen(ability->desc)-1] == '.' ) {
        ability->desc[strlen(ability->desc)-1] = 0;
    }
    return ability;
}
// name, id, type, subtype, ability_one, ability_two, ability_three, base_exp,
// weight, height, hp, attack, defense, special_attack, special_defense, speed,
// offset
POKEMON_T *parse_pokemon(char * buf, ABILITY_T** abilities, size_t total_abilities) {
    char * stringp = buf;
    // the name
    char * token = strsep(&stringp, ",");
    char * name = strdup(token);

    // id
    token = strsep(&stringp, ",");
    // if there is no id, means empty line. 
    if (*stringp == ',') {
        free(name);
        return 0;
    }

    POKEMON_T *pokemon = malloc(sizeof(*pokemon));
    pokemon->name = name;
    pokemon->id = atoi(token);

    // type
    token = strsep(&stringp, ",");
    for(int i = 0; i < 19; i++) {
        if(strcasestr(token, type_str[i]) != 0) {
            pokemon->type = i;
            break;
        }
    }
    // subtype
    if (*stringp != ',') {
        token = strsep(&stringp, ",");
        for(int i = 0; i < 19; i++) {
            if(strcasestr(token, type_str[i]) != 0) {
                pokemon->subtype = i;
                break;
            }
        }
    } else {
        pokemon->subtype = 0;
        stringp++;
    }
    // ability_one
    token = strsep(&stringp, ",");
    token[0] -= 32;

    char* stringp2= token;
    while(*(stringp2++)) {
        if(*stringp2 == '-') {
            *stringp2-= 13;
            if(stringp2+1) *(stringp2+1) -= 32;
        }
    }
    ABILITY_T **found_ability = bsearch(token, abilities, total_abilities, sizeof(ABILITY_T*), sort_comp);

    pokemon->ability_one = *found_ability;
    // the first ability is never hidden

    // ability_two
    if (*stringp != ',') {
        token = strsep(&stringp, ",");
        token[0] -= 32;

        stringp2= token;
        while(*(stringp2++)) {
            if(*stringp2 == '-') {
                *stringp2-= 13;
                if(stringp2+1) *(stringp2+1) -= 32;
            }
        }
        found_ability = bsearch(token, abilities, total_abilities, sizeof(ABILITY_T*), sort_comp);
        pokemon->ability_two = *found_ability;
    }
    else {
        pokemon->ability_two = NULL;
        stringp++;
    }
    // ability_three
    if (*stringp != ',') {
        token = strsep(&stringp, ",");
        token[0] -= 32;

        stringp2= token;
        while(*(stringp2++)) {
            if(*stringp2 == '-') {
                *stringp2-= 13;
                if(stringp2+1) *(stringp2+1) -= 32;
            }
        }
        found_ability = bsearch(token, abilities, total_abilities, sizeof(ABILITY_T*), sort_comp);
        pokemon->ability_three = *found_ability;
        pokemon->ability_three_hidden = 1;
        pokemon->ability_two_hidden = 0;
    }
    else {
        // if there is a second ability, but not a third, it is hidden.
        if(pokemon->ability_two) pokemon->ability_two_hidden = 1;
        pokemon->ability_three = 0;
        stringp++;
    }
    // base_exp
    token = strsep(&stringp, ",");
    pokemon->base_experience = atoi(token);
    // weight
    token = strsep(&stringp, ",");
    char * token_str = malloc(strlen(token) + 2);
    memset(token_str, 0, strlen(token) +2);
    strcat(token_str, token);
    token_str[strlen(token)] = token_str[strlen(token) - 1];
    token_str[strlen(token) -1] = '.';
    float num = atof(token_str);
    pokemon->weight = num;
    free(token_str);
    // height
    token = strsep(&stringp, ",");
    token_str = malloc(strlen(token) + 2);
    memset(token_str, 0, strlen(token) +2);
    strcat(token_str, token);
    token_str[strlen(token)] = token_str[strlen(token) - 1];
    token_str[strlen(token) -1] = '.';
    num = atof(token_str);
    pokemon->height = num;
    free(token_str);
    // hp
    pokemon->hp = atoi(strsep(&stringp, ","));
    // attack
    pokemon->attack = atoi(strsep(&stringp, ","));
    // defense
    pokemon->defense = atoi(strsep(&stringp, ","));
    // special_attack
    pokemon->special_attack = atoi(strsep(&stringp, ","));
    // special_defense
    pokemon->special_defense = atoi(strsep(&stringp, ","));
    // speed
    pokemon->speed = atoi(strsep(&stringp, ","));
    // offset
    pokemon->offset = atoi(strsep(&stringp, ","));
    return pokemon;
}

void pretty_print(POKEMON_T* pokemon, FILE* fp_ascii) {
        // PICTURE ////////////
    // if fp_ascii is null, that means its not supposed to display the art
    if(fp_ascii) {
        size_t data_len = 0;
        fseek(fp_ascii, pokemon->offset, SEEK_SET);
        fread(&data_len, sizeof(data_len), 1, fp_ascii);
        char *data_buf = malloc(data_len + 1);
        fread(data_buf, 1, data_len, fp_ascii);
        data_buf[data_len] = 0;
        printf("%s\n", data_buf);
        free(data_buf);
    }

    // ID, NAME & TYPE ////////////
    // print name & pokedex #
    printf("#%.03d ", pokemon->id);
    printf("%s\n", pokemon->name);
    // print type (and subtype if applicable)
    printf("\033[1m%s\033[0m", type_str[pokemon->type]);
    if (pokemon->subtype != NONE) {
        printf(" / \033[1m%s\033[0m\n", type_str[pokemon->subtype]);
    } else {
        printf("\n");
    }

    // ABILITIES ////////////
    // print abilities in a nice list
    printf("\n\033[1mAbilities:\033[0m");
    printf("\n1. %s\n", pokemon->ability_one->name);
    printf("   %s\n", pokemon->ability_one->desc);

    if(pokemon->ability_two){
        // sometimes the second ability is hidden
        if(pokemon->ability_two_hidden) {
            printf("\033[3m");
        }
        printf("2. %s", pokemon->ability_two->name);
        if(pokemon->ability_two_hidden) {
            printf(" (hidden)");
        }
        printf("\n   %s", pokemon->ability_two->desc);

        printf("\033[0m\n");
    }
    // third ability if it exists is guaranteed to be hidden. make italic
    if(pokemon->ability_three) {
        printf("\033[3m");
        printf("3. %s (hidden)\n", pokemon->ability_three->name);
        printf("   %s", pokemon->ability_three->desc);
        printf("\033[0m\n");
    }

    // remaining stat block
    printf("\nHP: %d\t\t| %.2fkg\n", pokemon->hp, pokemon->weight);
    if (pokemon->attack > 99) {
        printf("Atk: %d\t| %.2fm\n", pokemon->attack, pokemon->height);
    } else {
        printf("Atk: %d\t\t| %.2fm\n", pokemon->attack, pokemon->height);
    }
    if (pokemon->defense > 99) {
        printf("Def: %d\t|\n", pokemon->defense);
    } else {
        printf("Def: %d\t\t|\n", pokemon->defense);
    }
    printf("Sp. Atk: %d\t|\n", pokemon->attack);
    printf("Sp. Def: %d\t|\n", pokemon->special_defense);
    printf("Speed: %d\t| XP: %d\n\n", pokemon->speed, pokemon->base_experience);
}
