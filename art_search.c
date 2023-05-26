#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "card.h"

int main(int argc, char * argv[]) {
    FILE * fp_ascii = fopen("ascii.bin", "rb");
    FILE * fp_index = fopen("indexes.bin", "rb");
    size_t pkm_count;
    fread(&pkm_count, sizeof(pkm_count), 1, fp_index);
    printf("pkm_count: %d\n", pkm_count);
	INDEX_T **indexes = malloc(sizeof(*indexes) * pkm_count);
    size_t name_len;
    size_t index;
    char * name = malloc(4096);
    for (int i=0; i < pkm_count; i++) {
        memset(name, 0, 4096);
        fread(&name_len, sizeof(name_len), 1, fp_index);
        fread(name, 1, name_len, fp_index);
        fread(&index, sizeof(index), 1, fp_index);
        INDEX_T * pkm = malloc(sizeof(*pkm));
        pkm->name = strdup(name);
        pkm->index = index;
        indexes[i] = pkm;
    }
    size_t ascii_len;
    char * ascii_art = malloc(37226);
    for(int i=0; i < pkm_count; i++) {
        memset(ascii_art, 0, 37226);
        fseek(fp_ascii, indexes[i]->index, SEEK_SET);
        fread(&ascii_len, sizeof(uint64_t), 1, fp_ascii);
        fread(ascii_art, 1, ascii_len, fp_ascii);
        printf("%s\n", ascii_art);
    }
    // free the shit
    for(int i = 0; i< pkm_count; i++) {
        free(indexes[i]->name);
        free(indexes[i]);
    }
    free(ascii_art);
    fclose(fp_ascii);
    fclose(fp_index);
    free(indexes);
    free(name);
}
