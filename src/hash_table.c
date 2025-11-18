#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hash_table.h"

static unsigned long hash(const char* str) {
    unsigned long h = 0;
    while (*str) {
        h = *str++ + 31 * h;
    }
    return h;
}

HashTable* ht_create(size_t size) {
    HashTable* ht = malloc(sizeof(HashTable));
    ht->size = size;
    ht->buckets = calloc(size, sizeof(CacheNode*));
    return ht;
}

void ht_insert(HashTable* ht, const char* url) {
    unsigned long h = hash(url) % ht->size;
    CacheNode* node = malloc(sizeof(CacheNode));
    node->url = strdup(url);
    node->hit_count = 0;
    node->next = ht->buckets[h];
    ht->buckets[h] = node;
}

CacheNode* ht_get(HashTable* ht, const char* url) {
    unsigned long h = hash(url) % ht->size;
    CacheNode* node = ht->buckets[h];
    while (node) {
        if (strcmp(node->url, url) == 0) return node;
        node = node->next;
    }
    return NULL;
}

void ht_destroy(HashTable* ht) {
    for (size_t i = 0; i < ht->size; i++) {
        CacheNode* node = ht->buckets[i];
        while (node) {
            CacheNode* temp = node;
            node = node->next;
            free(temp->url);
            free(temp);
        }
    }
    free(ht->buckets);
    free(ht);
}

static int cmp_nodes(const void* a, const void* b) {
    CacheNode* na = *(CacheNode**)a;
    CacheNode* nb = *(CacheNode**)b;
    return strcmp(na->url, nb->url);
}

void ht_save_results(HashTable* ht, const char* filename) {
    size_t total = 0;
    for (size_t i = 0; i < ht->size; i++) {
        CacheNode* n = ht->buckets[i];
        while (n) { total++; n = n->next; }
    }
    CacheNode** list = malloc(total * sizeof(CacheNode*));
    size_t idx = 0;
    for (size_t i = 0; i < ht->size; i++) {
        CacheNode* n = ht->buckets[i];
        while (n) { list[idx++] = n; n = n->next; }
    }
    qsort(list, total, sizeof(CacheNode*), cmp_nodes);
    FILE* f = fopen(filename, "w");
    for (size_t i = 0; i < total; i++) {
        fprintf(f, "%s,%ld\n", list[i]->url, list[i]->hit_count);
    }
    fclose(f);
    free(list);
}
