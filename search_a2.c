#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "card.h"

void pretty_print(POKEMON_T* pokemon, FILE* fp_ascii);
void free_pokemon(POKEMON_T* pokemon);
void free_ability(ABILITY_T* ability);
INDEX_T* read_index(FILE* fp_index);
POKEMON_T* read_pokemon(off_t offset, FILE* fp_pokemon, FILE* fp_abilities);
ABILITY_T* read_ability(off_t offset, FILE* fp_abilities);

int index_comp(const void *a, const void *b) {
    const char *a_ptr = a;
    INDEX_T *b_ptr = *((INDEX_T**) b);
    return strcmp(a_ptr, b_ptr->name);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: ./search (0 || 1)\n");
        return -1;
    }
    if(*argv[1] != '0' && *argv[1] != '1') {
        printf("usage: ./search (0 || 1)\n");
        return -1;
    }
    // load the indexes from file into **indexes
    FILE * fp_indexes = fopen("indexes.bin", "r");
    size_t total_pokemon;
    fread(&total_pokemon, sizeof(size_t), 1, fp_indexes);
    INDEX_T **indexes = malloc(sizeof(INDEX_T*) * total_pokemon);
    for(int i = 0; i < total_pokemon; i++) {
        indexes[i] = read_index(fp_indexes);
    }
    fclose(fp_indexes);

    FILE * fp_pokemon = fopen("pokemon.bin", "r");
    FILE * fp_abilities = fopen("abilities.bin", "r");
    FILE * fp_ascii = 0;
    if(atoi(argv[1])) fp_ascii = fopen("ascii.bin", "r");

    int echo = !isatty(fileno(stdin));
    char *input_buf = NULL;
    size_t n = 0;
    while (1) {
        printf(">> ");

        // read search query from the user
        getline(&input_buf, &n, stdin);
        input_buf[strcspn(input_buf, "\n")] = 0;

        if(echo) printf("%s\n", input_buf);

        // if user types !, exit loop
        if(input_buf[0] == '!') break;

        // search for pokemon name user typed
        INDEX_T **found_index = bsearch(input_buf, indexes, total_pokemon, sizeof(INDEX_T*), index_comp);
        if(!found_index) {
            printf("%s not found!\n", input_buf);
            continue;
        }

        POKEMON_T * pokemon = read_pokemon(found_index[0]->offset, fp_pokemon, fp_abilities);
        pokemon->name = strdup(input_buf);

        pretty_print(pokemon, fp_ascii);
        free_pokemon(pokemon);
    }
    free(input_buf);
    fclose(fp_pokemon);
    fclose(fp_abilities);
    if(atoi(argv[1])) fclose(fp_ascii);

    for(int i = 0; i < total_pokemon; i++) {
        free(indexes[i]->name);
        free(indexes[i]);
    }
    free(indexes);

    return 0;
}

INDEX_T* read_index(FILE* fp) {
    INDEX_T* index = malloc(sizeof(INDEX_T));
    size_t name_len;

    fread(&name_len, sizeof(size_t), 1, fp);
    index->name = malloc(name_len + 1);
    index->name[name_len] = 0;
    fread(index->name, 1, name_len, fp);
    fread(&index->offset, sizeof(off_t), 1, fp);

    return index;
}

