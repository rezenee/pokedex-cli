#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "card.h"

POKEMON_T *parse_pokemon(char * buf, ABILITY_T** abilities, size_t total_abilities);
ABILITY_T* parse_ability(char * buf);
void write_ability(ABILITY_T* ability, FILE* fp);
void write_index(INDEX_T* index, FILE* fp);
void write_pokemon(POKEMON_T* pokemon, FILE* fp);

int sort_abilities(const void *a, const void *b) {
    ABILITY_T *a_ptr = *((ABILITY_T**) a);
    ABILITY_T *b_ptr = *((ABILITY_T**) b);
    return strcmp(a_ptr->name, b_ptr->name);
}
int sort_indexes(const void*a, const void *b) {
    INDEX_T *a_ptr = *((INDEX_T**) a);
    INDEX_T *b_ptr = *((INDEX_T**) b);
    return strcmp(a_ptr->name, b_ptr->name);
}
int sort_comp(const void *a, const void *b) {
    const char *a_ptr = a;
    ABILITY_T *b_ptr = *((ABILITY_T**) b);
    return strcmp(a_ptr, b_ptr->name);
}
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage ./parser pokemon_csv.csv\n");
        return -1;
    }
    FILE * fp_abilities_bin = fopen("abilities.bin", "wb");
    FILE * fp_abilities = fopen("abilities.csv", "r");
    ABILITY_T** abilities = NULL;
    size_t total_abilities = 0;
    char * line_buf = NULL;
    size_t n = 0;

    // skip line with variable definitions
    getline(&line_buf, &n, fp_abilities);
    // write the abilities to the file, also load them into array
    while(getline(&line_buf, &n, fp_abilities) > 0) {
        ABILITY_T * ability = parse_ability(line_buf);
        total_abilities++;
        abilities = realloc(abilities, total_abilities * 8);
        abilities[total_abilities - 1] = ability;
        write_ability(ability, fp_abilities_bin);
    }

    // sort the abilities array
    qsort(abilities, total_abilities, sizeof(*abilities), sort_abilities);

    FILE * fp_pokemon = fopen(argv[1], "r");
    FILE * fp_pokemon_bin = fopen("pokemon.bin", "wb");
    INDEX_T** indexes = NULL;
    size_t total_pokemon = 0;
    POKEMON_T ** pokemon_structs = NULL;
    // skip line with variable definitions
    getline(&line_buf, &n, fp_pokemon);
    // write the pokemon to the file also create indexes array with their data
    while(getline(&line_buf, &n, fp_pokemon) > 0) {
        POKEMON_T * pokemon = parse_pokemon(line_buf, abilities, total_abilities);
        if(pokemon == NULL) continue;
        // must do indexes stuff before actually writing the data, for proper offset
        total_pokemon++;
        indexes = realloc(indexes, sizeof(INDEX_T*) * total_pokemon);
        INDEX_T* the_index = malloc(sizeof(INDEX_T));
        the_index->name = strdup(pokemon->name);
        the_index->offset = ftell(fp_pokemon_bin);
        indexes[total_pokemon - 1] = the_index;

        write_pokemon(pokemon, fp_pokemon_bin);
        free(pokemon->name);
        free(pokemon);
    }
    qsort(indexes, total_pokemon, sizeof(*indexes), sort_indexes);

    // write indexes.bin also freeing
    FILE* fp_pokemon_index = fopen("indexes.bin", "wb");
    fwrite(&total_pokemon, 1, sizeof(size_t), fp_pokemon_index);
    for(int i = 0; i < total_pokemon; i++) {
        write_index(indexes[i], fp_pokemon_index);
        free(indexes[i]->name);
        free(indexes[i]);
    }
    free(indexes);
    free(line_buf);
    for(int i = 0; i < total_abilities; i++) {
        free(abilities[i]->name);
        free(abilities[i]->desc);
        free(abilities[i]);
    }
    free(abilities);
    fclose(fp_pokemon_index);
    fclose(fp_pokemon);
    fclose(fp_pokemon_bin);
    fclose(fp_abilities);
    fclose(fp_abilities_bin);

    return 0;
}

void write_pokemon(POKEMON_T* pokemon, FILE* fp) {
    size_t empty = 0;
    fwrite(&pokemon->id, 1, sizeof(unsigned), fp);
    fwrite(&pokemon->type, 1, sizeof(unsigned), fp);
    fwrite(&pokemon->subtype, 1, sizeof(unsigned), fp);
    fwrite(&pokemon->ability_one->offset, 1, sizeof(off_t), fp);
    if(pokemon->ability_two) fwrite(&pokemon->ability_two->offset, 1, sizeof(off_t), fp);
    else fwrite(&empty, 1, sizeof(empty), fp);
    if(pokemon->ability_three) fwrite(&pokemon->ability_three->offset, 1, sizeof(off_t), fp);
    else fwrite(&empty, 1, sizeof(empty), fp);
    fwrite(&pokemon->base_experience, 1, sizeof(unsigned), fp);
    fwrite(&pokemon->weight, 1, sizeof(float), fp);
    fwrite(&pokemon->height, 1, sizeof(float), fp);
    fwrite(&pokemon->hp, 1, sizeof(unsigned), fp);
    fwrite(&pokemon->attack, 1, sizeof(unsigned), fp);
    fwrite(&pokemon->defense, 1, sizeof(unsigned), fp);
    fwrite(&pokemon->special_attack, 1, sizeof(unsigned), fp);
    fwrite(&pokemon->special_defense, 1, sizeof(unsigned), fp);
    fwrite(&pokemon->speed, 1, sizeof(unsigned), fp);
    fwrite(&pokemon->offset, 1, sizeof(off_t), fp);
}
void write_index(INDEX_T* index, FILE* fp) {
    size_t data_len = strlen(index->name);
    fwrite(&data_len, 1, sizeof(size_t), fp);
    fwrite(index->name, data_len, 1, fp);
    fwrite(&index->offset, 1, sizeof(off_t), fp);
}

void write_ability(ABILITY_T* ability, FILE* fp) {
    ability->offset = ftell(fp);

    // write the name of the ability
    size_t data_len = strlen(ability->name);
    fwrite(&data_len, sizeof(size_t), 1, fp);
    fwrite(ability->name, 1, strlen(ability->name), fp);

    // write the description of the ability
    data_len = strlen(ability->desc);
    fwrite(&data_len, sizeof(size_t), 1, fp);
    fwrite(ability->desc, 1, strlen(ability->desc), fp);
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
    // make the first char of the ability capital
    token[0] -= 32;
    char* stringp2= token;
    // iterate through each char of the token.
    while(*(stringp2++)) {
        // if the ability has more than one word, make the - a ' ', and capitalize the next letter.
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
