/*
 * this program opens every file in data/ascii_data, concatenating the data
 * into a single ascii.bin file. it then attemps to create pokemon.csv with the 
 * data in data/pokemon_data. if the directory does not exist it is created.
 * if there are any missing .jsons in the directory they are regenerated.
 * then it attempts to create abilities.csv with the data in data/abilities_data
 * if the directory does not exist it is created. if there are any missing .json files
 * in the directory they are regenerated. 
 * if there are files in ascii_data, abilities_data, pokemon_data, the program makes
 * no attempt to make sure the files are valid. so don't put things in those directories
 * if they aren't the data that is expected. 
 * the purpose of this program is to generaate the data that is to be used for a1 / a2.
 * the root of the data is the ascii files for each pokemon, you must find these
 * manually. the .json files are generated using pokeapi.co.
 * pokeapi.co's api does not require setting up a key or account or anything.
 */
#include "data/cJSON.h"
#include "card.h"
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>

INDEX_T** write_ascii(FILE* fp_ascii, char* path_to_ascii_data, size_t* pkm_count);

void generate_pokemon_json(char * dir_name, char * ascii_data_dir);
void write_pokemon(FILE* fp_pokemon, char* path_to_pokemon_data, INDEX_T** indexes, size_t pkm_count);
char * process_json_pkm(char * file_path, INDEX_T** indexes, size_t total_indexes);
cJSON* get_nested_item(cJSON* root, const char** path, size_t path_len);
cJSON* load_json(char * file_path);

void generate_abilities_json(char * dir_name, char * pokemon_csv_name);
GEN_ABILITY_T * parse_pokemon(char * line_buf);
void output_to_json_ability(GEN_ABILITY_T * pokemon, char * dir_name);
void write_abilities(FILE* fp_abilities, char* path_to_abilities_data);
char * process_json_ability(char * file_path);

int sort_comp(const void *a, const void *b) {
    const char* a_ptr = a;
    INDEX_T *b_ptr = *((INDEX_T**) b);
    return strcmp(a_ptr, b_ptr->name);
}
int sort_comp_ascii(const void*a, const void*b) {
    INDEX_T *a_ptr = *((INDEX_T**) a);
    INDEX_T *b_ptr = *((INDEX_T**) b);
    return strcmp(a_ptr->name, b_ptr->name);
}
size_t write_callback(char *ptr, size_t size, size_t nmemb, FILE * file) {
    return fwrite(ptr, size, nmemb, file);
}