POKEMON_T* read_pokemon(off_t offset, FILE* fp_pokemon, FILE* fp_abilities) {
    // update the pokemon data to be at proper offset
    fseeko(fp_pokemon, offset, SEEK_SET);
    POKEMON_T* pokemon = malloc(sizeof(POKEMON_T));

    // read id, type, subtype
    fread(&pokemon->id, sizeof(unsigned), 1, fp_pokemon);
    fread(&pokemon->type, sizeof(unsigned), 1, fp_pokemon);
    fread(&pokemon->subtype, sizeof(unsigned), 1, fp_pokemon);

    // offset is location into abilities file to the data
    fread(&offset, sizeof(off_t), 1, fp_pokemon);
    pokemon->ability_one = read_ability(offset, fp_abilities);

    // read ability two
    fread(&offset, sizeof(off_t), 1, fp_pokemon);
    if(offset) pokemon->ability_two = read_ability(offset, fp_abilities);
    else pokemon->ability_two = 0;

    // read ability three
    fread(&offset, sizeof(off_t), 1, fp_pokemon);
    if(offset) {
        pokemon->ability_three = read_ability(offset, fp_abilities);
        // if a third ability exists, that means its the hidden one
        pokemon->ability_three_hidden = 1;
        pokemon->ability_two_hidden = 0;
    }
    else {
        pokemon->ability_three = 0;
        // if there isnt a third ability, but there is a second, it is hidden
        if(pokemon->ability_two) {
            pokemon->ability_two_hidden = 1;
        }
    }
    // load the rest of the data that is simple
    fread(&pokemon->base_experience, sizeof(unsigned), 1, fp_pokemon);
    fread(&pokemon->weight, sizeof(float), 1, fp_pokemon);
    fread(&pokemon->height, sizeof(float), 1, fp_pokemon);
    fread(&pokemon->hp, sizeof(unsigned), 1, fp_pokemon);
    fread(&pokemon->attack, sizeof(unsigned), 1, fp_pokemon);
    fread(&pokemon->defense, sizeof(unsigned), 1, fp_pokemon);
    fread(&pokemon->special_attack, sizeof(unsigned), 1, fp_pokemon);
    fread(&pokemon->special_defense, sizeof(unsigned), 1, fp_pokemon);
    fread(&pokemon->speed, sizeof(unsigned), 1, fp_pokemon);
    fread(&pokemon->offset, sizeof(offset), 1, fp_pokemon);

    return pokemon;
}

ABILITY_T* read_ability(off_t offset, FILE* fp_abilities) {
    ABILITY_T* ability = malloc(sizeof(ABILITY_T));
    size_t text_len;
    fseeko(fp_abilities, offset, SEEK_SET);

    // read ability name
    fread(&text_len, sizeof(size_t), 1, fp_abilities);
    ability->name = malloc(text_len + 1);
    ability->name[text_len] = 0;
    fread(ability->name, 1, text_len, fp_abilities);

    // read abilty desc
    fread(&text_len, sizeof(size_t), 1, fp_abilities);
    ability->desc = malloc(text_len + 1);
    ability->desc[text_len] = 0;
    fread(ability->desc, 1, text_len, fp_abilities);

    return ability;
}

void pretty_print(POKEMON_T* pokemon, FILE* fp_ascii) {
    if(fp_ascii) {
        char * data_buf = malloc(40000);
        memset(data_buf, 0, 40000);
        size_t data_len = 0;
        fseek(fp_ascii, pokemon->offset, SEEK_SET);
        fread(&data_len, sizeof(data_len), 1, fp_ascii);
        fread(data_buf, 1, data_len, fp_ascii);
        data_buf[data_len] = 0;
        printf("%s\n", data_buf);
        free(data_buf);
    }
    printf("name: %s ", pokemon->name);
    printf("id: %d ", pokemon->id);
    printf("type: \033[1m%s\033[0m ", type_str[pokemon->type]);
    printf("subtype: %s\n", type_str[pokemon->subtype]);
    printf("first ability: %s\n", pokemon->ability_one->name);
    printf("descripton: %s", pokemon->ability_one->desc);

    if(pokemon->ability_two){
        if(pokemon->ability_two_hidden) {
            printf("second ability: \033[3m%s\033[0m\n", pokemon->ability_two->name);
        }
        else {
            printf("second ability: %s\n", pokemon->ability_two->name);
        }
        printf("descripton: %s", pokemon->ability_two->desc);
    }
    if(pokemon->ability_three) {
        printf("third ability: \033[3m%s\033[0m\n", pokemon->ability_three->name);
        printf("descripton: %s", pokemon->ability_three->desc);
    }
    printf("base_experience: %d ", pokemon->base_experience);
    printf("weight: %.2fkg ", pokemon->weight);
    printf("height: %.2fm ", pokemon->height);
    printf("hp: %d\n", pokemon->hp);
    printf("attack: %d ", pokemon->attack);
    printf("defense: %d ", pokemon->defense);
    printf("special_attack: %d ", pokemon->special_attack);
    printf("special_defense: %d ", pokemon->special_defense);
    printf("speed: %d\n\n", pokemon->speed);

}
void free_pokemon(POKEMON_T* pokemon) {
    free(pokemon->name);
    free_ability(pokemon->ability_one);
    if(pokemon->ability_two) free_ability(pokemon->ability_two);
    if(pokemon->ability_three) free_ability(pokemon->ability_three);
    free(pokemon);
}
void free_ability(ABILITY_T* ability) {
    free(ability->name);
    free(ability->desc);
    free(ability);
}
