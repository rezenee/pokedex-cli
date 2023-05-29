#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdint.h>
//#include <curl/curl.h>
#include "card.h"

void append_and_close_into_file(FILE * input_file, FILE * output_file);

//size_t write_callback(char *ptr, size_t size, size_t nmemb, FILE * file) {
//    return fwrite(ptr, size, nmemb, file);
//}
int sort_comp(const void *a, const void *b) {
	INDEX_T *a_ptr = *((INDEX_T**) a);
	INDEX_T *b_ptr = *((INDEX_T**) b);
	return strcmp(a_ptr->name, b_ptr->name);
}
int main(int argc, char * argv[]) {
 //   char * url = strdup("https://pokeapi.co/api/v2/pokemon/");
  //  CURL *easy_handle = curl_easy_init();
 //   CURLcode res;
 //   curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, write_callback); 
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
    size_t pkm_count = 0;
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
			size_t file_len;
			if (fp_single != NULL) {
            //    // create strings for file name and url
            //    char * json_file_name = malloc(4096);
            //    char * json_url = malloc(10000);
            //    json_url[0] = 0;
            //    strcat(json_url, url);
            //    strcat(json_url, entry->d_name);
            //    json_file_name[0] = 0;
            //    strcat(json_file_name, entry->d_name);
            //    strcat(json_file_name, ".json");
            //    
            //    FILE * fp_json = fopen(json_file_name, "w");
            //    // update handle to have correct file and url
            //    curl_easy_setopt(easy_handle, CURLOPT_URL, json_url);
            //    curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, fp_json);
            //    res = curl_easy_perform(easy_handle);
            //    fclose(fp_json);

            //    free(json_file_name);
				// the file name is the name of each pokemon
   				INDEX_T * pkm = malloc(sizeof(*pkm));
				pkm->name = strdup(entry->d_name);
				pkm->index = ftell(fp_ascii);
				fseek(fp_single, 0, SEEK_END);
			    file_len = ftell(fp_single);
                fseek(fp_single, 0, SEEK_SET);
				fwrite(&file_len, sizeof(file_len), 1, fp_ascii);
				append_and_close_into_file(fp_single, fp_ascii);
				indexes = realloc(indexes, sizeof(*indexes) * ++pkm_count);
				indexes[pkm_count -1] = pkm;
			}
			else {
				printf("Failed to open file %s\n", file_path);
			}
        }
    }
	// sort cards before writing to file
	qsort(indexes, pkm_count, sizeof(*indexes), sort_comp);
	// writing cards to index.bin also freeing
	FILE * fp_index = fopen("indexes.bin", "wb");
	fwrite(&pkm_count, sizeof(uint64_t), 1, fp_index);
	size_t name_len;
   	for (int i=0; i < pkm_count; i++) {
		name_len = strlen(indexes[i]->name);
		fwrite(&name_len, sizeof(uint64_t), 1, fp_index);
		fwrite(indexes[i]->name, 1, name_len, fp_index);
		fwrite(&indexes[i]->index, sizeof(uint64_t), 1, fp_index);
		free(indexes[i]->name);
		free(indexes[i]);
	}
	fclose(fp_index);
	free(indexes);
	closedir(dir);
	fclose(fp_ascii);
	free(file_path);
//    free(url);
    printf("total pokemon: %d\n", pkm_count);
    return 0;
}
void append_and_close_into_file(FILE * input_file, FILE * output_file) {
   	int ch;
	while ((ch = fgetc(input_file)) != EOF) {
		fputc(ch, output_file);
	}
	fclose(input_file);
}
