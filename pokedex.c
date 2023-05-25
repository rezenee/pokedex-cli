#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

typedef struct {
	char* name;
	size_t size;
	size_t index;
} INDEX_T;
void append_and_close_into_file(FILE * input_file, FILE * output_file);

int main(int argc, char * argv[]) {
	// this is the file the ascii art will be outputted into
	FILE * fp_ascii = fopen("ascii.bin", "wb");
	// move to the directory that has the ascii art
    char * dir_to_ascii_art = "ascii_art/";
    DIR * dir = opendir(dir_to_ascii_art);
    if (dir == NULL) {
        printf("Failed to open directory.\n");
        return 1;
    }
	char * file_path = malloc(4096);
    int pkm_count = 0;
	INDEX_T **indexes = malloc(sizeof(*indexes) * pkm_count);
	struct dirent* entry;
	// read through every file in the directory
    while ((entry = readdir(dir)) != NULL) {
		// every regular file in the directory is a pokemon
        if (entry ->d_type == DT_REG) {
			memset(file_path, 0, 4096);
			strcat(file_path, dir_to_ascii_art);
			strcat(file_path, entry->d_name);
			FILE * fp_single = fopen(file_path, "rb");
			if (fp_single != NULL) {
				// the file name is the name of each pokemon
   				INDEX_T * pkm = malloc(sizeof(*pkm));
				pkm->name = strdup(entry->d_name);
				pkm->index = ftell(fp_ascii);
				append_and_close_into_file(fp_single, fp_ascii);
				pkm->size = ftell(fp_ascii) - pkm->index;
				indexes = realloc(indexes, sizeof(*indexes) * ++pkm_count);
				indexes[pkm_count -1] = pkm;
			}
			else {
				printf("Failed to open file %s\n", file_path);
			}
        }
    }
	// writing cards to index.bin also freeing
	FILE * fp_index = fopen("indexes.bin", "wb");
	fwrite(&pkm_count, sizeof(pkm_count), 1, fp_index);
	size_t name_len;
   	for (int i=0; i < pkm_count; i++) {
		name_len = strlen(indexes[i]->name);
		fwrite(&name_len, sizeof(name_len), 1, fp_index);
		fwrite(indexes[i]->name, 1, name_len, fp_index);
		fwrite(&indexes[i]->index, sizeof(indexes[i]->index), 1, fp_index);
		free(indexes[i]->name);
		free(indexes[i]);
	}
	fclose(fp_index);
	free(indexes);
	closedir(dir);
	fclose(fp_ascii);
	free(file_path);
    printf("total pokemon: %d\n", pkm_count);
    return 0;
}
void append_and_close_into_file(FILE * input_file, FILE * output_file) {
   	int ch;
	while ((ch = getc(input_file)) != EOF) {
		fputc(ch, output_file);
	}
	fclose(input_file);
}
