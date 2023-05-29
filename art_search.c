#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "card.h"

void search_loop(INDEX_T** indexes, size_t total_indexes, FILE *fp);

int sort_comp(const void *a, const void *b) {
	const char* a_ptr = a;
	INDEX_T *b_ptr = *((INDEX_T**) b);
	return strcmp(a_ptr, b_ptr->name);
}
int main(int argc, char * argv[]) {
	// file is pkm_count, then: name_len, name, index; ... for all cards
	FILE * fp_index = fopen("indexes.bin", "rb");
	size_t pkm_count;
    size_t name_len;
    char * name = malloc(4096);
    size_t index;

    fread(&pkm_count, sizeof(pkm_count), 1, fp_index);
    printf("pkm_count: %d\n", pkm_count);
    INDEX_T **indexes = malloc(sizeof(*indexes) * pkm_count);

	// loads pokemon names and index from file into indexes array 
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
	// file is data_length, ascii_data ... for all cards
	FILE * fp_ascii = fopen("ascii.bin", "rb");
	search_loop(indexes, pkm_count, fp_ascii);

    // free the shit
    for(int i = 0; i< pkm_count; i++) {
        free(indexes[i]->name);
        free(indexes[i]);
    }
    fclose(fp_ascii);
    fclose(fp_index);
    free(indexes);
    free(name);
}
void search_loop(INDEX_T** indexes, size_t total_indexes, FILE * fp) {
	char * input_buf = NULL;
	char * data_buf = malloc(40000);
	size_t n = 0;

	size_t data_len = 0;
    int echo = 0;
	if(isatty(fileno(stdin)) == 0) {
		echo = 1;
	}
	for (;;) {
		printf(">> ");

		getline(&input_buf, &n, stdin);
		// remove the \n from input
		input_buf[strlen(input_buf) -1] = 0;
		if (input_buf[0] == 'q') {
			if (echo) printf("q\n");
			break;
		}
		INDEX_T **found_index = bsearch(input_buf, indexes, total_indexes, sizeof(INDEX_T*), sort_comp);
		if (found_index) {
            memset(data_buf, 0, 40000);
		    fseek(fp, found_index[0]->index, SEEK_SET);
            fread(&data_len, sizeof(data_len), 1, fp);
            int num = fread(data_buf, 1, data_len, fp);
            printf("%s\n", data_buf);
		}
	}
}
