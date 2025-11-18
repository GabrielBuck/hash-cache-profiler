#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "hash_table.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <log_file>\n", argv[0]);
        return 1;
    }
    char* log_file = argv[1];
    HashTable* ht = ht_create(16384);
    FILE* manifest = fopen("manifest.txt", "r");
    if (!manifest) { perror("manifest.txt"); exit(1); }
    char line[2048];
    while (fgets(line, sizeof(line), manifest)) {
        line[strcspn(line, "\r\n")] = '\0';
        ht_insert(ht, line);
    }
    fclose(manifest);
    FILE* log = fopen(log_file, "r");
    if (!log) { perror(log_file); exit(1); }
    char** lines = NULL;
    size_t num = 0, cap = 0;
    while (fgets(line, sizeof(line), log)) {
        if (num == cap) {
            cap = cap ? cap * 2 : 1024;
            lines = realloc(lines, cap * sizeof(char*));
        }
        line[strcspn(line, "\r\n")] = '\0';
        lines[num++] = strdup(line);
    }
        fclose(log);
    double start = omp_get_wtime();

    #pragma omp parallel for
    for (size_t i = 0; i < num; i++) {
        char* line = lines[i];
        char url[1024];

        // Tenta achar o padrão de log HTTP: "GET <URL> HTTP/1.1"
        char* get_pos = strstr(line, "GET ");
        if (get_pos) {
            // Pula o "GET "
            get_pos += 4;

            // Fim da URL é o próximo espaço em branco depois dela
            char* space_after = strchr(get_pos, ' ');
            if (!space_after) {
                // Linha malformada, pula
                continue;
            }

            size_t len = (size_t)(space_after - get_pos);
            if (len >= sizeof(url)) {
                len = sizeof(url) - 1;  // evita estouro
            }

            memcpy(url, get_pos, len);
            url[len] = '\0';
        } else {
            // Se não tem "GET ", assumimos que a linha JÁ É a URL
            strncpy(url, line, sizeof(url) - 1);
            url[sizeof(url) - 1] = '\0';
        }

        CacheNode* node = ht_get(ht, url);
        if (node) {
            #pragma omp critical
            node->hit_count++;
        }
    }

    double end = omp_get_wtime();


    printf("Processing time: %f seconds\n", end - start);
    ht_save_results(ht, "results.csv");
    for (size_t i = 0; i < num; i++) free(lines[i]);
    free(lines);
    ht_destroy(ht);
    return 0;
}