int main(int argc, char* argv[]) {
    FILE* fp_ascii = fopen("ascii.bin", "wb");
    // this is the source of all other data, must be provided before hand
    char* path_to_ascii_data = strdup("data/ascii_data/");
    size_t pkm_count = 0;
    // combine all ascii data from directory into bin file. allocate indexes with that data 
    printf("creating ascii.bin\n");
    INDEX_T** indexes = write_ascii(fp_ascii, path_to_ascii_data, &pkm_count);
    fclose(fp_ascii);

    char * path_to_pokemon_data = strdup("data/pokemon_data/");
    DIR * dir = opendir(path_to_pokemon_data);
    // if the dir storing pokemon data does not exist, create it.
    if(!dir) {
        printf("%s not found, creating\n");
        mkdir(path_to_pokemon_data, 0700);
    }
    closedir(dir);
    
    // generate the json for the pokemon data. use the ascii bin file as a resource.
    generate_pokemon_json(path_to_pokemon_data, path_to_ascii_data);
    free(path_to_ascii_data);

    char *pokemon_csv_name = strdup("pokemon_full.csv");
    FILE* fp_pokemon = fopen(pokemon_csv_name, "wb");
    printf("creating pokemon_full.csv\n");
    // create pokemon.csv, use indexes information as guide
    write_pokemon(fp_pokemon, path_to_pokemon_data, indexes, pkm_count);
    fclose(fp_pokemon);
    free(path_to_pokemon_data);
    for(int i = 0; i < pkm_count; i++) {
        free(indexes[i]->name);
        free(indexes[i]);
    }
    free(indexes);

    FILE* fp_abilities = fopen("abilities.csv", "w");
    char * path_to_abilities_data = strdup("data/abilities_data/");
    dir = opendir(path_to_abilities_data);
    // if the dir storing ability data does not exist, create it.
    if(!dir) {
        printf("%s not found, creating\n");
        mkdir(path_to_abilities_data, 0700);
    }
    closedir(dir);

    // use the pokemon csv as the source for the abilities to generate.
    generate_abilities_json(path_to_abilities_data, pokemon_csv_name);
    free(pokemon_csv_name);
    printf("creating abilities.csv\n");
    // combine the data in the jsons into single abilities.csv
    write_abilities(fp_abilities, path_to_abilities_data);
    fclose(fp_abilities);
    free(path_to_abilities_data);

    return 0;
}
INDEX_T** write_ascii(FILE* fp_ascii, char* path_to_ascii_data, size_t *pkm_count) {
    DIR * dir = opendir(path_to_ascii_data);
    if (dir == NULL) {
        printf("Failed to open directory.\n");
    }
    char * file_path = malloc(1024);
    INDEX_T **indexes = malloc(sizeof(*indexes) * *pkm_count);
    struct dirent* entry;
    // read through every file in the directory
    while ((entry = readdir(dir)) != NULL) {
        // every regular file in the directory is a pokemon
        if (entry ->d_type == DT_REG) {
            memset(file_path, 0, 1024);
            strcat(file_path, path_to_ascii_data);
            strcat(file_path, entry->d_name);
            FILE * fp_single = fopen(file_path, "rb");
            size_t file_len;
            if (fp_single == NULL) {
                printf("Failed to open file %s\n", file_path);
                continue;
            }
            INDEX_T * pkm = malloc(sizeof(*pkm));
            pkm->name = strdup(entry->d_name);
            pkm->offset = ftell(fp_ascii);
            fseek(fp_single, 0, SEEK_END);
            file_len = ftell(fp_single);
            fseek(fp_single, 0, SEEK_SET);
            fwrite(&file_len, sizeof(file_len), 1, fp_ascii);
            int ch;
            while ((ch = fgetc(fp_single)) != EOF) {
                fputc(ch, fp_ascii);
            }
            fclose(fp_single);
            (*pkm_count)++;
            indexes = realloc(indexes, sizeof(*indexes) * *pkm_count);
            indexes[(*pkm_count -1)] = pkm;
        }
    }

    // sort cards before writing to file
    qsort(indexes, *pkm_count, sizeof(*indexes), sort_comp_ascii);
    closedir(dir);
    free(file_path);

    return indexes;
}
void generate_pokemon_json(char * dir_name, char *ascii_data_dir) {
    char * url = strdup("https://pokeapi.co/api/v2/pokemon/");
    CURL *easy_handle = curl_easy_init();
    CURLcode res;
    curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, write_callback);
    DIR * dir = opendir(ascii_data_dir);
    struct dirent* entry;
    // read through every file in the directory
    while ((entry = readdir(dir)) != NULL) {
        // every regular file in the directory is a pokemon
        if (entry ->d_type == DT_REG) {
            // create strings for file name and url
            char * json_file_name = malloc(4096);
            char * json_url = malloc(10000);
            json_url[0] = 0;
            strcat(json_url, url);
            strcat(json_url, entry->d_name);
            json_file_name[0] = 0;
            strcat(json_file_name, dir_name);
            strcat(json_file_name, entry->d_name);
            strcat(json_file_name, ".json");
            // todo add check to see if the file already exists
            if(access(json_file_name, F_OK) == -1) {
                printf("%s not found, regenerating\n", entry->d_name);
                FILE * fp_json = fopen(json_file_name, "w");
                // update handle to have correct file and url
                curl_easy_setopt(easy_handle, CURLOPT_URL, json_url);
                curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, fp_json);
                res = curl_easy_perform(easy_handle);
                fclose(fp_json);
            }
            free(json_file_name);
            free(json_url);
        }
    }
    free(url);
    curl_easy_cleanup(easy_handle);
    closedir(dir);
}
void write_pokemon(FILE* fp_csv, char* path_to_pokemon_data, INDEX_T** indexes, size_t pkm_count) {
    char * file_name = malloc(512);
    char * buf = malloc(1024);

    DIR * dir = opendir(path_to_pokemon_data);
    struct dirent* entry;
    // add variable names
    const char variables[] = {"name,id,type,subtype,ablty_one,ablty_two,ablty_three,"
                              "base_exp,weight,height,hp,attack,defense,spec_attack,"
                              "spec_defense,speed,offset\n"};
    fwrite(variables, 1, strlen(variables), fp_csv);
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            memset(file_name, 0, 512);
            strcat(file_name, path_to_pokemon_data);
            strcat(file_name, entry->d_name);
            char * csv_entry = process_json_pkm(file_name, indexes, pkm_count);
            strcat(csv_entry, "\n");
            fwrite(csv_entry, strlen(csv_entry), 1, fp_csv);
            free(csv_entry);
        }
    }
    free(file_name);
    free(buf);
    closedir(dir);
}
char * process_json_pkm(char * file_path, INDEX_T** indexes, size_t total_indexes) {
    char* string = malloc(512);
    memset(string, 0, 512);
    
    cJSON* root = load_json(file_path);

    char * number_string = malloc(12);

    if(!root) {// if there is no json data for the pokemon
        file_path = strrchr(file_path, '/');
        file_path++;
        file_path = strsep(&file_path, ".");
        strcat(string, file_path);
        strcat(string, ",,,,,,,,,,,,,");

        INDEX_T **found_index = bsearch(file_path, indexes, total_indexes, sizeof(INDEX_T*), sort_comp);
        memset(number_string, 0, 12);
        sprintf(number_string, "%d", found_index[0]->offset);
        strcat(string, number_string);
        free(number_string);
        return string;
    } 
    // add the name field to the string
    cJSON *name = cJSON_GetObjectItem(root, "name");
    strcat(string, name->valuestring);
    strcat(string, ",");

    // add the id field to the string
    cJSON *id = cJSON_GetObjectItem(root, "id");
    sprintf(number_string, "%d", id->valueint); 
    strcat(string, number_string);
    strcat(string, ",");
    
    // add type
    const char* path_type_one[] = {"types", "0", "type", "name"};
    cJSON* type_one = get_nested_item(root, path_type_one, 4);
    strcat(string, type_one->valuestring);
    strcat(string, ",");

    // add potential subtype
    const char* path_type_two[] = {"types", "1", "type", "name"};
    cJSON* type_two_object = get_nested_item(root, path_type_two, 2);
    if(type_two_object) {
        cJSON* type_two = get_nested_item(root, path_type_two, 4);
        strcat(string, type_two->valuestring);
    }
    strcat(string, ",");

    // add the first ability to the field
    const char* path_ability_one[] = {"abilities", "0", "ability", "name"};
    cJSON* ability_one = get_nested_item(root, path_ability_one, 4);
    strcat(string, ability_one->valuestring);
    strcat(string, ",");

    // add the second ability field, if it exists
    const char* path_ability_two[] = {"abilities", "1", "ability", "name"};
    cJSON* ability_two_object = get_nested_item(root,path_ability_two, 2);
    if(ability_two_object) {
        cJSON* ability_two = get_nested_item(root, path_ability_two, 4);
        strcat(string, ability_two->valuestring);
    }
    strcat(string, ",");

    // add the third ability field, if it exists
    const char* path_ability_three[] = {"abilities", "2", "ability", "name"};
    cJSON* ability_three_object = get_nested_item(root, path_ability_three, 2);
    if(ability_three_object) {
        cJSON* ability_three = get_nested_item(root, path_ability_three, 4);
        strcat(string, ability_three->valuestring);
    }
    strcat(string, ",");

    // add the base experience field
    cJSON *base_experience = cJSON_GetObjectItem(root, "base_experience");
    memset(number_string, 0, 12);
    sprintf(number_string, "%d", base_experience->valueint);
    strcat(string, number_string);
    strcat(string, ",");

    // add the weight field
    cJSON *weight = cJSON_GetObjectItem(root, "weight");
    memset(number_string, 0, 12);
    sprintf(number_string, "%d", weight->valueint);
    strcat(string, number_string);
    strcat(string, ",");

    // add the height field
    cJSON *height = cJSON_GetObjectItem(root, "height");
    memset(number_string, 0, 12);
    sprintf(number_string, "%d", height->valueint);
    strcat(string, number_string);
    strcat(string, ",");

    // add the hp field
    const char* path_hp[] = {"stats", "0", "base_stat"};
    cJSON* hp = get_nested_item(root, path_hp, 3);
    memset(number_string, 0, 12);
    sprintf(number_string, "%d", hp->valueint);
    strcat(string, number_string);
    strcat(string, ",");

    // add the attack field
    const char* path_attack[] = {"stats", "1", "base_stat"};
    cJSON* attack = get_nested_item(root, path_attack, 3);
    memset(number_string, 0, 12);
    sprintf(number_string, "%d", attack->valueint);
    strcat(string, number_string);
    strcat(string, ",");

    // add the defense field
    const char* path_defense[] = {"stats", "2", "base_stat"};
    cJSON* defense = get_nested_item(root, path_defense, 3);
    memset(number_string, 0, 12);
    sprintf(number_string, "%d", defense->valueint);
    strcat(string, number_string);
    strcat(string, ",");

    // add the special attack field
    const char* path_special_attack[] = {"stats", "3", "base_stat"};
    cJSON* special_attack = get_nested_item(root, path_special_attack, 3);
    memset(number_string, 0, 12);
    sprintf(number_string, "%d", special_attack->valueint);
    strcat(string, number_string);
    strcat(string, ",");

    // add the special defense field
    const char* path_special_defense[] = {"stats", "4", "base_stat"};
    cJSON* special_defense = get_nested_item(root, path_special_defense, 3);
    memset(number_string, 0, 12);
    sprintf(number_string, "%d", special_defense->valueint);
    strcat(string, number_string);
    strcat(string, ",");

    // add the speed field
    const char* path_speed[] = {"stats", "5", "base_stat"};
    cJSON* speed = get_nested_item(root, path_speed, 3);
    memset(number_string, 0, 12);
    sprintf(number_string, "%d", speed->valueint);
    strcat(string, number_string);
    strcat(string, ",");

    // find the index to the ascii data. add to field
    INDEX_T **found_index = bsearch(name->valuestring, indexes, total_indexes, sizeof(INDEX_T*), sort_comp);
    memset(number_string, 0, 12);
    sprintf(number_string, "%d", found_index[0]->offset);
    strcat(string, number_string);

    cJSON_Delete(root);
    free(number_string);
    return string;
}
cJSON* get_nested_item(cJSON* root, const char** path, size_t path_len) {
    cJSON* current_item = root;
    for (int i = 0; i < path_len; ++i) {
        if (current_item == NULL) {
            return NULL;
        }
        if (atoi(path[i]) == 0 && strlen(path[i]) > 1) { // object
            current_item = cJSON_GetObjectItem(current_item, path[i]);
        } 
        else { // array
            current_item = cJSON_GetArrayItem(current_item, atoi(path[i]));
        }
    }
    return current_item;
}
void generate_abilities_json(char * dir_name, char * pokemon_csv_name) {
    FILE * fp_csv = fopen(pokemon_csv_name, "r");

    char * line_buf = NULL;
    size_t n = 0;
    size_t total_pokemon = 0;
    GEN_ABILITY_T ** pokemon_array = NULL;

    // remove variable def line
    getline(&line_buf, &n, fp_csv);
    while(getline(&line_buf, &n, fp_csv) > 0) {
        GEN_ABILITY_T * pokemon = parse_pokemon(line_buf);

        if(pokemon) {
            total_pokemon++;
            pokemon_array = realloc(pokemon_array, total_pokemon * 8);
            pokemon_array[total_pokemon - 1] = pokemon;
        }
    }

    for(int i = 0; i < total_pokemon; i++) {
        output_to_json_ability(pokemon_array[i], dir_name);
        free(pokemon_array[i]->ability_one);
        if(pokemon_array[i]->ability_two) free(pokemon_array[i]->ability_two);
        if(pokemon_array[i]->ability_three) free(pokemon_array[i]->ability_three);
        free(pokemon_array[i]);
    }
    fclose(fp_csv);
    free(pokemon_array);
    free(line_buf);
}
GEN_ABILITY_T* parse_pokemon(char * buf) {
    char * stringp = buf;
    // moving to the first ability
    strsep(&stringp, ",");
    strsep(&stringp, ",");
    strsep(&stringp, ",");
    strsep(&stringp, ",");
    char * token = strsep(&stringp, ",");
    if(strlen(token) == 0) {
        return NULL;
    }
    GEN_ABILITY_T* gen_ability = malloc(sizeof(*gen_ability));
    gen_ability->ability_one = strdup(token);
    token = strsep(&stringp, ",");
    if(strlen(token) == 0) {
        gen_ability->ability_two = 0;
        gen_ability->ability_three = 0;
        return gen_ability;
    }
    else {
        gen_ability->ability_two = strdup(token);
    }
    token = strsep(&stringp, ",");
    if(strlen(token) == 0) {
        gen_ability->ability_three = 0;
    }
    else {
        gen_ability->ability_three = strdup(token);
    }
    return gen_ability;
}
void output_to_json_ability(GEN_ABILITY_T* pokemon, char* path_to_dir) {
    CURLcode res;
    char *url = strdup("https://pokeapi.co/api/v2/ability/");
    CURL *easy_handle = curl_easy_init();
    curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, write_callback);

    char * json_file_name = malloc(4096);
    char * json_url = malloc(4096);

    // ability one
    memset(json_url, 0, 4096);
    strcat(json_url, url);
    strcat(json_url, pokemon->ability_one);
    memset(json_file_name, 0, 4096);
    strcat(json_file_name, path_to_dir);
    strcat(json_file_name, pokemon->ability_one);
    strcat(json_file_name, ".json");

    if(access(json_file_name, F_OK) == -1) {
        printf("%s ability not found, regenerating\n", pokemon->ability_one);
        FILE * fp_json = fopen(json_file_name, "w");
        curl_easy_setopt(easy_handle, CURLOPT_URL, json_url);
        curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, fp_json);
        res = curl_easy_perform(easy_handle);
        fclose(fp_json);
    }

    // ability two
    if(pokemon->ability_two) {
        memset(json_url, 0, 4096);
        strcat(json_url, url);
        strcat(json_url, pokemon->ability_two);
        memset(json_file_name, 0, 4096);
        strcat(json_file_name, path_to_dir);
        strcat(json_file_name, pokemon->ability_two);
        strcat(json_file_name, ".json");

        if(access(json_file_name, F_OK) == -1) {
            printf("%s ability not found, regenerating\n", pokemon->ability_two);
            FILE * fp_json = fopen(json_file_name, "w");
            curl_easy_setopt(easy_handle, CURLOPT_URL, json_url);
            curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, fp_json);
            res = curl_easy_perform(easy_handle);
            fclose(fp_json);
        }
    }

    // ability three
    if(pokemon->ability_three) {
        memset(json_url, 0, 4096);
        strcat(json_url, url);
        strcat(json_url, pokemon->ability_three);
        memset(json_file_name, 0, 4096);
        strcat(json_file_name, path_to_dir);
        strcat(json_file_name, pokemon->ability_three);
        strcat(json_file_name, ".json");

        if(access(json_file_name, F_OK) == -1) {
            printf("%s ability not found, regenerating\n", pokemon->ability_three);
            FILE * fp_json = fopen(json_file_name, "w");
            curl_easy_setopt(easy_handle, CURLOPT_URL, json_url);
            curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, fp_json);
            res = curl_easy_perform(easy_handle);
            fclose(fp_json);
        }
    }
    free(url);
    free(json_file_name);
    free(json_url);
    curl_easy_cleanup(easy_handle);
}

