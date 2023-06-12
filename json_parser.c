#include "cJSON.h"
#include "card.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char * process_json(char * filename, INDEX_T** indexes, size_t total_indexes);

int sort_comp(const void *a, const void *b) {
    const char* a_ptr = a;
    INDEX_T *b_ptr = *((INDEX_T**) b);
    return strcmp(a_ptr, b_ptr->name);
}
int main(int argc, char* argv[]) {
    FILE * fp_index = fopen("indexes.bin", "rb");
    size_t pkm_count;
    size_t name_len;
    char * name = malloc(512);
    size_t index;

    fread(&pkm_count, sizeof(pkm_count), 1, fp_index);
    printf("pkm_count: %d\n", pkm_count);
    INDEX_T **indexes = malloc(sizeof(*indexes) * pkm_count);

    // loads pokemon names and index from file into indexes array
    for (int i=0; i < pkm_count; i++) {
        memset(name, 0, 512);
        fread(&name_len, sizeof(name_len), 1, fp_index);
        fread(name, 1, name_len, fp_index);
        fread(&index, sizeof(index), 1, fp_index);

        INDEX_T * pkm = malloc(sizeof(*pkm));
        pkm->name = strdup(name);
        pkm->index = index;
        indexes[i] = pkm;
    }
    fclose(fp_index);

    char * file_path_before_name = strdup("json_data/");
    char * file_name = malloc(512);
    FILE * fp_csv = fopen("pokemon.csv", "wb");
    // create csv entry for each pokemon. write to output file.
    for(int i = 0; i < pkm_count; i++) {
        // constructing the filename
        memset(file_name, 0, 512);
        strcat(file_name, file_path_before_name);
        strcat(file_name, indexes[i]->name);
        strcat(file_name, ".json");

        char * csv_entry = process_json(file_name, indexes, pkm_count);
        strcat(csv_entry, "\n");
        fwrite(csv_entry, strlen(csv_entry), 1, fp_csv);
    }

    fclose(fp_csv);
    for(int i = 0; i < pkm_count; i++) {
        free(indexes[i]->name);
        free(indexes[i]);
    }
    free(indexes);
    free(name);
    free(file_path_before_name);
    free(file_name);
    return 0;
}

char * process_json(char * filename, INDEX_T** indexes, size_t total_indexes) {
    char* string = malloc(512);
    memset(string, 0, 512);
    
    // open and read file into buffer
    FILE * fp_json = fopen(filename, "r");

    fseek(fp_json, 0, SEEK_END);
    long fileSize = ftell(fp_json);
    fseek(fp_json, 0, SEEK_SET);
    char * buffer = malloc(fileSize + 1);
    fread(buffer, 1, fileSize, fp_json);
    buffer[fileSize] = 0;
    fclose(fp_json);

    cJSON* root = cJSON_Parse(buffer);

    char * number_string = malloc(12);
    if(root) {
        // add the name field to the string
        cJSON *name = cJSON_GetObjectItem(root, "name");
        strcat(string, name->valuestring);
        strcat(string, ",");

        // add the id field to the string
        cJSON *id = cJSON_GetObjectItem(root, "id");
        sprintf(number_string, "%d", id->valueint); // 
        strcat(string, number_string);
        strcat(string, ",");

        cJSON* abilities_array = cJSON_GetObjectItem(root, "abilities");

        // add the first ability to the field
        cJSON* ability_one_object = cJSON_GetArrayItem(abilities_array, 0);
        cJSON* ability_one_object_nested = cJSON_GetObjectItem(ability_one_object, "ability");
        cJSON* ability_one_name = cJSON_GetObjectItem(ability_one_object_nested, "name");
        strcat(string, ability_one_name->valuestring);
        strcat(string, ",");

        // add the second ability field, if it exists
        cJSON* ability_two_object = cJSON_GetArrayItem(abilities_array, 1);
        if(ability_two_object) {
            cJSON* ability_two_object_nested = cJSON_GetObjectItem(ability_two_object, "ability");
            cJSON* ability_two_name = cJSON_GetObjectItem(ability_two_object_nested, "name");
            strcat(string, ability_two_name->valuestring);
        }
        strcat(string, ",");

        // add the third ability field, if it exists
        cJSON* ability_three_object = cJSON_GetArrayItem(abilities_array, 2);
        if(ability_three_object) {
            cJSON* ability_three_object_nested = cJSON_GetObjectItem(ability_three_object, "ability");
            cJSON* ability_three_name = cJSON_GetObjectItem(ability_three_object_nested, "name");
            strcat(string, ability_three_name->valuestring);
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

        cJSON *stats = cJSON_GetObjectItem(root, "stats");

        // add the hp field
        cJSON* hp_object = cJSON_GetArrayItem(stats, 0);
        cJSON* hp = cJSON_GetObjectItem(hp_object, "base_stat");
        memset(number_string, 0, 12);
        sprintf(number_string, "%d", hp->valueint);
        strcat(string, number_string);
        strcat(string, ",");

        // add the attack field
        cJSON* attack_object = cJSON_GetArrayItem(stats, 1);
        cJSON* attack = cJSON_GetObjectItem(attack_object, "base_stat");
        memset(number_string, 0, 12);
        sprintf(number_string, "%d", attack->valueint);
        strcat(string, number_string);
        strcat(string, ",");

        // add the defense field
        cJSON* defense_object = cJSON_GetArrayItem(stats, 2);
        cJSON* defense = cJSON_GetObjectItem(defense_object, "base_stat");
        memset(number_string, 0, 12);
        sprintf(number_string, "%d", defense->valueint);
        strcat(string, number_string);
        strcat(string, ",");

        // add the special attack field
        cJSON* special_attack_object = cJSON_GetArrayItem(stats, 3);
        cJSON* special_attack = cJSON_GetObjectItem(special_attack_object, "base_stat");
        memset(number_string, 0, 12);
        sprintf(number_string, "%d", special_attack->valueint);
        strcat(string, number_string);
        strcat(string, ",");

        // add the special attack defense field
        cJSON* special_defense_object = cJSON_GetArrayItem(stats, 4);
        cJSON* special_defense = cJSON_GetObjectItem(special_defense_object, "base_stat");
        memset(number_string, 0, 12);
        sprintf(number_string, "%d", special_defense->valueint);
        strcat(string, number_string);
        strcat(string, ",");

        // add the speed field
        cJSON* speed_object = cJSON_GetArrayItem(stats, 5);
        cJSON* speed = cJSON_GetObjectItem(speed_object, "base_stat");
        memset(number_string, 0, 12);
        sprintf(number_string, "%d", speed->valueint);
        strcat(string, number_string);
        strcat(string, ",");

        // find the index to the ascii data. add to field
        INDEX_T **found_index = bsearch(name->valuestring, indexes, total_indexes, sizeof(INDEX_T*), sort_comp);
        memset(number_string, 0, 12);
        sprintf(number_string, "%d", found_index[0]->index);
        strcat(string, number_string);

        cJSON_Delete(root);
    } // if there is no json data for the pokemon
    else {
        strsep(&filename, "/");
        filename = strsep(&filename, ".");
        strcat(string, filename);
        strcat(string, ",,,,,,,,,,,,,");

        INDEX_T **found_index = bsearch(filename, indexes, total_indexes, sizeof(INDEX_T*), sort_comp);
        memset(number_string, 0, 12);
        sprintf(number_string, "%d", found_index[0]->index);
        strcat(string, number_string);
    }
    free(number_string);
    free(buffer);
    return string;
}
