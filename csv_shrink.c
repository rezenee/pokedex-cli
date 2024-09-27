#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "card.h"
 
unsigned parse_line(char* buf, unsigned max_id);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("usage: ./parser pokemon.csv output.csv max_id\n");
        return -1;
    }

    FILE * fp_pokemon = fopen(argv[1], "r");
    FILE * fp_output = fopen(argv[2], "w");
    unsigned max_id = atoi(argv[3]);
    size_t total_pokemon = 0;
    POKEMON_T ** pokemon_array = NULL;
    char* line_buf = 0;
    size_t n = 0;
    // skip line with variable definitions
    getline(&line_buf, &n, fp_pokemon);
    fwrite(line_buf, strlen(line_buf), 1, fp_output);
    while(getline(&line_buf, &n, fp_pokemon) > 0) {
        if( parse_line(strdup(line_buf), max_id) ) {
            fwrite(line_buf, strlen(line_buf), 1, fp_output);
        }

    }
    free(line_buf);
    fclose(fp_pokemon);
    fclose(fp_output);

    return 0;
}

unsigned parse_line(char* buf, unsigned max_id) {
    char* stringp = buf;
    strsep(&stringp, ",");
    char* id = strsep(&stringp, ",");
    if(*id == 0) {
        free(buf);
        return 0;
    }
    if (atof(id) > max_id) {
        free(buf);
        return 0;
    }
    free(buf);
    return 1;
}