void write_abilities(FILE* fp_abilities, char* path_to_abilities_data) {
    DIR * dir = opendir(path_to_abilities_data);
    if(dir == NULL) {
        printf("failed to open directory");
    }
    char * buf = malloc(1024);
    const char variables[] = {"name,desc\n"};
    fwrite(variables, 1, strlen(variables), fp_abilities);
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry ->d_type == DT_REG) {
            // construct the file path
            memset(buf, 0, 1024);
            strcat(buf, path_to_abilities_data);
            strcat(buf, entry->d_name);

            char * csv_data = process_json_ability(buf);

            // construct the ability name from the file
            char * ability_name = strdup(entry->d_name);
            ability_name = strsep(&ability_name, ".");

            // create the full line of csv
            memset(buf, 0, 1024);
            strcat(buf, ability_name);
            strcat(buf, ",");
            strcat(buf, csv_data);
            strcat(buf, "\n");

            fwrite(buf, strlen(buf), 1, fp_abilities);
            free(csv_data);
            free(ability_name);
        }
    }
    free(buf);
    closedir(dir);
}
char * process_json_ability(char * file_path) {
    cJSON* root = load_json(file_path);

    char * string = malloc(1000);
    memset(string, 0, 1000);

    cJSON* flavor_text_entries = cJSON_GetObjectItem(root, "flavor_text_entries");
    // loop through every entry in of the flavortext
    for(int i = 0; i < cJSON_GetArraySize(flavor_text_entries); i++) {
        cJSON* entry_json = cJSON_GetArrayItem(flavor_text_entries, i);
        cJSON* entry_language = cJSON_GetObjectItem(entry_json, "language");
        cJSON* language = cJSON_GetObjectItem(entry_language, "name");

        if(strcmp(language->valuestring, "en") == 0) {
            cJSON* flavor_text = cJSON_GetObjectItem(entry_json, "flavor_text");
            strcat(string, flavor_text->valuestring);
            // remove the /n's in the description because they mess up the csv
            for (int i = 0; i < strlen(string); i++) {
                if (string[i] == '\n') {
                    string[i] = ' ';
                }
            }
            break;
        }
    }
    cJSON_Delete(root);
    return string;
}

cJSON* load_json(char * file_path) {
    FILE * fp_json = fopen(file_path, "r");
    fseek(fp_json, 0, SEEK_END);
    off_t file_size = ftell(fp_json);
    rewind(fp_json);
    char * buffer = malloc(file_size + 1);
    fread(buffer, 1, file_size, fp_json);
    buffer[file_size] = 0;
    fclose(fp_json);
    cJSON* json_data = cJSON_Parse(buffer);
    free(buffer);
    return json_data;
}
